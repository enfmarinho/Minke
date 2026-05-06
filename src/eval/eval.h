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

#ifndef EVAL_H
#define EVAL_H

#include <algorithm>

#include "../position.h"
#include "../tune.h"
#include "../types.h"

static inline int apply_material_scaling(const Position& pos, ScoreType raw_eval) {
    const int material_scale = material_scaling_base()                                    //
                               + pos.get_material_count(PAWN) * pawn_scaling_factor()     //
                               + pos.get_material_count(KNIGHT) * knight_scaling_factor() //
                               + pos.get_material_count(BISHOP) * bishop_scaling_factor() //
                               + pos.get_material_count(ROOK) * rook_scaling_factor()     //
                               + pos.get_material_count(QUEEN) * queen_scaling_factor();

    return raw_eval * material_scale / 32768;
}

inline ScoreType adjust_eval(const Position& pos, const int correction, const ScoreType raw_eval) {
    int adjusted_eval = apply_material_scaling(pos, raw_eval) + correction;

    return std::clamp(adjusted_eval, -MATE_FOUND + 1, MATE_FOUND - 1);
}

#endif // !EVAL_H
