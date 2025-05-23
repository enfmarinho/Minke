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

int LMR_TABLE[64][64];
int LMP_TABLE[2][LMP_DEPTH];
HashKeys hash_keys;
Network network;
TranspositionTable TT;

void init_all() {
    TT.resize(EngineOptions::HASH_DEFAULT);
    init_search_params();
    init_network_params();
    init_hash_keys();
    init_magic_attack_tables();
}

void init_search_params() {
    constexpr double LMR_BASE = 1;
    constexpr double LMR_DIVISOR = 2.2;
    for (int depth = 1; depth < 64; ++depth) {
        for (int move_counter = 1; move_counter < 64; ++move_counter) {
            LMR_TABLE[depth][move_counter] = LMR_BASE + std::log(depth) * std::log(move_counter) / LMR_DIVISOR;
            assert(LMR_TABLE[depth][move_counter] > 0);
        }
    }

    constexpr double LMP_BASE = 1;
    constexpr double LMP_MULTIPLIER = 2.2;
    for (int depth = 1; depth < LMP_DEPTH; ++depth) {
        LMP_TABLE[0][depth] = LMP_BASE + LMP_MULTIPLIER * depth * depth;
        LMP_TABLE[1][depth] = 2 * LMP_BASE + 2 * LMP_MULTIPLIER * depth * depth; // Improving
    }
    LMR_TABLE[0][0] = 0;
}

void init_network_params() {
    const int16_t *pointer = reinterpret_cast<const int16_t *>(gNetParametersData);

    for (int i = 0; i < INPUT_LAYER_SIZE; ++i)
        for (int j = 0; j < HIDDEN_LAYER_SIZE; ++j)
            network.hidden_weights[i * HIDDEN_LAYER_SIZE + j] = *(pointer++);

    for (int i = 0; i < HIDDEN_LAYER_SIZE; ++i)
        network.hidden_bias[i] = *(pointer++);

    for (int i = 0; i < HIDDEN_LAYER_SIZE * 2; ++i)
        network.output_weights[i] = *(pointer++);

    network.output_bias = *(pointer++);
}

void init_hash_keys() {
    PRNG prng(1070372);
    for (int piece = WHITE_PAWN; piece <= BLACK_KING; ++piece) {
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
    init_magic_table(BISHOP);
    init_magic_table(ROOK);

    // Initialize non-slider attack tables
    for (int sqi = a1; sqi <= h8; ++sqi) {
        Square sq = static_cast<Square>(sqi);

        pawn_attacks[WHITE][sq] = generate_pawn_attacks(sq, WHITE);
        pawn_attacks[BLACK][sq] = generate_pawn_attacks(sq, BLACK);
        knight_attacks[sq] = generate_knight_attacks(sq);
        king_attacks[sq] = generate_king_attacks(sq);
    }
}
