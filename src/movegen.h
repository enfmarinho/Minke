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
