/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef ATTACKS_H
#define ATTACKS_H

#include "types.h"

const Bitboard not_a_file = 18374403900871474942ULL;
const Bitboard not_ab_file = 18229723555195321596ULL;
const Bitboard not_h_file = 9187201950435737471ULL;
const Bitboard not_hg_file = 4557430888798830399ULL;

const Bitboard not_1_rank = 18446744073709551360ULL;
const Bitboard not_1_2_rank = 18446744073709486080ULL;
const Bitboard not_8_rank = 72057594037927935ULL;
const Bitboard not_7_8_rank = 281474976710655ULL;

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

#endif // #ifndef ATTACKS_H
