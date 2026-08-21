#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
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

using RawNetworkData = std::array<uint8_t, sizeof(Network)>;
using RawNetwork = std::unique_ptr<RawNetworkData>;

constexpr std::array<size_t, PAIR_COUNT> activation_counts = {
    1858391, 1169548, 1543448, 1474654, 1045212,  331929,  3333282, 194757,  535889,  1355707, 1682462, 226188,
    2963083, 6021034, 982159,  514056,  291702,   2146109, 765850,  360204,  720853,  490325,  1081795, 1114100,
    538277,  902327,  771163,  1003677, 931427,   187238,  566099,  376726,  774391,  220990,  3740056, 902717,
    1300929, 3479044, 1323644, 305615,  797963,   1496365, 593181,  582837,  1111145, 399569,  334615,  806624,
    99234,   161595,  1047401, 324763,  1526624,  1474170, 489800,  2268954, 1767641, 250419,  4223650, 74153,
    1791733, 1906278, 1558290, 2017849, 757475,   547703,  555469,  567048,  979000,  233972,  920859,  772093,
    297830,  2325177, 1484225, 1009431, 2977424,  313466,  856484,  1544098, 1301674, 391807,  820570,  1237720,
    1264299, 617237,  221704,  2008484, 477996,   1170159, 1225915, 1292283, 1332725, 722297,  1078756, 789960,
    248572,  281593,  380585,  876724,  6649914,  1162735, 977602,  617561,  1906560, 949480,  126889,  1083515,
    287650,  456960,  226536,  890299,  732294,   876217,  795539,  686415,  475382,  577440,  553614,  938720,
    1895886, 770637,  403551,  1496937, 730970,   232163,  1083689, 920312,  879412,  1940620, 472661,  1539651,
    585055,  1731677, 519613,  190120,  1036969,  2077790, 345612,  1283658, 1126812, 837303,  342563,  393656,
    1278852, 1379842, 686932,  76673,   566166,   1499284, 321464,  2183356, 7373238, 1464178, 768864,  791522,
    573332,  1159367, 966716,  413599,  1007824,  798831,  566779,  1332869, 701957,  1432745, 400032,  1663635,
    206437,  2108838, 628745,  1385388, 591987,   315892,  2444184, 1176906, 688536,  782494,  3509032, 853601,
    1254824, 114247,  378107,  2751296, 1495697,  122955,  2091457, 374220,  209117,  905476,  121044,  239701,
    1011097, 288721,  97417,   1820345, 308303,   699255,  968368,  2245192, 402617,  519969,  665742,  984725,
    398857,  1352604, 632801,  1691306, 620369,   1849020, 2329748, 69963,   2234579, 919865,  1169504, 613106,
    1189689, 321965,  2019948, 1576916, 484299,   1764149, 2584994, 1285050, 2164015, 1305799, 2092583, 70320,
    1761553, 1384770, 1920488, 245580,  885254,   856050,  1246031, 1585248, 1279035, 336879,  2749320, 8755805,
    966111,  826257,  240000,  653482,  1910155,  699792,  2837324, 960540,  1430661, 397631,  1759566, 534594,
    1444326, 1303765, 2513093, 197657,  1297325,  2137102, 3208766, 857857,  3224382, 649342,  722529,  2506586,
    846125,  1035281, 2425353, 157421,  5200762,  95243,   2330485, 462332,  431821,  1647255, 169904,  1692149,
    523991,  3085526, 731832,  796122,  304046,   1514694, 291247,  253237,  204541,  220921,  2849262, 1997295,
    574646,  819985,  290676,  244902,  529146,   1137133, 808244,  3090461, 1628034, 2325239, 1666166, 532433,
    1716573, 1319253, 446676,  1659170, 504427,   2642981, 561873,  761642,  371084,  792516,  592194,  650947,
    161067,  8796775, 1066999, 213743,  6351865,  471912,  158350,  803953,  682259,  2105073, 635941,  268283,
    1923443, 552760,  200322,  1719631, 626930,   1110020, 1023994, 548809,  2070159, 1068539, 604540,  344519,
    574446,  2990001, 775937,  1815189, 694885,   1047684, 2177161, 2001177, 529278,  634004,  1243116, 483600,
    749911,  6397540, 276329,  344141,  555710,   252172,  1112714, 915283,  735766,  715096,  535014,  4179281,
    1087988, 689253,  698390,  483582,  125159,   728333,  506672,  439330,  682758,  267888,  671087,  797729,
    444580,  368617,  237483,  1130691, 1008443,  1538090, 1727850, 1150391, 426634,  1972345, 842835,  3450916,
    384049,  168195,  1992245, 1720155, 626218,   536523,  357742,  3463825, 3107363, 1371992, 2164498, 226213,
    403120,  880940,  1682966, 793787,  319936,   333202,  1394853, 2176391, 153219,  4990646, 483005,  706121,
    236173,  417670,  2794120, 213517,  990137,   601343,  803883,  938005,  605311,  2036970, 1258584, 2010219,
    1346881, 626129,  503303,  680957,  1073113,  297167,  2417907, 717731,  1038813, 542705,  4024302, 1555133,
    777074,  795491,  876982,  3970909, 1163024,  1168294, 2240328, 971762,  718890,  621097,  892232,  2558350,
    456115,  2772787, 2220496, 1880868, 11169841, 1223417, 2513113, 1183918, 1100811, 664303,  341799,  506863,
    6613022, 413150,  816987,  533922,  1764631,  576813,  1511423, 2686499, 465184,  816486,  791960,  1911742,
    221454,  757515,  2215529, 1060307, 2558546,  1765862, 994745,  1651306, 2033856, 233821,  929602,  1177362,
    297968,  1263885, 4528117, 288750,  1681995,  1836500, 810811,  833166,  715945,  604157,  377784,  423210,
    918779,  695577,  1531104, 337937,  400545,   611592,  611637,  2359705, 783972,  501938,  4214837, 186571,
    1475753, 966616,  554879,  1196665, 2070957,  1010901, 930763,  873431,  1305511, 565960,  1069210, 583945,
    174470,  581715,  197503,  1083344, 82384,    1450409, 1950957, 714548,  1354441, 536692,  3725489, 674810,
    1000882, 379984,  1455678, 588904,  844044,   1052352, 121420,  1658464, 1104182, 3288990, 459681,  1032602,
    446711,  966120,  2065323, 376618,  2749923,  805687,  1140707, 1094073, 619385,  686425,  477452,  2695371,
    9375403, 377630,  947096,  931271,  323414,   1258384, 1484794, 1125156, 1335978, 879523,  2587688, 1384642,
    1265319, 454717,  2188967, 910631,  314706,   644472,  1460098, 2386460, 489620,  660218,  551533,  900862,
    357615,  556709,  1063369, 2581005, 388632,   177189,  392184,  1152351, 1445243, 3405481, 867139,  597711,
    1993638, 1058083, 245365,  2619952, 581520,   1679146, 303954,  1024692, 859851,  1586124, 747452,  438682,
    1765841, 1902473, 406807,  1041327, 1217154,  2444481, 1113029, 532431,  1948883, 822995,  1398163, 1089261,
    233493,  1413459, 490608,  1119242, 190204,   754060,  397528,  760555,  1339454, 2171052, 1035579, 1883213,
    910023,  1680849, 1221794, 644875,  798620,   454268,  3395906, 1432234, 2105300, 826281,  125071,  5464395,
    957256,  803534,  1702850, 1159196};

