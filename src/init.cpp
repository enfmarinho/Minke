/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "init.h"

#include <cmath>

#include "attacks.h"
#include "hash.h"
#include "incbin.h"
#include "nnue.h"
#include "search.h"
#include "tt.h"
#include "types.h"
#include "uci.h"

INCBIN(NetParameters, "../src/minke.bin");

int LMRTable[64][64];
int LMPTable[2][LMPDepth];
HashKeys hash_keys;
Network network;
TranspositionTable TT;

void init_all() {
    TT.resize(EngineOptions::hash_default);
    init_search_params();
    init_network_params();
    init_hash_keys();
    init_magic_attack_tables();
}

void init_search_params() {
    constexpr double lmr_base = 1;
    constexpr double lmr_divisor = 2.2;
    for (int depth = 1; depth < 64; ++depth) {
        for (int move_counter = 1; move_counter < 64; ++move_counter) {
            LMRTable[depth][move_counter] = lmr_base + std::log(depth) * std::log(move_counter) / lmr_divisor;
            assert(LMRTable[depth][move_counter] > 0);
        }
    }

    constexpr double lmp_base = 1;
    constexpr double lmp_multiplier = 2.2;
    for (int depth = 1; depth < LMPDepth; ++depth) {
        LMPTable[0][depth] = lmp_base + lmp_multiplier * depth * depth;
        LMPTable[1][depth] = 2 * lmp_base + 2 * lmp_multiplier * depth * depth; // Improving
    }
    LMRTable[0][0] = 0;
}

void init_network_params() {
    const int16_t *pointer = reinterpret_cast<const int16_t *>(gNetParametersData);

    for (int i = 0; i < InputLayerSize; ++i)
        for (int j = 0; j < HiddenLayerSize; ++j)
            network.hidden_weights[i * HiddenLayerSize + j] = *(pointer++);

    for (int i = 0; i < HiddenLayerSize; ++i)
        network.hidden_bias[i] = *(pointer++);

    for (int i = 0; i < HiddenLayerSize * 2; ++i)
        network.output_weights[i] = *(pointer++);

    network.output_bias = *(pointer++);
}

void init_hash_keys() {
    PRNG prng(1070372);
    for (int piece = WhitePawn; piece <= BlackKing; ++piece) {
        for (int sqi = a1; sqi <= h8; ++sqi) {
            hash_keys.pieces[piece][sqi] = prng.rand<HashType>();
        }
    }

    for (int castle = 0; castle < 16; ++castle) {
        hash_keys.castle[castle] = prng.rand<HashType>();
    }

    for (int rank = 0; rank < 8; ++rank) {
        hash_keys.en_passant[rank] = prng.rand<HashType>();
    }

    hash_keys.side = prng.rand<HashType>();
}

void init_magic_attack_tables() {
    // This initializes all attacks, masks, magics and shifts for Bishop and Rook as a side effect
    init_magic_table(Bishop);
    init_magic_table(Rook);

    // Initialize non-slider attack tables
    for (int sqi = a1; sqi <= h8; ++sqi) {
        Square sq = static_cast<Square>(sqi);

        PawnAttacks[White][sq] = generate_pawn_attacks(sq, White);
        PawnAttacks[Black][sq] = generate_pawn_attacks(sq, Black);
        KnightAttacks[sq] = generate_knight_attacks(sq);
        KingAttacks[sq] = generate_king_attacks(sq);
    }
}
