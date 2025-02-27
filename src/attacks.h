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


Bitboard generate_bishop_mask(Square sq);
Bitboard generate_rook_mask(Square sq);

Bitboard generate_pawn_attack(Square sq, Color color);
Bitboard generate_knight_attack(Square sq);
Bitboard generate_bishop_attack(Square sq, const Bitboard& blockers);
Bitboard generate_rook_attack(Square sq, const Bitboard& blockers);
Bitboard generate_king_attack(Square sq);
#endif // #ifndef ATTACKS_H