std::array<uint16_t, L1_SIZE> build_permutation() {
    std::array<uint16_t, PAIR_COUNT> order;
    std::iota(order.begin(), order.end(), 0);

    std::stable_sort(order.begin(), order.end(),
                     [](uint16_t a, uint16_t b) { return activation_counts[a] > activation_counts[b]; });

    // perm_full[new_idx] = old_idx
    std::array<uint16_t, L1_SIZE> perm_full;
    for (size_t k = 0; k < PAIR_COUNT; ++k) {
        perm_full[k] = order[k];
        perm_full[k + PAIR_COUNT] = order[k] + PAIR_COUNT;
    }
    return perm_full;
}

std::unique_ptr<Network> transpose(const RawNetwork& raw_net) {
    std::unique_ptr<Network> net = std::make_unique_for_overwrite<Network>();
    const uint8_t* base_ptr = reinterpret_cast<const uint8_t*>(raw_net->data());

    // Copy FT weights
    std::memcpy(net->ft_weights, base_ptr + offsetof(Network, ft_weights), sizeof(net->ft_weights));

    // Copy FT biases
    std::memcpy(net->ft_biases, base_ptr + offsetof(Network, ft_biases), sizeof(net->ft_biases));

    // Transform raw l1 weights, bullet output (transposed: (output_buckets * l2_size) x l1_size) into VNNI layout
    std::span<const int8_t> raw_l1w{reinterpret_cast<const int8_t*>(base_ptr + offsetof(Network, l1_weights)),
                                    sizeof(net->l1_weights) / sizeof(int8_t)};
    for (int l1_idx = 0; l1_idx < L1_SIZE; ++l1_idx) {
        for (int out_bucket_idx = 0; out_bucket_idx < OUTPUT_BUCKET_COUNT; ++out_bucket_idx) {
            for (int l2_idx = 0; l2_idx < L2_SIZE; ++l2_idx) {
                const int8_t w = raw_l1w[(out_bucket_idx * L2_SIZE + l2_idx) * L1_SIZE + l1_idx];
                net->l1_weights[out_bucket_idx][l1_idx / 4][l2_idx][l1_idx % 4] = w;
            }
        }
    }

    // Copy L1 biases
    std::span<const int32_t> raw_l1b{reinterpret_cast<const int32_t*>(base_ptr + offsetof(Network, l1_biases)),
                                     sizeof(net->l1_biases) / sizeof(int32_t)};
    for (int out_bucket_idx = 0; out_bucket_idx < OUTPUT_BUCKET_COUNT; ++out_bucket_idx) {
        for (int l2_idx = 0; l2_idx < L2_SIZE; ++l2_idx) {
            const int32_t b = raw_l1b[out_bucket_idx * L2_SIZE + l2_idx];
            net->l1_biases[out_bucket_idx][l2_idx] = b;
        }
    }

    // Transform raw l2 weights, bullet output (transposed: (output_buckets * l3_size) x l2_size)
    std::span<const int32_t> raw_l2w{reinterpret_cast<const int32_t*>(base_ptr + offsetof(Network, l2_weights)),
                                     sizeof(net->l2_weights) / sizeof(int32_t)};
    for (int l2_idx = 0; l2_idx < ACTUAL_L2_SIZE; ++l2_idx) {
        for (int out_bucket_idx = 0; out_bucket_idx < OUTPUT_BUCKET_COUNT; ++out_bucket_idx) {
            for (int l3_idx = 0; l3_idx < L3_SIZE; ++l3_idx) {
                const int32_t w = raw_l2w[(out_bucket_idx * L3_SIZE + l3_idx) * ACTUAL_L2_SIZE + l2_idx];
                net->l2_weights[out_bucket_idx][l2_idx][l3_idx] = w;
            }
        }
    }

    // L2 biases
    std::memcpy(net->l2_biases, base_ptr + offsetof(Network, l2_biases), sizeof(net->l2_biases));

    // Transform raw l3 weights, bullet output (transposed: output_buckets x l3_size)
    std::span<const int32_t> raw_l3w{reinterpret_cast<const int32_t*>(base_ptr + offsetof(Network, l3_weights)),
                                     sizeof(net->l3_weights) / sizeof(int32_t)};
    for (int l3_idx = 0; l3_idx < L3_SIZE; ++l3_idx) {
        for (int out_bucket_idx = 0; out_bucket_idx < OUTPUT_BUCKET_COUNT; ++out_bucket_idx) {
            const int32_t w = raw_l3w[out_bucket_idx * L3_SIZE + l3_idx];
            net->l3_weights[out_bucket_idx][l3_idx] = w;
        }
    }

    // L3 biases
    std::memcpy(net->l3_biases, base_ptr + offsetof(Network, l3_biases), sizeof(net->l3_biases));

    return net;
}

