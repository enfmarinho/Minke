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

const Bitboard not_a_file = ~FileMasks[0];
const Bitboard not_ab_file = ~(FileMasks[0] | FileMasks[1]);
const Bitboard not_h_file = ~FileMasks[7];
const Bitboard not_hg_file = ~(FileMasks[6] | FileMasks[7]);

const Bitboard not_1_rank = ~RankMasks[0];
const Bitboard not_1_2_rank = ~(RankMasks[0] | RankMasks[1]);
const Bitboard not_8_rank = ~RankMasks[7];
const Bitboard not_7_8_rank = ~(RankMasks[7] | RankMasks[6]);

extern Bitboard BishopMasks[64];
extern Bitboard RookMasks[64];

extern Bitboard BishopShifts[64];
extern Bitboard RookShifts[64];

extern Bitboard BishopMagicNumbers[64];
extern Bitboard RookMagicNumbers[64];

extern Bitboard PawnAttacks[2][64];
extern Bitboard KnightAttacks[64];
extern Bitboard KingAttacks[64];
extern Bitboard BishopAttacks[64][512];
extern Bitboard RookAttacks[64][4096];

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
    return BishopAttacks[sq][get_attack_index(occupancy & BishopMasks[sq], BishopMagicNumbers[sq], BishopShifts[sq])];
}

inline Bitboard get_rook_attacks(const Square& sq, const Bitboard& occupancy) {
    return RookAttacks[sq][get_attack_index(occupancy & RookMasks[sq], RookMagicNumbers[sq], RookShifts[sq])];
}

inline Bitboard get_queen_attacks(const Square& sq, const Bitboard& occupancy) {
    return get_rook_attacks(sq, occupancy) | get_bishop_attacks(sq, occupancy);
}

inline Bitboard get_piece_attacks(const Square& sq, const Bitboard& occupancy, PieceType piece_type) {
    assert(piece_type >= Knight && piece_type <= King);

    switch (piece_type) {
        case Knight:
            return KnightAttacks[sq];
        case Bishop:
            return get_bishop_attacks(sq, occupancy);
        case Rook:
            return get_rook_attacks(sq, occupancy);
        case Queen:
            return get_queen_attacks(sq, occupancy);
        case King:
            return KingAttacks[sq];
        default:
            __builtin_unreachable();
    }
}

#endif // #ifndef ATTACKS_H
