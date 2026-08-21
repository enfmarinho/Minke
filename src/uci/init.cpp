/*
 *  Minke is a UCI chess engine
 *  Copyright (C) 2026 Eduardo Marinho <eduardomarinho@pm.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "uci/init.h"

#include <cmath>

#include "core/attacks.h"
#include "core/types.h"
#include "eval/nnue/arch.h"
#include "search/cuckoo.h"
#include "search/search.h"
#include "uci/tune.h"
#include "utils/incbin.h"

INCBIN(NetParameters, EVALFILE);

int LMR_TABLE[64][64];
int LMP_TABLE[2][LMP_DEPTH];
Network network;

Bitboard inbetween_masks[64][64];
Bitboard passing_masks[64][64];
Bitboard diagonal_masks[64];
Bitboard antidiagonal_masks[64];

void init_all() {
    init_search_params();
    init_network_params();
    init_magic_attack_tables();
    init_inbetween_masks();
    init_passing_masks();
    init_diagonal_antidiagonal_masks();
    Cuckoo::init();
}

void init_search_params() {
    for (int depth = 1; depth < 64; ++depth) {
        for (int move_counter = 1; move_counter < 64; ++move_counter) {
            LMR_TABLE[depth][move_counter] = lmr_base() + lmr_scale() * std::log(depth) * std::log(move_counter);
        }
    }
    LMR_TABLE[0][0] = 0;

    for (int depth = 0; depth < LMP_DEPTH; ++depth) {
        LMP_TABLE[0][depth] = (lmp_base() / 100.0) + (lmp_scale() / 100.0) * depth * depth;
        LMP_TABLE[1][depth] = (lmp_improving_base() / 100.0) + (lmp_improving_scale() / 100.0) * depth * depth;
    }
}

void init_network_params() { network = *reinterpret_cast<const Network *>(&gNetParametersData); }

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

void init_inbetween_masks() {
    for (int sqi1 = a1; sqi1 <= h8; ++sqi1) {
        for (int sqi2 = a1; sqi2 <= h8; ++sqi2) {
            Square sq1 = static_cast<Square>(sqi1);
            Square sq2 = static_cast<Square>(sqi2);
            Bitboard occ1(sq1);
            Bitboard occ2(sq2);
            if (get_bishop_attacks(sq1, 0) & occ2) {
                inbetween_masks[sq1][sq2] = get_bishop_attacks(sq1, occ2) & get_bishop_attacks(sq2, occ1);
            } else if (get_rook_attacks(sq1, 0) & occ2) {
                inbetween_masks[sq1][sq2] = get_rook_attacks(sq1, occ2) & get_rook_attacks(sq2, occ1);
            }
        }
    }
}

void init_passing_masks() {
    for (int src = a1; src <= h8; ++src) {
        Square src_sq = static_cast<Square>(src);
        Bitboard src_mask(src_sq);

        Bitboard rook_attack = get_rook_attacks(src_sq, 0);
        Bitboard bishop_attack = get_bishop_attacks(src_sq, 0);
        for (int to = a1; to <= h8; ++to) {
            if (src == to)
                continue;

            Square to_sq = static_cast<Square>(to);
            Bitboard to_mask(to_sq);

            if (rook_attack & to_mask) {
                passing_masks[src][to] = rook_attack & (get_rook_attacks(to_sq, src_mask) | to_mask);
            } else if (bishop_attack & to_mask) {
                passing_masks[src][to] = bishop_attack & (get_bishop_attacks(to_sq, src_mask) | to_mask);
            }
        }
    }
}

void init_diagonal_antidiagonal_masks() {
    for (int sqi = a1; sqi <= h8; ++sqi) {
        Square sq = static_cast<Square>(sqi);

        Bitboard diag(sq); // south_west to north_east
        for (Bitboard mask = diag.shift_north_east(); mask; mask = mask.shift_north_east())
            diag |= mask;
        for (Bitboard mask = diag.shift_south_west(); mask; mask = mask.shift_south_west())
            diag |= mask;
        diagonal_masks[sq] = diag;

        Bitboard antidiag(sq); // north_west to south_east
        for (Bitboard mask = antidiag.shift_north_west(); mask; mask = mask.shift_north_west())
            antidiag |= mask;
        for (Bitboard mask = antidiag.shift_south_east(); mask; mask = mask.shift_south_east())
            antidiag |= mask;
        antidiagonal_masks[sq] = antidiag;
    }
}
