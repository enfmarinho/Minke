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
    QUIET = 1,
    NOISY = QUIET << 1,
    GEN_ALL = QUIET | NOISY
};

ScoredMove* gen_moves(ScoredMove* moves, const Position& position, const MoveGenType type);

#endif // #ifndef MOVEGEN_H
