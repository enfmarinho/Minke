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

#include "accumulator.h"

#include "pov_accumulator.h"

Accumulator::Accumulator(const Square white_king_sq, const Square black_king_sq, const PovAccumulator &white_pov_acc,
                         const PovAccumulator &black_pov_acc)
    : m_pov_accumulators{white_pov_acc, black_pov_acc} {
    m_updated[WHITE] = m_updated[BLACK] = true;
    m_king_sqs[WHITE] = white_king_sq;
    m_king_sqs[BLACK] = black_king_sq;
}

Accumulator::Accumulator(const DirtyPiece &dp, const Square white_king_sq, const Square black_king_sq) {
    init(dp, white_king_sq, black_king_sq);
}

void Accumulator::init(const DirtyPiece &dp, const Square white_king_sq, const Square black_king_sq) {
    m_updated[WHITE] = m_updated[BLACK] = false;

    m_king_sqs[WHITE] = white_king_sq;
    m_king_sqs[BLACK] = black_king_sq;

    m_dirty_piece = dp;
}

void Accumulator::update(const Color pov, const PovAccumulator &prev_pov_acc) {
    if (m_updated[pov])
        return;

    // clang-format off
    switch (m_dirty_piece.move_type) {
        case REGULAR:
            m_pov_accumulators[pov].add_sub(prev_pov_acc, 
                                            feature_idx(m_dirty_piece.add0, m_king_sqs[pov], pov),
                                            feature_idx(m_dirty_piece.sub0, m_king_sqs[pov], pov));
            break;
        case CAPTURE:
            m_pov_accumulators[pov].add_sub2(prev_pov_acc, 
                                             feature_idx(m_dirty_piece.add0, m_king_sqs[pov], pov),
                                             feature_idx(m_dirty_piece.sub0, m_king_sqs[pov], pov),
                                             feature_idx(m_dirty_piece.sub1, m_king_sqs[pov], pov));
            break;
        case CASTLING:
            m_pov_accumulators[pov].add2_sub2(prev_pov_acc, 
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

bool Accumulator::needs_refresh(const Color pov, const Square new_king_sq) const {
    return (new_king_sq & 0b100) != (m_king_sqs[pov] & 0b100) ||                       // King crossed half of the board
           king_bucket_idx(new_king_sq, pov) != king_bucket_idx(m_king_sqs[pov], pov); // King bucket change
}

void Accumulator::refresh(const Color side, const PovAccumulator &finny_table_neurons) {
    m_pov_accumulators[side] = finny_table_neurons;
    m_updated[side] = true;
}

// This does not check if the Accumulators are identical, only their neurons. Used for debugging
bool operator==(const Accumulator &lhs, const Accumulator &rhs) {
    for (int color_i = 0; color_i <= 1; ++color_i) {
        Color color = static_cast<Color>(color_i);
        if (lhs.pov(color) != rhs.pov(color))
            return false;
        if (lhs.m_updated[color] != rhs.m_updated[color])
            return false;
    }

    return true;
}
