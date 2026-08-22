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
    433716,  336076,  975382,  667135,  1647072,  163717,  1079874, 1108753, 9769675, 643773,   620167,  1734742,
    838134,  877217,  2894599, 621289,  469206,   172707,  865291,  975269,  405857,  1566601,  556405,  184926,
    598085,  305926,  3217427, 1063545, 239453,   559112,  283576,  2361025, 571747,  631961,   749029,  2822458,
    454330,  672273,  852183,  126941,  341755,   371131,  1286252, 769527,  447575,  713974,   408012,  208155,
    1976494, 672633,  2932703, 611449,  1743393,  1010487, 227385,  2642926, 970719,  1084425,  393927,  299865,
    1292640, 654939,  2339982, 512648,  209417,   372696,  1381917, 2045721, 481289,  305922,   1229126, 875702,
    2567739, 650008,  439706,  1434082, 880979,   1365133, 116865,  586662,  305468,  238246,   420829,  284735,
    226931,  1755722, 222273,  545793,  1168007,  252375,  669291,  1443020, 1366848, 343106,   587518,  711953,
    737362,  781185,  1157123, 1678979, 298675,   455151,  941556,  1588716, 132438,  781564,   400370,  461283,
    279979,  1004854, 627677,  272394,  2087889,  444891,  390663,  112853,  1055941, 505036,   577317,  5859485,
    1641751, 608167,  217000,  313017,  488757,   337480,  745509,  371750,  3258854, 320087,   298970,  1663675,
    432079,  413880,  342727,  1066904, 486930,   492573,  1353581, 1311319, 568246,  3575708,  244382,  668458,
    582662,  105683,  745590,  706826,  2185493,  897585,  1739133, 784034,  2895939, 3741443,  735702,  715634,
    653081,  741951,  532036,  1591763, 374433,   1161100, 341717,  450090,  395509,  755778,   683189,  158479,
    7984767, 1630581, 707903,  2343921, 493316,   228090,  357499,  2449893, 478463,  835613,   982850,  279301,
    276276,  1119976, 910252,  2283515, 1500104,  559733,  354185,  269072,  725500,  269839,   402056,  889678,
    1855830, 224636,  1818613, 1754288, 338309,   2476613, 436155,  596113,  365663,  1684742,  236373,  566371,
    4346636, 595371,  1610737, 1158575, 1247195,  1294619, 188139,  1239599, 1958433, 1502079,  9349773, 141977,
    71505,   261464,  910326,  844513,  617897,   1783915, 776224,  677627,  414847,  1693756,  1247548, 784962,
    338760,  611423,  544792,  2051235, 302804,   1248117, 1216023, 5532134, 1568660, 603545,   483822,  347794,
    661943,  277315,  1006302, 480840,  3430572,  115556,  278238,  2295719, 1358010, 1757377,  296053,  313183,
    675292,  2251559, 218751,  944784,  292946,   877704,  97267,   381504,  103956,  604468,   1172750, 789832,
    202299,  1361091, 669331,  896199,  624726,   183902,  1678232, 298073,  170125,  334843,   126798,  107701,
    555943,  187143,  342213,  392297,  948843,   544705,  102222,  291745,  342836,  228512,   454336,  892250,
    760076,  395691,  264660,  1015662, 1509478,  218803,  825112,  536862,  273532,  1304489,  484022,  1676121,
    2249606, 2861585, 1103510, 593792,  397780,   828364,  1607977, 1286206, 621151,  1272026,  448051,  147042,
    268528,  123635,  3043018, 2786882, 190145,   2264420, 953772,  773087,  503476,  1180120,  1183759, 890260,
    588898,  3658892, 886186,  3884392, 481108,   769756,  429326,  520021,  1457638, 1007152,  251932,  5799135,
    1897522, 1425617, 1299587, 611651,  4451061,  1533892, 2040741, 2960681, 1465192, 2396174,  481227,  118379,
    213190,  352908,  907826,  1028304, 223559,   1297901, 86662,   631162,  1437072, 2999261,  359226,  1997449,
    667659,  884429,  794234,  1238033, 510854,   363370,  810657,  432916,  1211990, 1406781,  96822,   192016,
    1935299, 344058,  563551,  882482,  744530,   144933,  1265535, 618691,  1006816, 1071201,  418318,  216065,
    221070,  889544,  1476180, 794876,  478754,   199790,  1047437, 174908,  880557,  623168,   764819,  472820,
    424399,  362898,  573437,  1119194, 505565,   1112554, 520812,  2205730, 599061,  4686111,  1257148, 1896357,
    1474746, 2186425, 415508,  437649,  108749,   1166649, 1132736, 1007415, 101626,  1815813,  851323,  945205,
    255777,  1014702, 262292,  442793,  149484,   670134,  1006493, 260868,  2555655, 1294528,  920675,  1454634,
    292105,  258632,  1629751, 679056,  1741786,  311456,  1850919, 1088611, 1197079, 1691708,  1114997, 344834,
    976267,  934305,  772435,  955938,  100760,   586267,  793642,  534496,  716300,  791376,   187587,  145348,
    504094,  677959,  1441498, 1229363, 631347,   1958653, 607058,  164901,  113340,  221472,   408563,  269646,
    2299054, 1665017, 243822,  516509,  820885,   821677,  585486,  654206,  291783,  2292072,  714767,  405152,
    1960376, 761788,  715855,  1534020, 173117,   131624,  495351,  3769423, 850549,  1699840,  387817,  1023844,
    199598,  1880366, 830225,  2159130, 953225,   436547,  559435,  2910369, 3482210, 584000,   283025,  1759574,
    1638458, 409673,  1139030, 300981,  326485,   724821,  556612,  295163,  595376,  335697,   267641,  372267,
    603507,  776016,  65266,   759137,  835632,   1690390, 1071843, 6600626, 1613295, 992087,   417341,  642727,
    767230,  817262,  810288,  631258,  342763,   2476506, 460653,  118475,  4546741, 1263134,  3905380, 794923,
    1134262, 505253,  267031,  434196,  2363331,  293911,  630760,  1408512, 208702,  638178,   115212,  520648,
    1399581, 305173,  206037,  2322380, 686671,   380699,  555482,  1273899, 1091938, 147602,   1009446, 310100,
    111216,  1784992, 628589,  1319573, 791613,   1178535, 251061,  1008993, 609138,  937514,   2237049, 1790775,
    1582908, 512081,  966786,  1012364, 1793486,  3544588, 769830,  99538,   2601337, 626472,   1570247, 686002,
    1231069, 162214,  1376770, 211533,  327269,   1511198, 351399,  462657,  954184,  311521,   1095048, 306416,
    649881,  745241,  747978,  1269620, 844250,   309988,  2284264, 260925,  5425044, 1287816,  807528,  2015714,
    1302032, 1056277, 973383,  1085549, 1520217,  3453924, 772643,  1252355, 172919,  1524270,  541682,  283395,
    488302,  769721,  2173352, 175651,  1195060,  323332,  391790,  643509,  641835,  966320,   1973515, 414496,
    537189,  380598,  756621,  475275,  284566,   411062,  424007,  155136,  494558,  494981,   1318611, 1803561,
    1655546, 1199412, 449987,  3564926, 1270380,  900505,  650734,  855086,  956591,  575576,   1602941, 1133504,
    1054317, 76906,   1904737, 103550,  425897,   178176,  385433,  1351936, 1244862, 1070281,  844538,  1806731,
    537445,  970400,  372683,  1288089, 128362,   2871003, 117877,  1248683, 717373,  10314411, 1551349, 2069289,
    1655438, 236211,  1017309, 1090188, 10044743, 388624,  800666,  183071,  3460316, 1314082,  2755863, 3284188,
    648124,  953685,  3159296, 897797,  397111,   187016,  947708,  1614047, 1032360, 692842,   1125892, 536813,
    882209,  1012566, 1255409, 1015390, 1123759,  2145010, 582150,  529716,  66339,   480882,   1688439, 1123789,
    1035746, 1295796, 186367,  102752,  2525326,  2237759, 529353,  235685,  318825,  1254646,  417072,  155166,
    1301196, 210298,  701771,  352495,  578046,   409401,  374975,  547755,  736072,  1027347,  164817,  2080388,
    66720,   333864,  171973,  1348079, 1054515,  1185215, 1677580, 690684,  855769,  852886,   907268,  1054048,
    449492,  873318,  260409,  614169,  379271,   311518,  825379,  526323,  370946,  2030961,  1247633, 425364};

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
