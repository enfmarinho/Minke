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
#include <array>

#include "core/position.h"
#include "core/types.h"
#include "eval/nnue/accumulator/state.h"
#include "eval/nnue/arch.h"
#include "eval/nnue/inference.h"
#include "search/search.h"
#include "uci/tune.h"

namespace minke::eval {

ScoreType evaluate(const Position& pos, const nnue::accumulator::State& acc_state) {
    const int bucket = nnue::output_bucket(pos.piece_count());

    return nnue::propagate(acc_state.pov(pos.stm()).neurons(), acc_state.pov(pos.nstm()).neurons(), bucket);
}

ScoreType evaluate(ThreadData& td) { return evaluate(td.position, td.acc_stack.update_top(td.position)); }

ScoreType adjust(const Position& pos, const ScoreType raw_eval, const ScoreType correction,
                 const std::array<ScoreType, 2>* optmism) {
    using namespace minke::tunable;

    const int material_count = pos.piece_count(PAWN) * pawn_scaling_factor()       //
                               + pos.piece_count(KNIGHT) * knight_scaling_factor() //
                               + pos.piece_count(BISHOP) * bishop_scaling_factor() //
                               + pos.piece_count(ROOK) * rook_scaling_factor()     //
                               + pos.piece_count(QUEEN) * queen_scaling_factor();

    int adjusted_eval = 0;
    adjusted_eval += raw_eval * (material_scaling_base() + material_count) / 32768; // material scaling
    adjusted_eval += correction;
    if (optmism != nullptr) {
        adjusted_eval += (*optmism)[pos.stm()] * (optimism_base() + material_count) / 32768; // optimism
    }

    return std::clamp(adjusted_eval, -MATE_FOUND + 1, MATE_FOUND - 1);
}

} // namespace minke::eval
