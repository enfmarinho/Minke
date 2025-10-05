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
#include "tune.h"
#include "types.h"

#ifndef EVALFILE
#define EVALFILE "../src/minke.bin"
#endif // !EVALFILE

INCBIN(NetParameters, EVALFILE);

int LMR_TABLE[64][64];
// int LMP_TABLE[2][LMP_DEPTH];
HashKeys hash_keys;
Network network;
Bitboard between_squares[64][64];

void init_all() {
    init_search_params();
    init_network_params();
    init_hash_keys();
    init_magic_attack_tables();
    init_between_squares();
}

void init_search_params() {
    for (int depth = 1; depth < 64; ++depth) {
        for (int move_counter = 1; move_counter < 64; ++move_counter) {
            LMR_TABLE[depth][move_counter] =
                (lmr_base() / 100.0) + std::log(depth) * std::log(move_counter) / (lmr_divisor() / 100.0);
            assert(LMR_TABLE[depth][move_counter] > 0);
        }
    }
    LMR_TABLE[0][0] = 0;

    // constexpr double LMP_BASE = 1;
    // constexpr double LMP_MULTIPLIER = 2.2;
    // for (int depth = 1; depth < LMP_DEPTH; ++depth) {
    //     LMP_TABLE[0][depth] = LMP_BASE + LMP_MULTIPLIER * depth * depth;
    //     LMP_TABLE[1][depth] = 2 * LMP_BASE + 2 * LMP_MULTIPLIER * depth * depth; // Improving
    // }
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

void init_between_squares() {
    for (int sqi1 = a1; sqi1 <= h8; ++sqi1) {
        for (int sqi2 = a1; sqi2 <= h8; ++sqi2) {
            Square sq1 = static_cast<Square>(sqi1);
            Square sq2 = static_cast<Square>(sqi2);
            Bitboard occ1 = (1ULL << sq1);
            Bitboard occ2 = (1ULL << sq2);
            if (get_bishop_attacks(sq1, 0) & occ2) {
                between_squares[sq1][sq2] = get_bishop_attacks(sq1, occ2) & get_bishop_attacks(sq2, occ1);
            } else if (get_rook_attacks(sq1, 0) & occ2) {
                between_squares[sq1][sq2] = get_rook_attacks(sq1, occ2) & get_rook_attacks(sq2, occ1);
            }
        }
    }
}
