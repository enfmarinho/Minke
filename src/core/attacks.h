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

namespace Attacks {

alignas(64) extern Bitboard bishop_mask_table[64];
alignas(64) extern Bitboard rook_mask_table[64];

alignas(64) extern int bishop_shifts[64];
alignas(64) extern int rook_shifts[64];

alignas(64) extern uint64_t bishop_magic_numbers[64];
alignas(64) extern uint64_t rook_magic_numbers[64];

alignas(64) extern Bitboard pawn_attack_table[2][64];
alignas(64) extern Bitboard knight_attack_table[64];
alignas(64) extern Bitboard king_attack_table[64];
alignas(64) extern Bitboard bishop_attack_table[64][512];
alignas(64) extern Bitboard rook_attack_table[64][4096];

// indexed as [from_sq][to_sq]
alignas(64) extern Bitboard inbetween_mask_table[64][64];
alignas(64) extern Bitboard passing_mask_table[64][64];

// indexed as [sq]
alignas(64) extern Bitboard diagonal_mask_table[64];
alignas(64) extern Bitboard antidiagonal_mask_table[64];

void init();

inline int attack_index(Bitboard blockers, uint64_t magic, int shift) { return (blockers.raw() * magic) >> shift; }

inline Bitboard inbetween_mask(Square sq1, Square sq2) { return inbetween_mask_table[sq1][sq2]; }

inline Bitboard passing_mask(Square sq1, Square sq2) { return passing_mask_table[sq1][sq2]; }

inline Bitboard diagonal_mask(Square sq) { return diagonal_mask_table[sq]; }

inline Bitboard antidiagonal_mask(Square sq) { return antidiagonal_mask_table[sq]; }

inline Bitboard pawn_attack(Color side, Square sq) { return pawn_attack_table[side][sq]; }

inline Bitboard knight_attack(Square sq) { return knight_attack_table[sq]; }

inline Bitboard bishop_attack(Square sq, const Bitboard& occupancy) {
    return bishop_attack_table[sq][attack_index(occupancy & bishop_mask_table[sq], bishop_magic_numbers[sq],
                                                bishop_shifts[sq])];
}

inline Bitboard rook_attack(Square sq, const Bitboard& occupancy) {
    return rook_attack_table[sq]
                            [attack_index(occupancy & rook_mask_table[sq], rook_magic_numbers[sq], rook_shifts[sq])];
}

inline Bitboard queen_attack(Square sq, const Bitboard& occupancy) {
    return rook_attack(sq, occupancy) | bishop_attack(sq, occupancy);
}

inline Bitboard king_attack(Square sq) { return king_attack_table[sq]; }

inline Bitboard piece_attack(PieceType piece_type, Square sq, const Bitboard& occupancy = {}) {
    assert(piece_type >= KNIGHT && piece_type <= KING);

    switch (piece_type) {
        case KNIGHT:
            return knight_attack_table[sq];
        case BISHOP:
            return bishop_attack(sq, occupancy);
        case ROOK:
            return rook_attack(sq, occupancy);
        case QUEEN:
            return queen_attack(sq, occupancy);
        case KING:
            return king_attack_table[sq];
        default:
            __builtin_unreachable();
    }
}

} // namespace Attacks
