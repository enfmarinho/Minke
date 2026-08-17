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
    772617,  598880,  519784,  1305857, 1138089, 1093832, 796804,  283417,  1247586, 641125,  513974,  2066896, 719291,
    1725023, 1233774, 273209,  606441,  1917717, 848092,  1324450, 906316,  293503,  421928,  303368,  437927,  1179233,
    691954,  2899942, 321554,  677132,  689007,  3944028, 772031,  706445,  5071673, 557300,  1223661, 706000,  512747,
    93830,   1076140, 1093100, 292354,  503643,  4058131, 1011116, 1007299, 1957358, 901831,  1344561, 1568589, 736068,
    1293213, 442002,  2043431, 1442509, 7080985, 808906,  429943,  420335,  420341,  368104,  415033,  515232,  216493,
    3606111, 1212426, 1594638, 1027606, 1523723, 1431191, 568887,  3777534, 479405,  292512,  567888,  2374486, 731815,
    1745208, 2257952, 1128053, 488492,  351365,  1389626, 83970,   1431652, 439753,  114021,  422358,  451747,  1239208,
    5482889, 822193,  741314,  847914,  1515707, 404384,  631292,  286722,  494543,  1010082, 945215,  692014,  510389,
    1647697, 1065268, 1497908, 928853,  453725,  826303,  472849,  330873,  2900992, 800011,  1113560, 248571,  42224,
    693595,  1717633, 1920221, 922107,  561015,  249148,  227172,  9043932, 1534406, 471955,  280846,  747628,  796773,
    1709950, 909434,  740887,  3437236, 487841,  64483,   342850,  1110876, 709371,  834087,  2575099, 747583,  510539,
    471187,  2120765, 4487368, 825059,  3007475, 2397381, 1871876, 140745,  735365,  1194210, 1503205, 388952,  1236150,
    1256556, 1658011, 916567,  603946,  1577079, 167218,  765457,  1405846, 1360064, 457046,  1279435, 1438492, 440432,
    201964,  1134356, 1286425, 731678,  742921,  1383024, 371824,  1913187, 201762,  718417,  230783,  631995,  1503509,
    823593,  3154029, 1403549, 1241847, 213428,  365943,  626620,  1434331, 1089347, 351261,  234711,  305014,  2145603,
    726889,  1048953, 2429784, 808818,  2399708, 625871,  593814,  1155037, 679151,  836897,  1028015, 3539098, 578546,
    609084,  769736,  427709,  189898,  2025537, 1082843, 1547776, 1253531, 864867,  353748,  3585832, 1757909, 446661,
    696340,  514126,  881432,  755498,  331762,  2226916, 949940,  884758,  872524,  540691,  230074,  490351,  581447,
    2774419, 381959,  1214053, 672425,  529466,  3785015, 409801,  1155117, 425173,  1517088, 450701,  1244509, 375842,
    1787182, 667971,  413816,  1867512, 634574,  436344,  623833,  1683490, 897445,  969084,  671971,  273977,  1504091,
    1222851, 459030,  515744,  1049942, 1716842, 356184,  3766619, 1122154, 472469,  460218,  346295,  601427,  1746424,
    1340707, 153363,  1421292, 699664,  911425,  745502,  948568,  216469,  9886581, 568633,  514843,  1042738, 882781,
    7570912, 635358,  547351,  175773,  1090633, 194880,  1761509, 1941121, 240476,  3661093, 1126142, 1204327, 240784,
    805006,  1739467, 453904,  485059,  481864,  2731762, 288265,  642876,  1816655, 889115,  981229,  878422,  2004847,
    2708723, 1700036, 627797,  1662836, 1272495, 737629,  1108233, 637586,  1718727, 749375,  626799,  479528,  1691483,
    728159,  1052095, 571671,  474412,  515929,  466409,  2169301, 906309,  1522082, 827857,  571333,  3520252, 221418,
    310562,  356294,  4123328, 354201,  753893,  345897,  1390080, 121757,  673861,  990866,  1452881, 1925884, 437824,
    662392,  1773785, 280999,  395449,  409252,  2784920, 604420,  1047570, 1144934, 542049,  1132391, 333055,  110810,
    750638,  373703,  2573799, 362205,  4090336, 1774840, 213917,  2047957, 207206,  314755,  2086070, 1471136, 270338,
    134631,  1065706, 1217072, 209046,  8519875, 241247,  621618,  519909,  1648040, 2815427, 291537,  810448,  1146231,
    846050,  490831,  675354,  1298393, 652875,  1998919, 1233871, 588861,  195779,  698283,  1540890, 826359,  255827,
    286132,  1174921, 1786766, 1138411, 413788,  191748,  1013133, 1751613, 450594,  640364,  568764,  741314,  493338,
    1347983, 635290,  445649,  1254944, 870130,  404148,  1742789, 292132,  663019,  108609,  833789,  507007,  160668,
    401259,  412435,  674866,  1482402, 779942,  776186,  321175,  712176,  391700,  661986,  492667,  222188,  267966,
    145286,  775891,  1362781, 463934,  1387478, 2168685, 122006,  95225,   352035,  1086596, 2146056, 684467,  1210283,
    1363789, 1668758, 797435,  586398,  1621656, 547261,  1200101, 922744,  1696943, 1215548, 631776,  2051171, 528988,
    1487190, 195814,  767081,  434964,  631579,  1004306, 535201,  522763,  2153573, 462066,  1407429, 329585,  364114,
    336716,  449546,  1160605, 631850,  273523,  147133,  1825363, 2610230, 893511,  308700,  3050439, 460699,  529492,
    1315387, 494826,  346242,  3214703, 916270,  555136,  2107553, 586255,  1128687, 641913,  582312,  2788235, 2410517,
    2513512, 134297,  714973,  759304,  206146,  2181281, 2601179, 229617,  1020333, 282345,  606254,  696846,  1667334,
    242651,  1066382, 1333837, 1759424, 1675116, 1244458, 348514,  987959,  688087,  131045,  1263747, 404450,  194296,
    303694,  2730466, 539868,  545970,  234635,  465514,  2997412, 1025168, 1154653, 269835,  1640309, 305333,  791941,
    1088158, 194812,  1629409, 638370,  633958,  227477,  1118196, 239654,  280792,  1969426, 128818,  378572,  951649,
    1139058, 787997,  514648,  905292,  2229322, 1035028, 1253972, 309837,  321871,  270890,  395939,  504578,  718024,
    559821,  433712,  984664,  610737,  262996,  913244,  1864676, 1027345, 151481,  850912,  1100360, 362966,  2125585,
    4811390, 487913,  1096783, 1132297, 1467385, 1710437, 1492450, 547878,  1100921, 706464,  377961,  985147,  2126562,
    322263,  209805,  537345,  359693,  968672,  906049,  375460,  2029061, 1062494, 1093523, 1280069, 9241932, 405939,
    493752,  1471042, 370343,  259093,  617666,  690723,  510164,  650605,  2901660, 1915562, 361131,  848574,  1129869,
    1332522, 2020823, 2240937, 397274,  426927,  514578,  1707029, 1090405, 503224,  326552,  824185,  765303,  677751,
    1466815, 359051,  1359531};

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
