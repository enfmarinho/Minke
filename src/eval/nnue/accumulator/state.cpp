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

#include "eval/nnue/accumulator/state.h"

#include "core/types.h"
#include "eval/nnue/accumulator/perspective.h"

namespace minke::nnue::accumulator {

State::State(const Perspective &white_perspective_acc, const Perspective &black_perspective_acc,
             const Square white_king_sq, const Square black_king_sq)
    : m_perspective_accs{white_perspective_acc, black_perspective_acc} {
    m_updated[WHITE] = m_updated[BLACK] = true;
    m_king_sqs[WHITE] = white_king_sq;
    m_king_sqs[BLACK] = black_king_sq;
}

State::State(const DirtyPiece dp, const Square white_king_sq, const Square black_king_sq) {
    init(dp, white_king_sq, black_king_sq);
}

void State::init(const DirtyPiece dp, const Square white_king_sq, const Square black_king_sq) {
    m_updated[WHITE] = m_updated[BLACK] = false;

    m_king_sqs[WHITE] = white_king_sq;
    m_king_sqs[BLACK] = black_king_sq;

    m_dirty_piece = dp;
}

void State::update(const Perspective &prev_perspective_acc, const Color pov) {
    if (m_updated[pov])
        return;

    // clang-format off
    switch (m_dirty_piece.move_type) {
        case ADD_SUB:
            m_perspective_accs[pov].add_sub(prev_perspective_acc, 
                                            feature_idx(m_dirty_piece.add0, m_king_sqs[pov], pov),
                                            feature_idx(m_dirty_piece.sub0, m_king_sqs[pov], pov));
            break;
        case ADD_SUB2:
            m_perspective_accs[pov].add_sub2(prev_perspective_acc, 
                                             feature_idx(m_dirty_piece.add0, m_king_sqs[pov], pov),
                                             feature_idx(m_dirty_piece.sub0, m_king_sqs[pov], pov),
                                             feature_idx(m_dirty_piece.sub1, m_king_sqs[pov], pov));
            break;
        case ADD2_SUB2:
            m_perspective_accs[pov].add2_sub2(prev_perspective_acc, 
                                              feature_idx(m_dirty_piece.add0, m_king_sqs[pov], pov), 
                                              feature_idx(m_dirty_piece.add1, m_king_sqs[pov], pov),
                                              feature_idx(m_dirty_piece.sub0, m_king_sqs[pov], pov), 
                                              feature_idx(m_dirty_piece.sub1, m_king_sqs[pov], pov));
            break;
        default:
            assert(false);
            __builtin_unreachable();
    }
    // clang-format on

    m_updated[pov] = true;
}

bool State::needs_refresh(const Color pov, const Square new_king_sq) const {
    return (new_king_sq & 0b100) != (m_king_sqs[pov] & 0b100) ||                       // King crossed half of the board
           king_bucket_idx(new_king_sq, pov) != king_bucket_idx(m_king_sqs[pov], pov); // King bucket change
}

void State::refresh(const Perspective &finny_table_neurons, const Color side) {
    m_perspective_accs[side] = finny_table_neurons;
    m_updated[side] = true;
}

// This does not check if the Accumulators are identical, only their neurons. Used for debugging
bool operator==(const State &lhs, const State &rhs) {
    for (int color_i = 0; color_i <= 1; ++color_i) {
        Color color = static_cast<Color>(color_i);
        if (lhs.pov(color) != rhs.pov(color))
            return false;
        if (lhs.m_updated[color] != rhs.m_updated[color])
            return false;
    }

    return true;
}
} // namespace minke::nnue::accumulator