// Reorders the L1_SIZE ("neuron pair") dimension of a raw network so that
// pair-indices with similar (low) activation frequency land in the same 4-wide chunk. This
// increases the fraction of all-zero uint8 chunks that propagate_l1's SparseIterator can skip.
void repermute_for_sparsity(std::unique_ptr<Network>& net) {
    const auto perm_full = build_permutation();

    std::array<int16_t, L1_SIZE> tmp{};

    // ft weights
    for (size_t f = 0; f < NUM_KING_BUCKETS * INPUT_LAYER_SIZE; ++f) {
        int16_t* row = net->ft_weights + f * L1_SIZE;
        for (size_t new_idx = 0; new_idx < L1_SIZE; ++new_idx)
            tmp[new_idx] = row[perm_full[new_idx]];
        std::memcpy(row, tmp.data(), sizeof(tmp));
    }

    // ft biases
    for (size_t new_idx = 0; new_idx < L1_SIZE; ++new_idx)
        tmp[new_idx] = net->ft_biases[perm_full[new_idx]];
    std::memcpy(net->ft_biases, tmp.data(), sizeof(tmp));

    // l1 weights
    int8_t old_l1w[OUTPUT_BUCKET_COUNT][L1_SIZE / 4][L2_SIZE][4]; // TODO put this on the heap
    std::memcpy(old_l1w, net->l1_weights, sizeof(net->l1_weights));
    for (int out_bucket_idx = 0; out_bucket_idx < OUTPUT_BUCKET_COUNT; ++out_bucket_idx) {
        for (int l1_chunk_idx = 0; l1_chunk_idx < L1_SIZE / 4; ++l1_chunk_idx) {
            for (int l2_idx = 0; l2_idx < L2_SIZE; ++l2_idx) {
                for (int l1_rem = 0; l1_rem < 4; ++l1_rem) {
                    const int new_full_idx = l1_chunk_idx * 4 + l1_rem;
                    const int old_full_idx = perm_full[new_full_idx];

                    net->l1_weights[out_bucket_idx][l1_chunk_idx][l2_idx][l1_rem] =
                        old_l1w[out_bucket_idx][old_full_idx / 4][l2_idx][old_full_idx % 4];
                }
            }
        }
    }
}

