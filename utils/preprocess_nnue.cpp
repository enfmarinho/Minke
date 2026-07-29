#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <variant>

#include "eval/nnue/arch.h"
#include "eval/nnue/simd.h"

// TODO: change this to std::expected when change to C++23
using Err = std::string;
template <typename T>
using Result = std::variant<T, Err>;

using RawNetwork = std::array<std::byte, sizeof(Network)>;

void permute_network_ft_params(Network& net) {
    using namespace simd;

    // permutation for packus lane-crossing, necessary to avoid doing so in the network inference hot-path
    if constexpr (simd::PACKUS_LANE_COUNT > 1) {
        struct alignas(16) Chunk128 {
            std::array<int16_t, 8> data;
        };
        std::array<Chunk128, PACKUS_LANE_COUNT> temp;

        // Permute FT weights
        Chunk128* weights_chunk = reinterpret_cast<Chunk128*>(net.ft_weights);
        const std::size_t total_weight_chunks = sizeof(net.ft_weights) / sizeof(Chunk128);

        for (std::size_t i = 0; i < total_weight_chunks; i += PACKUS_LANE_COUNT) {
            for (std::size_t j = 0; j < PACKUS_LANE_COUNT; ++j)
                temp[j] = weights_chunk[i + j];
            for (std::size_t j = 0; j < PACKUS_LANE_COUNT; ++j)
                weights_chunk[i + j] = temp[PACKUS_LANE_ORDER[j]];
        }

        // Permute FT biases
        Chunk128* biases_chunk = reinterpret_cast<Chunk128*>(net.ft_biases);
        const std::size_t total_bias_chunks = sizeof(net.ft_biases) / sizeof(Chunk128);

        for (std::size_t i = 0; i < total_bias_chunks; i += PACKUS_LANE_COUNT) {
            for (std::size_t j = 0; j < PACKUS_LANE_COUNT; ++j)
                temp[j] = biases_chunk[i + j];
            for (std::size_t j = 0; j < PACKUS_LANE_COUNT; ++j)
                biases_chunk[i + j] = temp[PACKUS_LANE_ORDER[j]];
        }
    }
}

