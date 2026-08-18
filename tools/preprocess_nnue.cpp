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
    1863387, 1526960, 1193428,  2184994, 564377,   1536950, 1245130,  581048,  1192050,  1157422,  955899,  872324,
    649702,  1311216, 224483,   784773,  935311,   2952700, 2291720,  1041460, 1915915,  802698,   1191539, 420148,
    976838,  1834699, 1237627,  1434011, 1735057,  1144729, 635897,   1029448, 707362,   340820,   1200844, 492213,
    451983,  1488858, 733607,   1374949, 186665,   1225410, 426793,   4069714, 732375,   3562915,  258994,  1269199,
    4738188, 518779,  1339818,  334389,  3237606,  4646651, 296936,   1697290, 1629637,  5413756,  1934165, 420996,
    2757108, 973822,  12568760, 2441650, 2396223,  221460,  735726,   730557,  627580,   1475339,  2220370, 430906,
    1301625, 3478255, 871175,   2176551, 1897448,  8419674, 2185081,  899647,  2759763,  1702734,  538140,  775105,
    1213425, 372524,  1084896,  835447,  13011604, 599100,  2023944,  1457144, 1019136,  812420,   1834301, 616133,
    603893,  2020389, 847834,   1006957, 6654831,  1533776, 236404,   1624480, 203183,   435491,   747329,  1550671,
    255954,  2060289, 900182,   703710,  2871483,  565414,  12081294, 2670658, 2262491,  1817529,  506787,  187316,
    642193,  352796,  1600847,  4247610, 1840606,  3053390, 1173300,  3513100, 3224852,  686101,   3415569, 1954482,
    1442083, 538299,  1018498,  367322,  880848,   607465,  1738334,  1046519, 1195538,  1066467,  1050877, 995708,
    1855284, 3116740, 675482,   916934,  906511,   1980336, 268284,   1409198, 323680,   375532,   380809,  3119013,
    386238,  555034,  1555623,  743089,  530269,   1210286, 484367,   709828,  463849,   1017264,  1345484, 124639,
    385038,  612348,  1204032,  557799,  1800532,  2097638, 292621,   1179895, 2356323,  663240,   2717348, 541395,
    1489901, 3317485, 427217,   259569,  628968,   770048,  262084,   3273773, 711062,   526787,   370709,  786468,
    958114,  898933,  321586,   600633,  882304,   1270980, 4354898,  533725,  1054386,  1111895,  779526,  912737,
    1150087, 1537541, 1856986,  5227252, 1562936,  1564706, 359054,   3127129, 1380531,  9193326,  377942,  1435711,
    335189,  1638013, 1021972,  630092,  1585919,  841852,  1247298,  822884,  2720980,  794611,   402740,  1032405,
    1643266, 1074594, 2985030,  4039634, 84415,    225477,  776989,   4129301, 2625309,  2081884,  537686,  2876313,
    958477,  3358550, 1138755,  810057,  1519345,  1471954, 1966837,  1857999, 644893,   907849,   1699851, 214362,
    1829360, 1788066, 2086970,  1208499, 1423924,  144059,  2326737,  1827884, 412867,   121000,   595501,  349067,
    803994,  1198174, 908593,   1058110, 357757,   3488292, 463543,   453763,  2742610,  2990407,  1063308, 861920,
    618746,  1038252, 1070958,  915226,  1903393,  1335498, 388316,   340664,  3695265,  364605,   1263902, 1827390,
    338151,  929458,  948525,   972318,  1374064,  488252,  2748157,  1599820, 196295,   1976547,  2153205, 2095807,
    386051,  1193295, 4848442,  1268401, 436560,   566122,  936377,   1246523, 1862287,  1414811,  1288714, 431853,
    865461,  485697,  909461,   881373,  3213954,  952342,  332275,   1200942, 1709043,  1143224,  616581,  351345,
    1799784, 1335994, 355756,   579779,  1476446,  466693,  556317,   763744,  1093953,  1731241,  1002384, 757621,
    198421,  2739013, 2099665,  918469,  2135623,  4075596, 394194,   1445390, 633152,   1089525,  1563271, 1735456,
    2235427, 550719,  1557546,  581438,  918801,   445701,  609160,   1083129, 504939,   1358089,  3738260, 2801841,
    632723,  1323024, 3037345,  502424,  1262083,  4302879, 1362175,  7735881, 2123409,  423319,   568892,  592850,
    3676005, 709401,  4401914,  237944,  500713,   3200577, 1807662,  1977579, 1926743,  1111111,  1434298, 1200766,
    1937787, 2591657, 1382597,  2256566, 799721,   2573763, 1716615,  1107687, 3073449,  3056029,  270642,  1235784,
    604532,  1371595, 346302,   1852354, 6616279,  2309567, 617152,   2522045, 233034,   459328,   1381163, 1507246,
    446524,  3401111, 234468,   2949724, 1368385,  1272228, 870224,   1971584, 1116038,  790550,   4388380, 982006,
    718579,  366870,  1024528,  439285,  968918,   372475,  1473973,  2229245, 1087422,  895495,   1484343, 690721,
    3228004, 4592986, 448089,   454510,  860407,   449737,  2069390,  891976,  1081946,  12814864, 559656,  1534051,
    661758,  1819796, 244525,   1372968, 510352,   1728873, 1232408,  817788,  1482121,  643268,   309403,  1094038,
    956744,  1056799, 2228740,  3210481, 1059749,  765910,  2986435,  1687355, 340383,   467335,   2484580, 1257340,
    1314162, 95765,   2969679,  2396745, 169795,   963421,  903262,   1751699, 2173737,  410194,   225592,  480763,
    1142990, 1742138, 2128620,  458203,  968755,   665391,  2372841,  760509,  392044,   1311385,  1008202, 412183,
    812702,  1736075, 911134,   664140,  889172,   1564413, 454646,   462948,  2124988,  3041163,  993047,  670037,
    1100545, 905381,  1228843,  655488,  3333258,  234078,  1973993,  1914279, 1976543,  1168736,  2549368, 9189196,
    1139874, 692919,  1460358,  4052433, 2119614,  2129293, 758308,   272556,  547086,   1159599,  1434728, 1995566,
    5346335, 608128,  1357678,  714040,  1167212,  1252839, 1237197,  1348562, 998965,   1386220,  509107,  600712,
    618627,  214179,  944923,   1887281, 2780967,  2276150, 2127913,  1955497, 1939973,  711360,   1638272, 5909307,
    1062711, 966596,  2125827,  1360164, 1645941,  1012912, 199913,   4294704, 761625,   2605123,  1438140, 1932275,
    2333174, 1272391, 1383365,  1822760, 4573145,  886275,  3723155,  1063201, 491526,   910050,   318128,  1542277,
    2878051, 437848,  486840,   366581,  491351,   1402127, 378067,   2391508, 730370,   980347,   1315520, 321938,
    1654533, 1548222, 2787254,  2007337, 1197283,  776968,  768857,   2865733, 10631319, 1240370,  598301,  2353350,
    1450384, 529709,  2008714,  4110199, 833256,   294690,  698852,   717575,  2083812,  1529326,  2642748, 1268959,
    1760832, 763116,  5016659,  1761622, 861174,   944490,  825369,   578827,  2866314,  1350175,  896760,  449246,
    1631492, 1557155, 956135,   1769452, 652445,   693331,  1453487,  1442793, 680352,   624858,   573900,  1153453,
    406662,  756635,  253463,   415207};

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
