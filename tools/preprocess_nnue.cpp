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
    2809500, 454441,   378250,  1054062, 2673167, 665665,  762696,  483775,  3336749, 1196673, 1014629, 1483122,
    4752334, 301750,   1755201, 345203,  1755394, 466181,  652786,  258213,  373064,  663835,  1168798, 1004290,
    1158315, 296812,   1195296, 834826,  529308,  328622,  3047073, 2921742, 941963,  1765362, 867999,  307829,
    584902,  2602525,  893390,  458475,  914065,  1498329, 1020659, 126242,  1986282, 455404,  194869,  849273,
    1180541, 306083,   744596,  657114,  266300,  2361909, 4238030, 1791904, 1089722, 1035842, 1066955, 2268571,
    398324,  287908,   544948,  3015254, 1974375, 741127,  1465211, 351113,  968379,  6272395, 114140,  1473542,
    210055,  620991,   378064,  2311872, 442143,  609774,  2722371, 262011,  118363,  1661269, 970989,  8184995,
    2217877, 1233077,  1836102, 1802049, 446060,  842856,  863964,  1368355, 1329023, 110637,  401933,  324504,
    355945,  1037322,  1688911, 1437940, 1583747, 2050769, 152070,  1429714, 589371,  435754,  112583,  929169,
    783170,  1261639,  565758,  231990,  1163193, 261428,  2005724, 1590571, 899330,  282620,  205977,  1839583,
    813984,  10276517, 1041845, 1295968, 1120957, 416174,  363163,  503623,  1585005, 296838,  2142153, 560599,
    88580,   400863,   2209413, 1208873, 2703841, 218185,  315763,  3344796, 677093,  427347,  836389,  425790,
    446994,  2191653,  1105343, 1873760, 982188,  1337912, 8318692, 1295800, 490893,  1134445, 1008535, 1443371,
    347886,  440730,   2133746, 2188251, 861629,  2632337, 191787,  1265606, 1974417, 166713,  931960,  1556319,
    1017481, 1215734,  718506,  1503912, 502878,  501207,  1633804, 1845796, 964592,  1793778, 1295290, 297991,
    1251056, 1480692,  1302097, 329457,  1075372, 1976872, 398118,  338942,  149647,  1495063, 265795,  1504408,
    350060,  2877302,  881639,  158084,  1521107, 332074,  213606,  510865,  825225,  2003766, 803125,  168088,
    1151942, 330051,   1676657, 680856,  1177208, 1943095, 1638187, 4464675, 351937,  664099,  410169,  1252255,
    1471554, 134188,   218105,  1006719, 769701,  683061,  808352,  887350,  173014,  1815705, 521138,  5084781,
    944315,  266796,   1722133, 2080700, 846970,  1111283, 1652052, 613832,  308558,  461133,  466833,  1283716,
    394078,  389070,   121683,  1481546, 2063091, 3376321, 2868923, 441225,  812526,  465085,  4400134, 701013,
    1061666, 492873,   693619,  1058901, 313858,  332033,  348440,  792826,  428154,  2304364, 481137,  653021,
    1205740, 1149319,  367975,  934897,  660142,  245209,  1985024, 1171916, 573709,  210005,  342677,  2058010,
    1206039, 1696060,  879742,  951596,  546567,  390860,  2062660, 622066,  662602,  821620,  289082,  3153405,
    981211,  2327929,  745975,  777548,  837432,  240920,  713884,  1530480, 713803,  388299,  745692,  258244,
    2142001, 508104,   681647,  1357945, 384432,  583866,  712817,  1493338, 737427,  1137393, 1433447, 1186103,
    2027768, 1271310,  739821,  159251,  819727,  631682,  908151,  170101,  532357,  854941,  879545,  3761950,
    1851853, 1370864,  674597,  1004964, 1548209, 1479263, 886965,  856198,  1121041, 1819627, 621111,  421607,
    583461,  945464,   927900,  954918,  479361,  1121080, 500370,  451520,  706091,  670437,  1105315, 628138,
    1171435, 531065,   465221,  2437889, 950855,  99039,   2080145, 328980,  198757,  591490,  645902,  2206446,
    427061,  348182,   1201048, 487344,  208964,  516543,  408187,  1101306, 332973,  2014383, 1256253, 1511578,
    580311,  1885521,  1585830, 682459,  578115,  5654270, 351198,  2797524, 766364,  1982469, 217596,  810684,
    1115661, 220737,   452971,  969681,  801454,  897333,  1659544, 177763,  1710386, 467562,  312949,  1051831,
    1840538, 1421298,  55917,   1606537, 2987511, 662379,  256509,  351233,  599783,  426407,  2254167, 670544,
    375284,  2185757,  433626,  2377249, 2540925, 818836,  755269,  2156447, 1025177, 414469,  865023,  1879397,
    806838,  1236987,  1040659, 909724,  549422,  329527,  1537118, 1641202, 528846,  114621,  130300,  406622,
    827333,  2768666,  1375738, 506155,  1400070, 703684,  1299450, 1537987, 1262130, 2009443, 583801,  1062428,
    449833,  340870,   1593818, 522725,  601216,  842491,  1665777, 516078,  994825,  1050371, 550612,  1014141,
    1239316, 1069855,  1425574, 1908792, 2159517, 1172571, 497709,  1886089, 695454,  1450555, 244989,  444344,
    2193438, 1506567,  181776,  2576296, 338077,  407713,  359671,  269472,  1302508, 427553,  490365,  986675,
    726389,  1620640,  943548,  502852,  467754,  505659,  2574380, 348618,  186285,  725623,  862382,  1145396,
    2014822, 318151,   123216,  6269747, 414469,  710392,  634171,  1765993, 393179,  1750462, 3788478, 203823,
    3355046, 1027445,  917955,  1624615, 1274128, 764894,  287999,  800157,  2547498, 6763488, 94814,   224452,
    1420797, 961429,   256880,  1385825, 600500,  840015,  343247,  688480,  665110,  2568113, 1898048, 2209779,
    1847136, 174452,   442093,  516455,  92482,   403367,  921756,  1004771, 446937,  299283,  1906823, 482730,
    666624,  3289121,  2667151, 2840847, 589802,  815170,  1154104, 1345380, 317961,  2586085, 2874709, 558946,
    857103,  1601786,  491360,  671335,  1399678, 382859,  835126,  654522,  5606629, 996135,  840780,  1272491,
    1299369, 868700,   750968,  766169,  776750,  1202280, 520540,  1204287, 1655551, 1361991, 739406,  250392,
    411908,  1700121,  1329920, 1266377, 1666857, 452391,  359335,  818063,  1115532, 748580,  102722,  1324291,
    4342101, 487870,   1212767, 9779117, 792968,  1457268, 1648231, 952098,  1679836, 8487072, 1368968, 460312,
    4375871, 739071,   1708253, 2371010, 1475595, 593157,  904588,  1205703, 989982,  9862864, 623401,  1297116,
    1318967, 1213052,  314972,  268973,  2197399, 216003,  493984,  801938,  1395431, 648374,  545915,  1192847,
    681837,  758394,   1576192, 675172,  135150,  1553651, 1523995, 1586366, 532591,  581226,  651061,  914885,
    2464380, 317556,   1583987, 2873782};

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
