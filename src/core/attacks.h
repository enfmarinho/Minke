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

#pragma once

#include <cassert>
#include <cstdint>

#include "core/bitboard.h"
#include "core/types.h"

extern Bitboard bishop_masks[64];
extern Bitboard rook_masks[64];

extern int bishop_shifts[64];
extern int rook_shifts[64];

extern uint64_t bishop_magic_numbers[64];
extern uint64_t rook_magic_numbers[64];

extern Bitboard pawn_attacks[2][64];
extern Bitboard knight_attacks[64];
extern Bitboard king_attacks[64];
extern Bitboard bishop_attacks[64][512];
extern Bitboard rook_attacks[64][4096];

extern Bitboard between_squares[64][64];
extern Bitboard passing_rays[64][64];

void init_magic_table(PieceType piece_type);

Bitboard generate_bishop_mask(Square sq);
Bitboard generate_rook_mask(Square sq);

Bitboard generate_pawn_attacks(Square sq, Color color);
Bitboard generate_knight_attacks(Square sq);
Bitboard generate_bishop_attacks(Square sq, const Bitboard& blockers);
Bitboard generate_rook_attacks(Square sq, const Bitboard& blockers);
Bitboard generate_king_attacks(Square sq);

inline int get_attack_index(Bitboard blockers, uint64_t magic, int shift) { return (blockers.raw() * magic) >> shift; }

inline Bitboard get_bishop_attacks(const Square& sq, const Bitboard& occupancy) {
    return bishop_attacks[sq]
                         [get_attack_index(occupancy & bishop_masks[sq], bishop_magic_numbers[sq], bishop_shifts[sq])];
}

inline Bitboard get_rook_attacks(const Square& sq, const Bitboard& occupancy) {
    return rook_attacks[sq][get_attack_index(occupancy & rook_masks[sq], rook_magic_numbers[sq], rook_shifts[sq])];
}

inline Bitboard get_queen_attacks(const Square& sq, const Bitboard& occupancy) {
    return get_rook_attacks(sq, occupancy) | get_bishop_attacks(sq, occupancy);
}

inline Bitboard get_piece_attacks(const Square& sq, const Bitboard& occupancy, PieceType piece_type) {
    assert(piece_type >= KNIGHT && piece_type <= KING);

    switch (piece_type) {
        case KNIGHT:
            return knight_attacks[sq];
        case BISHOP:
            return get_bishop_attacks(sq, occupancy);
        case ROOK:
            return get_rook_attacks(sq, occupancy);
        case QUEEN:
            return get_queen_attacks(sq, occupancy);
        case KING:
            return king_attacks[sq];
        default:
            __builtin_unreachable();
    }
}