Network process(const RawNetwork& raw_net) {
    std::size_t offset = 0;
    auto advance = [&](std::size_t bytes) {
        assert(offset + bytes <= sizeof(Network));
        offset += bytes;
    };

    Network net;

    // Copy FT weights
    const std::size_t ft_w_size = sizeof(net.ft_weights);
    std::memcpy(net.ft_weights, raw_net.data() + offset, ft_w_size);
    advance(ft_w_size);

    // Copy FT biases
    const std::size_t ft_b_size = sizeof(net.ft_biases);
    std::memcpy(net.ft_biases, raw_net.data() + offset, ft_b_size);
    advance(ft_b_size);

    permute_network_ft_params(net);

    // Transform raw l1_weights, bullet output (transposed: (output_buckets * l2_size) x l1_size) into VNNI layout
    const int8_t* raw_l1 = reinterpret_cast<const int8_t*>(raw_net.data() + offset);
    for (int l1_idx = 0; l1_idx < L1_SIZE; ++l1_idx) {
        for (int out_bucket_idx = 0; out_bucket_idx < OUTPUT_BUCKET_COUNT; ++out_bucket_idx) {
            for (int l2_idx = 0; l2_idx < L2_SIZE; ++l2_idx) {
                const int8_t w = raw_l1[(out_bucket_idx * L2_SIZE + l2_idx) * L1_SIZE + l1_idx];
                net.l1_weights[out_bucket_idx][l1_idx / 4][l2_idx][l1_idx % 4] = w;
            }
        }
    }
    advance(sizeof(net.l1_weights));

    // Copy L1 biases
    const int32_t* raw_l1b = reinterpret_cast<const int32_t*>(raw_net.data() + offset);
    for (int out_bucket_idx = 0; out_bucket_idx < OUTPUT_BUCKET_COUNT; ++out_bucket_idx) {
        for (int l2_idx = 0; l2_idx < L2_SIZE; ++l2_idx) {
            const int32_t b = raw_l1b[out_bucket_idx * L2_SIZE + l2_idx];
            net.l1_biases[out_bucket_idx][l2_idx] = b;
        }
    }
    advance(sizeof(net.l1_biases));

    // Transform raw l2_weights, bullet output (transposed: (output_buckets * l3_size) x l2_size)
    const int32_t* raw_l2 = reinterpret_cast<const int32_t*>(raw_net.data() + offset);
    for (int l2_idx = 0; l2_idx < ACTUAL_L2_SIZE; ++l2_idx) {
        for (int out_bucket_idx = 0; out_bucket_idx < OUTPUT_BUCKET_COUNT; ++out_bucket_idx) {
            for (int l3_idx = 0; l3_idx < L3_SIZE; ++l3_idx) {
                const int32_t w = raw_l2[(out_bucket_idx * L3_SIZE + l3_idx) * ACTUAL_L2_SIZE + l2_idx];
                net.l2_weights[out_bucket_idx][l2_idx][l3_idx] = w;
            }
        }
    }
    advance(sizeof(net.l2_weights));

    // L2 biases
    const std::size_t l2b_size = sizeof(net.l2_biases);
    std::memcpy(net.l2_biases, raw_net.data() + offset, l2b_size);
    advance(l2b_size);

    // Transform raw l3_weights, bullet output (transposed: output_buckets x l3_size)
    const int32_t* raw_l3 = reinterpret_cast<const int32_t*>(raw_net.data() + offset);
    for (int l3_idx = 0; l3_idx < L3_SIZE; ++l3_idx) {
        for (int out_bucket_idx = 0; out_bucket_idx < OUTPUT_BUCKET_COUNT; ++out_bucket_idx) {
            const int32_t w = raw_l3[out_bucket_idx * L3_SIZE + l3_idx];
            net.l3_weights[out_bucket_idx][l3_idx] = w;
        }
    }
    advance(sizeof(net.l3_weights));

    // L3 biases
    const std::size_t l3b_size = sizeof(net.l3_biases);
    std::memcpy(net.l3_biases, raw_net.data() + offset, l3b_size);
    advance(l3b_size);

    assert(offset == raw_net.size()); // redundant, but for clearness

    return net;
}

Result<RawNetwork> read_in_raw_network(const std::string& in_path) {
    std::ifstream in_file(in_path, std::ios::binary | std::ios::ate);
    if (!in_file) {
        return "Failed to open input file: " + in_path;
    }

    const auto in_file_size = in_file.tellg();
    if (in_file_size == std::streampos(-1)) {
        return "Failed to determine input file size.";
    }
    in_file.seekg(0, std::ios::beg); // go back to beginning of file

    if (static_cast<std::size_t>(in_file_size) != sizeof(Network)) {
        return "Input file has an unexpected size.";
    }

    RawNetwork raw_net;
    if (!in_file.read(reinterpret_cast<char*>(&raw_net), sizeof(raw_net))) {
        return "Failed to read input file.";
    }

    return raw_net;
}

std::optional<std::string> write_out(const std::string& out_path, std::span<const uint8_t> bytes) {
    std::ofstream out_file(out_path, std::ios::binary);
    if (!out_file) {
        return "Failed to open output file: " + out_path;
    }

    out_file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size_bytes());
    if (!out_file) {
        return "Failed to write output file.";
    }

    return std::nullopt;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <preprocessed_net.nnue> <processed_net.nnue>\n";
        return EXIT_FAILURE;
    }

    const std::string in_path = argv[1];
    auto raw_net_or_err = read_in_raw_network(in_path);

    // File read failed
    if (std::holds_alternative<Err>(raw_net_or_err)) {
        std::cerr << "Err: " << std::get<Err>(raw_net_or_err) << std::endl;
        return EXIT_FAILURE;
    }

    const auto& raw_net = std::get<RawNetwork>(raw_net_or_err);
    auto net = process(raw_net);

    const std::string out_path = argv[2];
    std::span out_bytes{reinterpret_cast<const uint8_t*>(&net), sizeof(net)};
    auto err_msg = write_out(out_path, out_bytes);

    // File write failed
    if (err_msg.has_value()) {
        std::cerr << err_msg.value() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
