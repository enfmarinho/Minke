/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef ATTACKS_H
#define ATTACKS_H

#include "types.h"


Bitboard generate_bishop_mask(Square sq);
Bitboard generate_rook_mask(Square sq);
#endif // #ifndef ATTACKS_H
