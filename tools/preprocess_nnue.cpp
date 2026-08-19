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
    533725,  463543,  775105,  2573763, 1624480,  693331,  1357678,  1089525,  698852,  2720980,  349067,   334389,
    1761622, 2128620, 530269,  915226,  1856986,  2759763, 2878051,  995708,   1799784, 3041163,  1840606,  1084896,
    2986435, 2097638, 2176551, 2129293, 918469,   84415,   1386220,  321938,   768857,  644893,   2119614,  1534051,
    2990407, 1769452, 1852354, 451983,  703710,   600712,  378067,   2123409,  1697290, 385038,   865461,   1074594,
    2372841, 234468,  911134,  1150087, 426793,   1966837, 1829360,  436560,   1742138, 935311,   2969679,  956135,
    2262491, 1457144, 905381,  2099665, 1533776,  966596,  1460358,  1272391,  817788,  454510,   776968,   3210481,
    663240,  891976,  430906,  415207,  1976547,  616581,  765910,   1019136,  526787,  618627,   2866314,  1006957,
    1252839, 790550,  1760832, 1360164, 2220370,  1585919, 258994,   4075596,  812420,  10631319, 2135623,  214362,
    1012912, 841852,  812702,  2670658, 761625,   492213,  1066467,  910050,   880848,  412183,   467335,   1488858,
    898933,  2173737, 253463,  1197283, 12568760, 3073449, 604532,   900182,   1383365, 899647,   1937787,  1262083,
    221460,  1827884, 1736075, 717575,  1557155,  557799,  375532,   1434728,  3213954, 1046519,  1600847,  335189,
    1555623, 635897,  906511,  95765,   2441650,  918801,  1350175,  1442083,  579779,  386238,   1348562,  3695265,
    423319,  1702734, 268284,  2757108, 2333174,  4069714, 1564413,  1939973,  459328,  196295,   1314162,  2081884,
    6654831, 1716615, 1371595, 686101,  714040,   3237606, 1173300,  357757,   799721,  2095807,  3056029,  1645941,
    632723,  386051,  1563271, 484367,  437848,   630092,  757621,   642193,   1629637, 2748157,  1402127,  709828,
    1819796, 1056799, 776989,  784773,  998965,   1225410, 13011604, 2717348,  1193428, 4388380,  2060289,  2007337,
    6616279, 410194,  224483,  624858,  1081946,  2069390, 491526,   485697,   367322,  1550671,  2642748,  1094038,
    655488,  2549368, 1063201, 370709,  2605123,  871175,  4646651,  1021972,  2086970, 3513100,  968918,   4247610,
    1029448, 412867,  1973993, 1519345, 973822,   4354898, 903262,   936377,   568892,  1915915,  1654533,  518779,
    1237197, 735726,  2229245, 779526,  1751699,  309403,  711360,   1024528,  1476446, 1954482,  5909307,  1435711,
    1557546, 3401111, 968755,  366581,  366870,   233034,  388316,   541395,   9193326, 1728873,  259569,   958477,
    581438,  1159599, 402740,  956744,  392044,   550719,  573900,   4401914,  2185081, 861920,   618746,   810057,
    1301625, 1536950, 1315520, 1564706, 272556,   449737,  825369,   502424,   627580,  2125827,  3358550,  121000,
    889172,  1142990, 822884,  214179,  372475,   1093953, 1143224,  2391508,  1599820, 756635,   466693,   3053390,
    439285,  763744,  1362175, 718579,  453763,   1807662, 4052433,  1335498,  1473973, 538140,   1971584,  1358089,
    617152,  3200577, 607465,  296936,  1414811,  3224852, 1374949,  1008202,  559656,  4294704,  169795,   2522045,
    1887281, 1731241, 1063308, 612348,  3488292,  4302879, 446524,   124639,   359054,  1380531,  1382597,  1934165,
    600633,  332275,  1208499, 1738334, 1482121,  1193295, 1445390,  1932275,  944923,  2484580,  198421,   406662,
    1862287, 633152,  364605,  394194,  458203,   234078,  872324,   2291720,  1453487, 2625309,  556317,   908593,
    3478255, 3037345, 1139874, 510352,  1903393,  803994,  1475339,  1434298,  2023944, 292621,   1976543,  1822760,
    1409198, 352796,  509107,  1834301, 4110199,  1311385, 1107687,  608128,   1863387, 491351,   664140,   652445,
    462948,  1507246, 1195538, 506787,  1269199,  255954,  1638013,  747329,   1247298, 958114,   547086,   1484343,
    1537541, 907849,  3333258, 144059,  1323024,  2396223, 4848442,  1268959,  2801841, 1857999,  262084,   2309567,
    980347,  1914279, 225477,  598301,  5227252,  1168736, 680352,   3415569,  912737,  3317485,  1270980,  340664,
    1381163, 948525,  758308,  1070958, 1204032,  2083812, 1548222,  4738188,  294690,  1687355,  2153205,  1240370,
    555034,  237944,  2952700, 1368385, 1272228,  763116,  2871483,  1191539,  5346335, 1339818,  2780967,  1955497,
    770048,  420996,  730370,  944490,  3273773,  3127129, 270642,   323680,   1450384, 1827390,  628968,   1232408,
    886275,  1192050, 2124988, 2276150, 616133,   690721,  3738260,  909461,   1788066, 1041460,  12081294, 649702,
    435491,  1210286, 445701,  1562936, 2353350,  1709043, 1111895,  1237627,  454646,  5413756,  2326737,  786468,
    244525,  896760,  2591657, 802698,  1735456,  8419674, 1157422,  595501,   1489901, 448089,   1002384,  993047,
    500713,  675482,  1200766, 609160,  1980336,  599100,  1834699,  2235427,  225592,  709401,   1542277,  3116740,
    537686,  1977579, 1179895, 730557,  1372968,  3723155, 380809,   427217,   565414,  760509,   1153453,  1017264,
    972318,  794611,  643268,  199913,  4573145,  9189196, 707362,   346302,   488252,  372524,   2876313,  340383,
    1138755, 203183,  976838,  564377,  1100545,  1638272, 1643266,  881373,   1116038, 377942,   1058110,  1311216,
    1235784, 1213425, 1699851, 2228740, 1032405,  2184994, 486840,   833256,   963421,  2008714,  1442793,  743089,
    504939,  1198174, 321586,  1345484, 340820,   2256566, 1059749,  2949724,  578827,  882304,   1167212,  2787254,
    236404,  480763,  665391,  1144729, 1735057,  1228843, 1054386,  449246,   1434011, 538299,   835447,   592850,
    351345,  2985030, 2742610, 1245130, 1111111,  4039634, 603893,   1050877,  982006,  1800532,  529709,   355756,
    1038252, 2020389, 566122,  955899,  187316,   5016659, 661758,   1423924,  1200942, 861174,   2127913,  733607,
    4592986, 1855284, 1529326, 581048,  2356323,  1438140, 860407,   1246523,  1926743, 711062,   2739013,  1471954,
    1526960, 916934,  186665,  463849,  318128,   1631492, 7735881,  12814864, 2396745, 3228004,  338151,   1817529,
    431853,  4129301, 1062711, 1087422, 1018498,  732375,  1995566,  1263902,  929458,  870224,   1374064,  1257340,
    895495,  3562915, 3676005, 3119013, 847834,   670037,  1897448,  1335994,  1083129, 1268401,  2865733,  692919,
    420148,  1200844, 1288714, 952342};

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
