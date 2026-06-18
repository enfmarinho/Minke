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

#ifndef SEARCH_H
#define SEARCH_H

#include <cstddef>

#include "correction.h"
#include "history.h"
#include "move.h"
#include "search_limiter.h"
#include "thread.h"
#include "types.h"

constexpr int LMP_DEPTH = 32;
extern int LMP_TABLE[2][LMP_DEPTH];
extern int LMR_TABLE[64][64];

ScoreType normalize_score(ScoreType score);

ScoreType iterative_deepening(ThreadData &td);
ScoreType aspiration(const CounterType &depth, const ScoreType prev_score, ThreadData &td);
ScoreType negamax(ScoreType alpha, ScoreType beta, CounterType depth, const bool cutnode, ThreadData &td);
ScoreType quiescence(ScoreType alpha, ScoreType beta, ThreadData &td);
bool SEE(Position &position, const Move &move, int threshold);

#endif // #ifndef SEARCH_H