void permute_network_ft_params(std::unique_ptr<Network>& net) {
    using namespace simd;

    // permutation for packus lane-crossing, necessary to avoid doing so in the network inference hot-path
    if constexpr (simd::PACKUS_LANE_COUNT > 1) {
        struct alignas(16) Chunk128 {
            std::array<int16_t, 8> data;
        };
        std::array<Chunk128, PACKUS_LANE_COUNT> temp;

        // Permute FT weights
        Chunk128* weights_chunk = reinterpret_cast<Chunk128*>(net->ft_weights);
        const std::size_t total_weight_chunks = sizeof(net->ft_weights) / (sizeof(Chunk128));

        for (std::size_t i = 0; i < total_weight_chunks; i += PACKUS_LANE_COUNT) {
            for (std::size_t j = 0; j < PACKUS_LANE_COUNT; ++j)
                temp[j] = weights_chunk[i + j];
            for (std::size_t j = 0; j < PACKUS_LANE_COUNT; ++j)
                weights_chunk[i + j] = temp[PACKUS_LANE_ORDER[j]];
        }

        // Permute FT biases
        Chunk128* biases_chunk = reinterpret_cast<Chunk128*>(net->ft_biases);
        const std::size_t total_bias_chunks = sizeof(net->ft_biases) / (sizeof(Chunk128));

        for (std::size_t i = 0; i < total_bias_chunks; i += PACKUS_LANE_COUNT) {
            for (std::size_t j = 0; j < PACKUS_LANE_COUNT; ++j)
                temp[j] = biases_chunk[i + j];
            for (std::size_t j = 0; j < PACKUS_LANE_COUNT; ++j)
                biases_chunk[i + j] = temp[PACKUS_LANE_ORDER[j]];
        }
    }
}

Result<RawNetwork> read_in_raw_network(const std::string& in_path) {
    std::ifstream in_file(in_path, std::ios::binary);
    if (!in_file) {
        return "Failed to open input file: " + in_path;
    }

    size_t size = std::filesystem::file_size(in_path);
    if (size != sizeof(Network)) {
        return "Input file has an unexpected size.";
    }

    RawNetwork raw_net = std::make_unique_for_overwrite<RawNetworkData>();
    if (!in_file.read(reinterpret_cast<char*>(raw_net->data()), sizeof(RawNetworkData))) {
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
    auto net = transpose(raw_net);
    repermute_for_sparsity(net);
    permute_network_ft_params(net);

    const std::string out_path = argv[2];
    std::span out_bytes{reinterpret_cast<const uint8_t*>(net.get()), sizeof(Network)};
    auto err_msg = write_out(out_path, out_bytes);

    // File write failed
    if (err_msg.has_value()) {
        std::cerr << err_msg.value() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
