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
    44422,   59555,   54666,   54126,   58459,   50127,   58071,   93638,   57543,   135533,  67709,   81953,   77818,
    87454,   86165,   102920,  77402,   118715,  93047,   94883,   117561,  100084,  132572,  121331,  142099,  113535,
    126377,  105203,  118160,  136711,  142725,  115488,  125058,  125513,  129411,  110689,  106706,  142425,  139370,
    147031,  151600,  130453,  118266,  150442,  167944,  155520,  148420,  159297,  218117,  133820,  142300,  177576,
    155866,  149483,  150781,  176326,  175189,  126416,  216597,  167255,  210449,  192277,  151133,  150741,  217838,
    124343,  128386,  158540,  173758,  169908,  156731,  221507,  187112,  174429,  188318,  174792,  192234,  160856,
    177045,  166883,  157384,  171094,  178356,  179042,  188674,  206186,  233379,  192832,  238951,  202550,  135863,
    241252,  204347,  153303,  230650,  216593,  199081,  189223,  229002,  247744,  180855,  219036,  219960,  170880,
    232899,  192625,  212401,  251625,  247969,  226960,  234474,  213659,  246068,  219800,  320014,  222393,  232782,
    233693,  313386,  300949,  273895,  286919,  220643,  250281,  272120,  277931,  254330,  310295,  266435,  270172,
    306137,  279766,  222454,  241248,  213076,  312208,  238006,  266847,  239650,  219873,  255474,  276753,  305214,
    301925,  234419,  287755,  292289,  386820,  352468,  277655,  280533,  263879,  293888,  279937,  339207,  294533,
    266998,  311022,  241946,  322661,  302278,  310057,  304933,  318284,  389308,  272951,  279002,  324839,  302047,
    289611,  327553,  313484,  304238,  436086,  320276,  267804,  363230,  295284,  315549,  305359,  296500,  316571,
    366509,  367213,  288306,  290767,  389442,  324245,  365196,  405408,  357850,  380390,  400863,  346804,  356089,
    435855,  416667,  352728,  445571,  355036,  477842,  372112,  367815,  386549,  332698,  385902,  372325,  406716,
    298251,  336246,  383924,  294399,  437207,  376312,  439919,  446491,  400998,  324842,  395791,  372352,  454775,
    431335,  368025,  327072,  337154,  336779,  422938,  364756,  391119,  393459,  469995,  380321,  379620,  340604,
    380323,  493285,  400961,  350952,  379241,  437450,  406376,  353006,  349014,  453740,  390583,  429427,  469058,
    462335,  438276,  416467,  469891,  330520,  444973,  529597,  443848,  410660,  321428,  366513,  485898,  419824,
    553759,  428995,  473455,  451504,  420562,  440307,  491782,  415682,  490748,  414484,  598365,  454896,  452488,
    554372,  397618,  516655,  444033,  556227,  431937,  454658,  446055,  508939,  429028,  435933,  487645,  531911,
    432178,  518157,  467673,  432797,  506534,  454708,  602299,  475150,  541759,  562657,  569516,  611996,  559679,
    528228,  500274,  609613,  545889,  542114,  545802,  542952,  444424,  570141,  463005,  465762,  582340,  494774,
    547020,  523332,  557761,  546405,  565065,  594545,  489832,  474426,  480231,  620476,  544111,  475758,  546581,
    596677,  780633,  551898,  538893,  667967,  452961,  685890,  507901,  624991,  595576,  624305,  614257,  642258,
    673944,  730661,  619460,  545435,  559166,  626491,  686348,  640192,  519720,  558704,  598050,  693275,  632927,
    582052,  665159,  539517,  677380,  718621,  556454,  531977,  523008,  501822,  639660,  665053,  632839,  765668,
    645813,  593249,  621947,  618155,  723807,  730502,  647775,  727918,  562011,  565288,  728103,  614709,  710906,
    656901,  525929,  590139,  805443,  744202,  617066,  649406,  887691,  722507,  623744,  782868,  606237,  713666,
    609260,  811935,  586546,  619607,  772456,  639224,  750631,  593692,  749861,  654187,  772748,  903307,  717607,
    693304,  759815,  788466,  719761,  766736,  857585,  745791,  691512,  778861,  853617,  842731,  735812,  715104,
    704013,  723747,  912599,  768865,  682864,  860036,  904806,  944048,  893253,  788041,  951487,  722934,  914691,
    781074,  962043,  800901,  789123,  1038099, 792820,  1016194, 952251,  976083,  817069,  836258,  867759,  964056,
    731655,  1023752, 880808,  1004541, 798915,  1158351, 960734,  827987,  1136236, 992807,  883709,  929157,  857960,
    916187,  1005043, 930120,  964099,  1114101, 814283,  978584,  1070665, 1120710, 1001211, 915941,  1131866, 1058058,
    992278,  1117082, 949606,  1281000, 1061411, 981230,  1219554, 1001045, 911220,  872430,  1081938, 1234904, 941561,
    989972,  1297709, 1094248, 1293312, 1233120, 1253012, 1097754, 1145857, 1205873, 1084023, 1137043, 1107155, 1215619,
    958272,  1434871, 1153553, 1324082, 969278,  1255980, 1200436, 1208805, 1093781, 1199122, 1285630, 1201483, 1336856,
    1191729, 1349630, 1401434, 1339901, 1198633, 1345835, 1447195, 1359993, 1307731, 1409133, 1252100, 1335448, 1393364,
    1387739, 1416351, 1329189, 1358785, 1216575, 1332846, 1218371, 1411094, 1181308, 1305593, 1279907, 1426612, 1455271,
    1291598, 1228744, 1404127, 1372228, 1499466, 1689655, 1553968, 1323777, 1480762, 1762537, 1378771, 1512560, 1579942,
    1818036, 1630108, 1524189, 1553632, 1725015, 1832981, 1783825, 1630787, 1622840, 1771137, 1635459, 1800265, 1752272,
    1917893, 1722703, 1770907, 1683597, 1794272, 1759626, 2048786, 1658320, 1933474, 1800515, 1837355, 2066598, 2112785,
    2052492, 2207279, 2154086, 2086537, 1851390, 2138074, 2306060, 2080801, 2163965, 2129557, 2049461, 1982918, 2107060,
    2544510, 2225260, 2342436, 2482410, 2369201, 2302094, 2483070, 2569329, 2660974, 2354715, 2493630, 2428053, 2865333,
    3013818, 2726615, 2845929, 3057279, 2950325, 3023775, 2777413, 2900270, 2882565, 3030834, 3219224, 3387113, 3197062,
    3178985, 3601217, 3502315, 3774891, 3547831, 3643731, 3558052, 3818795, 3733440, 3742318, 3638884, 4021549, 4161582,
    4330772, 4261480, 4393473, 4577191, 4579844, 4658502, 4725386, 4771754, 5026760, 5094611, 5312968, 5424531, 5524502,
    5561950, 5555335, 5733720};

std::array<uint16_t, L1_SIZE> build_permutation() {
    std::array<uint16_t, PAIR_COUNT> order;
    std::iota(order.begin(), order.end(), 0);

    std::stable_sort(order.begin(), order.end(),
                     [](uint16_t a, uint16_t b) { return activation_counts[a] < activation_counts[b]; });

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
