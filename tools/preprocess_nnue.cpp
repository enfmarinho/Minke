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
    2596122, 515305,   430223,  929590,   3516108, 779931,  772239,  721724,  3341203, 1518362, 938433,  1235794,
    4674126, 408504,   1635320, 407450,   2155606, 445614,  901656,  357810,  394745,  735744,  984195,  1252720,
    954755,  284022,   923046,  1172374,  537948,  325416,  2193514, 2752950, 858098,  2159553, 697993,  431315,
    520929,  2887003,  946307,  436696,   546052,  999797,  1099749, 187479,  2134004, 430472,  220356,  903458,
    1038315, 258273,   754152,  1085718,  223633,  2259063, 4310461, 1828778, 815548,  1118885, 1190693, 2709387,
    452181,  333197,   502479,  2524829,  1961702, 833064,  1619237, 464553,  1241078, 6027546, 139467,  1275216,
    221854,  467563,   429334,  2684449,  509828,  516008,  2759574, 237537,  106085,  1456965, 1068501, 8241706,
    2276357, 1586076,  1500879, 1750078,  429158,  799676,  772177,  1472152, 1054309, 100831,  727352,  318791,
    298209,  854946,   2326778, 1219860,  1844239, 2287177, 169165,  1524764, 1012903, 438271,  121066,  935096,
    692814,  1514385,  848173,  260946,   1510085, 420294,  1418183, 1947072, 991045,  258878,  235273,  1347376,
    1034200, 10361403, 1028014, 1031552,  834035,  392658,  481442,  567454,  1567401, 271466,  2010383, 533669,
    86408,   390359,   2695039, 1338567,  2864822, 253920,  378744,  3267876, 455219,  300068,  845605,  518310,
    578047,  2145716,  1072799, 1513017,  922252,  1265994, 8358585, 1242218, 827435,  1588513, 1035863, 1395255,
    363425,  509214,   1645634, 2560095,  921683,  2350878, 185378,  1293936, 1459722, 241369,  878660,  1969922,
    869373,  2220642,  741465,  1590944,  400176,  591681,  1325156, 1279504, 939255,  1801992, 1222120, 306152,
    1118863, 1246949,  1246292, 347979,   1164802, 1717240, 432707,  460023,  143840,  1295437, 407682,  1273729,
    290310,  2310893,  1235044, 148819,   1721640, 282684,  190431,  421564,  786907,  1820690, 1219809, 160308,
    1299064, 303008,   1427326, 548853,   1276223, 2145086, 1418622, 4877231, 251307,  664508,  346754,  1297937,
    1581072, 154517,   221183,  1256483,  704292,  637270,  871488,  1025430, 275146,  1588641, 963517,  5214882,
    986816,  395512,   1464907, 2186712,  795182,  808157,  2282257, 639916,  423317,  546167,  409377,  1555413,
    423013,  947834,   146455,  2057767,  1695262, 3494753, 2871427, 420986,  900113,  537134,  4366368, 722860,
    715696,  545007,   744152,  1014653,  397208,  276044,  373064,  906905,  584709,  2095705, 478204,  579629,
    1083503, 1180354,  391598,  829255,   549242,  254666,  2522223, 1028448, 486351,  307140,  558860,  1686063,
    1074569, 1295630,  859328,  939809,   704465,  492167,  1893212, 1036194, 757617,  814720,  257495,  3904091,
    846110,  2522385,  978207,  1228020,  851478,  250856,  984562,  1448693, 742778,  345261,  781929,  288071,
    2830022, 512587,   569972,  1270185,  364654,  715325,  592147,  1601146, 677872,  1297982, 1387863, 930569,
    2227027, 1072604,  796290,  213823,   997833,  471290,  985365,  165065,  421615,  1032919, 571366,  3735687,
    1791070, 1369027,  906827,  957223,   1209730, 1097858, 847921,  924030,  1221456, 1391043, 577367,  359826,
    659334,  992911,   1116355, 1155725,  390374,  1169391, 523348,  1017245, 722669,  796564,  940980,  865739,
    1124888, 442624,   750586,  2446371,  848397,  84258,   1945643, 306922,  233392,  529366,  1121889, 1804385,
    441544,  421734,   1135525, 460320,   235509,  674565,  472222,  995386,  423665,  2020881, 1480615, 1454026,
    586557,  1803759,  2006659, 799416,   622789,  5949815, 292170,  3634506, 905019,  1681598, 293438,  595530,
    997954,  229261,   332974,  1052476,  926523,  811808,  2390221, 263248,  1926098, 585011,  399875,  971470,
    1884851, 1642966,  70177,   1461303,  2836691, 513703,  269485,  363610,  848737,  474811,  1709917, 674171,
    491301,  2867724,  485383,  2890436,  3137778, 744055,  748921,  2707269, 1161794, 402486,  1213454, 2497637,
    1285103, 1195176,  1107961, 683611,   587191,  376645,  1547573, 1925675, 638781,  138811,  161121,  669296,
    1392853, 2434406,  895480,  462050,   1175851, 632886,  1094643, 1763699, 931189,  2536063, 489931,  1352295,
    617516,  365696,   1782418, 484128,   559988,  764979,  1609546, 564545,  1050296, 901765,  470980,  819590,
    1002305, 880889,   1314436, 1633857,  1697485, 1259911, 426400,  1548806, 599540,  999633,  256078,  551673,
    2306305, 1402239,  254607,  2369909,  506194,  519954,  405835,  264496,  1200435, 436230,  730754,  1496471,
    725322,  1485734,  1068583, 710865,   404980,  577622,  3027944, 387524,  164504,  613974,  790169,  811317,
    1997823, 294203,   116767,  5851644,  407508,  699643,  657880,  1557250, 442400,  1306863, 4029301, 141190,
    3263263, 1292526,  941471,  2067369,  1442873, 602055,  333221,  771518,  2142912, 6531012, 74348,   233584,
    1398315, 870802,   275907,  1122655,  447877,  599408,  325764,  670875,  498254,  3287614, 2170622, 2183648,
    1515680, 256486,   420338,  522840,   141534,  420504,  1017246, 851399,  864360,  384818,  2111951, 502605,
    674198,  3578067,  2473626, 2720727,  521111,  887153,  962225,  1368229, 223281,  2165793, 3061342, 545161,
    728102,  1976764,  617819,  1069501,  1163840, 394197,  939442,  676631,  5992854, 933659,  684958,  1079920,
    1433175, 1000615,  633722,  545621,   743648,  1014701, 712649,  974725,  1707476, 1352275, 732599,  229100,
    928612,  1415489,  1249036, 1209045,  1576320, 450001,  300117,  749076,  1134652, 587393,  126789,  1924375,
    3677326, 324823,   1103144, 10467736, 1010552, 1286346, 2238289, 1092057, 1601971, 9156924, 1569061, 536386,
    3812173, 697158,   1650848, 2339159,  1188798, 673682,  877289,  1285139, 1047783, 9764114, 1049312, 914894,
    1010058, 1284907,  336126,  350975,   2370643, 240064,  398615,  887039,  1326686, 792877,  438587,  1369850,
    767679,  668738,   1783395, 709435,   153618,  1806364, 1195651, 1798814, 479664,  624009,  890486,  852616,
    1980711, 420838,   1415800, 2168869};

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
