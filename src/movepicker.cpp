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
    this->thread_data = thread_data;
    this->ttmove = ttmove;
    this->qsearch = qsearch;

    // Don't pick ttmove if in qsearch unless ttmove is noisy
    if (ttmove != MoveNone)
        stage = PickTT;
    else
        stage = GenNoisy;

    killer1 = thread_data->search_history.consult_killer1(thread_data->searching_ply);
    killer2 = thread_data->search_history.consult_killer2(thread_data->searching_ply);
    curr = end = end_bad = moves;
}

Move MovePicker::next_move(const bool &skip_quiets) { return next_move_scored(skip_quiets).move; }

ScoredMove MovePicker::next_move_scored(const bool &skip_quiets) {
    switch (stage) {
        case PickTT:
            stage = GenNoisy;
            if (!skip_quiets || ttmove.is_noisy()) {
                return {ttmove, TT_Score};
            } else {
                // Fall-through
            }
        case GenNoisy:
            end = gen_moves(curr, thread_data->position, Noisy);
            score_moves();
            stage = PickGoodNoisy;
            // Fall-through
        case PickGoodNoisy:
            while (curr != end) {
                sort_next_move();
                if (!SEE(thread_data->position, curr->move, 0) || curr->score == NonQueenPromotionScore) // Bad noisy
                    *end_bad++ = *curr++;
                else if (curr->move != ttmove)
                    return *curr++;
                else
                    ++curr;
            }
            if (qsearch) {
                stage = Finished;
                return ScoredMoveNone;
            }
            if (skip_quiets) {
                curr = moves;
                stage = PickBadNoisy;
                return next_move_scored(skip_quiets); // Work around to avoid the switch fall-through
            } else {
                curr = end_bad;
                stage = GenQuiet;
            }
            // Fall-through
        case GenQuiet:
            end = gen_moves(curr, thread_data->position, Quiet);
            score_moves();
            stage = PickQuiet;
            // Fall-through
        case PickQuiet:
            while (curr != end) {
                sort_next_move();
                if (curr->move != ttmove)
                    return *curr++;
                else
                    ++curr;
            }
            curr = moves;
            stage = PickBadNoisy;
            // Fall-through
        case PickBadNoisy:
            while (curr != end_bad) {
                // No need to call sort_next_move(), since bad noisy moves are already sorted in PickGoodNoisy
                if (curr->move != ttmove)
                    return *curr++;
                else
                    ++curr;
            }
            stage = Finished;
            // Fall-through
        case Finished:
            return ScoredMoveNone;
        default:
            __builtin_unreachable();
    }
}

void MovePicker::sort_next_move() {
    ScoredMove *best_move = curr;
    for (ScoredMove *runner = curr + 1; runner != end; ++runner) {
        if (runner->score > best_move->score)
            best_move = runner;
    }
    std::swap(*best_move, *curr);
}

void MovePicker::score_moves() {
    for (ScoredMove *runner = curr; runner != end; ++runner) {
        switch (runner->move.type()) {
            case Castling:
                // Fall-through
            case Regular:
                if (runner->move == killer1)
                    runner->score = Killer1Score;
                else if (runner->move == killer2)
                    runner->score = Killer2Score;
                else
                    runner->score = thread_data->search_history.consult(thread_data->position, runner->move);
                break;
            case Capture:
                runner->score = CaptureScore + 10 * SEE_values[thread_data->position.consult(runner->move.to())] -
                                SEE_values[thread_data->position.consult(runner->move.from())] / 10;
                assert(runner->score > CaptureScore && runner->score < CaptureScore + 10'000);
                break;
            case EnPassant:
                runner->score = CaptureScore;
                break;
            case PawnPromotionQueen:
                // Fall-through
            case PawnPromotionQueenCapture:
                runner->score = QueenPromotionScore;
                break;
            default: // Non queen promotion
                runner->score = NonQueenPromotionScore;
                assert(runner->move.is_promotion());
                break;
        }
    }
}
