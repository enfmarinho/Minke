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

#include "../position.h"
#include "pov_accumulator.h"

Accumulator::Accumulator(const Position &pos) { reset(pos); }

Accumulator::Accumulator(const DirtyPiece &dp) {
    m_pov_accumulators[WHITE].reset();
    m_pov_accumulators[BLACK].reset();

    m_updated[WHITE] = false;
    m_updated[BLACK] = false;

    m_dirty_piece = dp;
}

void Accumulator::reset(const Position &pos) {
    reset_pov(WHITE, pos);
    reset_pov(BLACK, pos);
}

void Accumulator::update(const Color pov, const PovAccumulator &prev_pov_acc) {
    if (m_updated[pov])
        return;

    // clang-format off
    switch (m_dirty_piece.move_type) {
        case REGULAR:
            m_pov_accumulators[pov].add_sub(prev_pov_acc, 
                                            m_dirty_piece.add0.feature_idx(pov),
                                            m_dirty_piece.sub0.feature_idx(pov));
            break;
        case CAPTURE:
            m_pov_accumulators[pov].add_sub2(prev_pov_acc, 
                                             m_dirty_piece.add0.feature_idx(pov),
                                             m_dirty_piece.sub0.feature_idx(pov),
                                             m_dirty_piece.sub1.feature_idx(pov));
            break;
        case CASTLING:
            m_pov_accumulators[pov].add2_sub2(prev_pov_acc, 
                                              m_dirty_piece.add0.feature_idx(pov), 
                                              m_dirty_piece.add1.feature_idx(pov),
                                              m_dirty_piece.sub0.feature_idx(pov), 
                                              m_dirty_piece.sub1.feature_idx(pov));
            break;
        default:
            __builtin_unreachable();
    }
    // clang-format on

    m_updated[pov] = true;
}

void Accumulator::reset_pov(const Color pov, const Position &pos) {
    m_pov_accumulators[pov].reset();

    for (int sqi = a1; sqi <= h8; ++sqi) {
        const Square sq = static_cast<Square>(sqi);
        const Piece piece = pos.consult(sq);
        if (piece != EMPTY) {
            m_pov_accumulators[pov].self_add(PieceSquare(piece, sq).feature_idx(pov));
        }
    }

    m_updated[pov] = true;
}

// This does not check if the Accumulators are identical, only their neurons needs to be
bool operator==(const Accumulator &lhs, const Accumulator &rhs) {
    for (int color_i = 0; color_i <= 1; ++color_i) {
        Color color = static_cast<Color>(color_i);
        if (lhs.pov(color).neurons() != rhs.pov(color).neurons())
            return false;

        if (lhs.m_updated[color] != rhs.m_updated[color])
            return false;
    }

    return true;
}
