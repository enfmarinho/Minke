/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "movepicker.h"

#include <utility>

#include "move.h"
#include "movegen.h"
#include "search.h"
#include "types.h"

MovePicker::MovePicker(Move ttmove, ThreadData *thread_data, bool qsearch) { init(ttmove, thread_data, qsearch); }

void MovePicker::init(Move ttmove, ThreadData *thread_data, bool qsearch) {
    this->m_thread_data = thread_data;
    this->m_ttmove = ttmove;
    this->m_qsearch = qsearch;

    // Don't pick ttmove if in qsearch unless ttmove is noisy
    if (ttmove != MOVE_NONE)
        m_stage = PICK_TT;
    else
        m_stage = GEN_NOISY;

    m_killer1 = thread_data->search_history.consult_killer1(thread_data->searching_ply);
    m_killer2 = thread_data->search_history.consult_killer2(thread_data->searching_ply);
    m_curr = m_end = m_end_bad = m_moves;
}

Move MovePicker::next_move(const bool &skip_quiets) { return next_move_scored(skip_quiets).move; }

ScoredMove MovePicker::next_move_scored(const bool &skip_quiets) {
    switch (m_stage) {
        case PICK_TT:
            m_stage = GEN_NOISY;
            if (!skip_quiets || m_ttmove.is_noisy()) {
                return {m_ttmove, TT_SCORE};
            } else {
                // Fall-through
            }
        case GEN_NOISY:
            m_end = gen_moves(m_curr, m_thread_data->position, NOISY);
            score_moves();
            m_stage = PICK_GOOD_NOISY;
            // Fall-through
        case PICK_GOOD_NOISY:
            while (m_curr != m_end) {
                sort_next_move();
                if (!SEE(m_thread_data->position, m_curr->move, 0) ||
                    m_curr->score == NON_QUEEN_PROMOTION_SCORE) // Bad noisy
                    *m_end_bad++ = *m_curr++;
                else if (m_curr->move != m_ttmove)
                    return *m_curr++;
                else
                    ++m_curr;
            }
            if (m_qsearch) {
                m_stage = FINISHED;
                return SCORED_MOVE_NONE;
            }
            if (skip_quiets) {
                m_curr = m_moves;
                m_stage = PICK_BAD_NOISY;
                return next_move_scored(skip_quiets); // Work around to avoid the switch fall-through
            } else {
                m_curr = m_end_bad;
                m_stage = GEN_QUIET;
            }
            // Fall-through
        case GEN_QUIET:
            m_end = gen_moves(m_curr, m_thread_data->position, QUIET);
            score_moves();
            m_stage = PICK_QUIET;
            // Fall-through
        case PICK_QUIET:
            while (m_curr != m_end) {
                sort_next_move();
                if (m_curr->move != m_ttmove)
                    return *m_curr++;
                else
                    ++m_curr;
            }
            m_curr = m_moves;
            m_stage = PICK_BAD_NOISY;
            // Fall-through
        case PICK_BAD_NOISY:
            while (m_curr != m_end_bad) {
                // No need to call sort_next_move(), since bad noisy moves are already sorted in PickGoodNoisy
                if (m_curr->move != m_ttmove)
                    return *m_curr++;
                else
                    ++m_curr;
            }
            m_stage = FINISHED;
            // Fall-through
        case FINISHED:
            return SCORED_MOVE_NONE;
        default:
            __builtin_unreachable();
    }
}

void MovePicker::sort_next_move() {
    ScoredMove *best_move = m_curr;
    for (ScoredMove *runner = m_curr + 1; runner != m_end; ++runner) {
        if (runner->score > best_move->score)
            best_move = runner;
    }
    std::swap(*best_move, *m_curr);
}

void MovePicker::score_moves() {
    for (ScoredMove *runner = m_curr; runner != m_end; ++runner) {
        switch (runner->move.type()) {
            case CASTLING:
                // Fall-through
            case REGULAR:
                if (runner->move == m_killer1)
                    runner->score = KILLER_1_SCORE;
                else if (runner->move == m_killer2)
                    runner->score = KILLER_2_SCORE;
                else
                    runner->score = m_thread_data->search_history.get_history(m_thread_data->position, runner->move);
                break;
            case CAPTURE:
                runner->score = CAPTURE_SCORE + 10 * SEE_VALUES[m_thread_data->position.consult(runner->move.to())] -
                                SEE_VALUES[m_thread_data->position.consult(runner->move.from())] / 10;
                assert(runner->score > CAPTURE_SCORE && runner->score < CAPTURE_SCORE + 10'000);
                break;
            case EP:
                runner->score = CAPTURE_SCORE;
                break;
            case PAWN_PROMOTION_QUEEN:
                // Fall-through
            case PAWN_PROMOTION_QUEEN_CAPTURE:
                runner->score = QUEEN_PROMOTION_SCORE;
                break;
            default: // Non queen promotion
                runner->score = NON_QUEEN_PROMOTION_SCORE;
                assert(runner->move.is_promotion());
                break;
        }
    }
}
