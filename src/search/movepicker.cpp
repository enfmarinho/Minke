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

#include "search/movepicker.h"

#include <utility>

#include "core/move.h"
#include "core/movegen.h"
#include "core/types.h"
#include "search/search.h"
#include "uci/tune.h"

MovePicker::MovePicker(Move ttmove, ThreadData &td, MovePickerType mp_type, ScoreType threshold) {
    init(ttmove, td, mp_type, threshold);
}

void MovePicker::init(Move ttmove, ThreadData &td, MovePickerType mp_type, ScoreType threshold) {
    m_td = &td;
    m_ttmove = ttmove;
    m_mp_type = mp_type;
    m_threshold = threshold;
    m_move_list.clear();

    if (ttmove)
        m_stage = PICK_TT;
    else
        m_stage = GEN_NOISY;

    m_killer1 = m_td->search_history.consult_killer1(m_td->height);
    m_killer2 = m_td->search_history.consult_killer2(m_td->height);

    m_idx = m_end = m_bad_noisy_end = 0;
}

Move MovePicker::next_move(bool skip_quiets) {
    switch (m_stage) {
        case PICK_TT:
            m_stage = GEN_NOISY;
            if ((!skip_quiets || m_ttmove.is_noisy()) && m_td->position.is_pseudo_legal(m_ttmove) &&
                m_td->position.is_legal(m_ttmove)) {
                return m_ttmove;
            } else {
            }
            [[fallthrough]];
        case GEN_NOISY:
            Movegen::noisies(m_move_list, m_td->position);
            m_end = m_move_list.size();
            score_noisy_moves();
            m_stage = PICK_GOOD_NOISY;
            [[fallthrough]];
        case PICK_GOOD_NOISY:
            while (m_idx != m_end) {
                const size_t idx = sort_next_move();
                const auto [move, score] = m_move_list[idx];

                const ScoreType see_threshold =
                    m_mp_type == PROBCUT ? m_threshold : -score / 32 + mp_see_threshold_base();

                if (!SEE(m_td->position, move, see_threshold)) // Bad noisy
                    m_move_list[m_bad_noisy_end++] = m_move_list[idx];
                else if (move != m_ttmove)
                    return move;
            }
            if ((m_mp_type == QSEARCH && skip_quiets) || m_mp_type == PROBCUT) {
                m_stage = FINISHED;
                return Move::none();
            } else if (skip_quiets) {
                m_idx = 0;
                m_stage = PICK_BAD_NOISY;
                return next_move(skip_quiets); // Work around to avoid the switch fall-through
            } else {
                m_idx = m_bad_noisy_end;
                m_move_list.resize(m_bad_noisy_end);
                m_stage = GEN_QUIET;
            }
            [[fallthrough]];
        case GEN_QUIET:
            Movegen::quiets(m_move_list, m_td->position);
            m_end = m_move_list.size();
            score_quiet_moves();
            m_stage = PICK_QUIET;
            [[fallthrough]];
        case PICK_QUIET:
            while (m_idx != m_end && !skip_quiets) {
                size_t idx = sort_next_move();
                const auto [move, score] = m_move_list[idx];

                if (move != m_ttmove)
                    return move;
            }
            m_idx = 0;
            m_stage = PICK_BAD_NOISY;
            [[fallthrough]];
        case PICK_BAD_NOISY:
            while (m_idx != m_bad_noisy_end) {
                // No need to call 'sort_next_move', since bad noisy moves are already sorted in PickGoodNoisy
                const auto [move, score] = m_move_list[m_idx++];
                if (move != m_ttmove)
                    return move;
            }
            m_stage = FINISHED;
            [[fallthrough]];
        case FINISHED:
            return Move::none();
        default:
            __builtin_unreachable();
    }
}

size_t MovePicker::sort_next_move() {
    size_t best_move_idx = m_idx;
    for (size_t i = m_idx + 1; i < m_end; ++i) {
        if (m_move_list[i].score > m_move_list[best_move_idx].score)
            best_move_idx = i;
    }
    std::swap(m_move_list[best_move_idx], m_move_list[m_idx]);

    return m_idx++;
}

void MovePicker::score_quiet_moves() {
    for (size_t i = m_idx; i < m_end; ++i) {
        auto &[move, score] = m_move_list[i];
        score = m_td->search_history.get_history(*m_td, move);
        if (move == m_killer1)
            score += mp_killer1_bonus();
        else if (move == m_killer2)
            score += mp_killer2_bonus();
    }
}

void MovePicker::score_noisy_moves() {
    for (size_t i = m_idx; i < m_end; ++i) {
        auto &[move, score] = m_move_list[i];
        const Piece captured = [&]() {
            if (move.is_ep())
                return WHITE_PAWN; // color does not matter
            else
                return m_td->position.piece_at(move.to());
        }();

        score = 20 * SEE_VALUES[captured] + m_td->search_history.get_capture_history(m_td->position, move);

        if (move.is_promotion()) {
            score += SEE_VALUES[move.promotee()] - SEE_VALUES[WHITE_PAWN];
        }
    }
}
