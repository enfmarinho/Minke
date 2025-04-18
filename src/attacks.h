/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef ATTACKS_H
#define ATTACKS_H

#include <cassert>

#include "types.h"

const Bitboard NOT_A_FILE = ~FILE_MASKS[0];
const Bitboard NOT_AB_FILE = ~(FILE_MASKS[0] | FILE_MASKS[1]);
const Bitboard NOT_H_FILE = ~FILE_MASKS[7];
const Bitboard NOT_HG_FILE = ~(FILE_MASKS[6] | FILE_MASKS[7]);

const Bitboard NOT_1_RANK = ~RANK_MASKS[0];
const Bitboard NOT_1_2_RANK = ~(RANK_MASKS[0] | RANK_MASKS[1]);
const Bitboard NOT_8_RANK = ~RANK_MASKS[7];
const Bitboard NOT_7_8_RANK = ~(RANK_MASKS[7] | RANK_MASKS[6]);

extern Bitboard bishop_masks[64];
extern Bitboard rook_masks[64];

extern Bitboard bishop_shifts[64];
extern Bitboard rook_shifts[64];

extern Bitboard bishop_magic_numbers[64];
extern Bitboard rook_magic_numbers[64];

extern Bitboard pawn_attacks[2][64];
extern Bitboard knight_attacks[64];
extern Bitboard king_attacks[64];
extern Bitboard bishop_attacks[64][512];
extern Bitboard rook_attacks[64][4096];

void init_magic_table(PieceType piece_type);

Bitboard generate_bishop_mask(Square sq);
Bitboard generate_rook_mask(Square sq);

Bitboard generate_pawn_attacks(Square sq, Color color);
Bitboard generate_knight_attacks(Square sq);
Bitboard generate_bishop_attacks(Square sq, const Bitboard& blockers);
Bitboard generate_rook_attacks(Square sq, const Bitboard& blockers);
Bitboard generate_king_attacks(Square sq);

inline int get_attack_index(Bitboard blockers, Bitboard magic, int shift) { return (blockers * magic) >> shift; }

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

#endif // #ifndef ATTACKS_H
