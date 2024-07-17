/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef HASH_H
#define HASH_H

#include "game_elements.h"
#include "position.h"

namespace zobrist {
HashType hash(const Position &position);
HashType rehash(const Position &position, const PastMove &last_move);
int piece_start_index(const Square &piece_square);
IndexType placement_index(const PiecePlacement &piece_placement);
IndexType placement_index(const IndexType &file, const IndexType &rank);

constexpr static int WhiteKingSideCastlingRightsIndex = 768;
constexpr static int WhiteQueenSideCastlingRightsIndex = 769;
constexpr static int BlackKingSideCastlingRightsIndex = 770;
constexpr static int BlackQueenSideCastlingRightsIndex = 771;
constexpr static int EnPassantStarterIndex = 772;
constexpr static int BlackTurnIndex = 780;
}; // namespace zobrist

#endif // #ifndef HASH_H
