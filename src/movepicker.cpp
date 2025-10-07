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

MovePicker::MovePicker(Move ttmove, ThreadData *td, bool qsearch) { init(ttmove, td, qsearch); }

void MovePicker::init(Move ttmove, ThreadData *td, bool qsearch) {
    m_td = td;
    m_ttmove = ttmove;
    m_qsearch = qsearch;

    if (ttmove != MOVE_NONE)
        m_stage = PICK_TT;
    else
        m_stage = GEN_NOISY;

    m_killer1 = td->search_history.consult_killer1(td->height);
    m_killer2 = td->search_history.consult_killer2(td->height);

    m_counter = MOVE_NONE;
    if (m_td->height > 0)
        m_counter = td->search_history.consult_counter(td->nodes[td->height - 1].curr_move);
    if (m_counter == m_killer1 || m_counter == m_killer2)
        m_counter = MOVE_NONE;

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
            }
            // Fall-through
        case GEN_NOISY:
            m_end = gen_moves(m_curr, m_td->position, NOISY);
            score_moves();
            m_stage = PICK_GOOD_NOISY;
            // Fall-through
        case PICK_GOOD_NOISY:
            while (m_curr != m_end) {
                sort_next_move();
                if (!SEE(m_td->position, m_curr->move, 0) || m_curr->score == NON_QUEEN_PROMOTION_SCORE) // Bad noisy
                    *m_end_bad++ = *m_curr++;
                else if (m_curr->move != m_ttmove)
                    return *m_curr++;
                else
                    ++m_curr;
            }
            if (m_qsearch && skip_quiets) {
                m_stage = FINISHED;
                return SCORED_MOVE_NONE;
            } else if (skip_quiets) {
                m_curr = m_moves;
                m_stage = PICK_BAD_NOISY;
                return next_move_scored(skip_quiets); // Work around to avoid the switch fall-through
            } else {
                m_curr = m_end_bad;
                m_stage = GEN_QUIET;
            }
            // Fall-through
        case GEN_QUIET:
            m_end = gen_moves(m_curr, m_td->position, QUIET);
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
                else if (runner->move == m_counter)
                    runner->score = COUNTER_SCORE;
                else
                    runner->score = m_td->search_history.get_history(m_td->position, runner->move);
                break;
            case CAPTURE:
                runner->score = CAPTURE_SCORE + 20 * SEE_VALUES[m_td->position.consult(runner->move.to())] +
                                m_td->search_history.get_capture_history(m_td->position, runner->move);
                break;
            case EP:
                runner->score = CAPTURE_SCORE + 20 * SEE_VALUES[PAWN] +
                                m_td->search_history.get_capture_history(m_td->position, runner->move);
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
