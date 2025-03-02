/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "move.h"
#include "position.h"

enum MoveGenType {
    Quiet = 1,
    Noisy = Quiet << 1,
    GenAll = Quiet | Noisy
};

ScoredMove* gen_moves(ScoredMove* moves, const Position& position, const MoveGenType type);

#endif // #ifndef MOVEGEN_H
