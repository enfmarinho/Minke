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

#include "eval/eval.h"

#include <algorithm>

#include "core/position.h"
#include "core/types.h"
#include "eval/nnue/accumulator/state.h"
#include "eval/nnue/inference.h"
#include "search/search.h"
#include "uci/tune.h"

namespace {

inline int apply_material_scaling(const Position& pos, ScoreType raw_eval) {
    const int material_scale = material_scaling_base()                             //
                               + pos.piece_count(PAWN) * pawn_scaling_factor()     //
                               + pos.piece_count(KNIGHT) * knight_scaling_factor() //
                               + pos.piece_count(BISHOP) * bishop_scaling_factor() //
                               + pos.piece_count(ROOK) * rook_scaling_factor()     //
                               + pos.piece_count(QUEEN) * queen_scaling_factor();

    return raw_eval * material_scale / 32768;
}

} // namespace

namespace eval {

ScoreType evaluate(const Position& pos, const accumulator::State& acc_state) {
    const int bucket = (pos.piece_count() - 2) / BUCKET_SIZE;

    return propagate(acc_state.pov(pos.stm()).neurons(), acc_state.pov(pos.nstm()).neurons(), bucket);
}

ScoreType evaluate(ThreadData& td) { return evaluate(td.position, td.acc_stack.top(td.position)); }

ScoreType adjust(const Position& pos, const ScoreType raw_eval, const ScoreType correction) {
    int adjusted_eval = apply_material_scaling(pos, raw_eval);
    adjusted_eval += correction;

    return std::clamp(adjusted_eval, -MATE_FOUND + 1, MATE_FOUND - 1);
}

} // namespace eval
