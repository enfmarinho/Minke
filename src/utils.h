/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef UTILS_H
#define UTILS_H

#include "types.h"

// Returns the number of '1' bit in bitboard, just like popcount
inline int count_bits(const Bitboard &bitboard);

// Returns the least significant bit in a non-zero bitboard and sets it to 0.
inline Square poplsb(Bitboard &bitboard);

// Returns the least significant bit in a non-zero bitboard.
inline Square lsb(Bitboard bitboard);

// Returns the most significant bit in a non-zero bitboard.
inline Square msb(Bitboard bitboard);

// Returns the rank of "sq"
inline int rank(Square sq);

// Returns the file of "sq"
inline int file(Square sq);

#endif // #ifndef UTILS_H
