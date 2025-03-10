/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef MOVEPICKER_H
#define MOVEPICKER_H

#include "search.h"
#include "types.h"

enum BaseScore {
    TT_Score = 100'000,
    QueenPromotionScore = 90'000,
    NonQueenPromotionScore = -90'000,
    CaptureScore = 20'000
};

enum MovePickerStage {
    PickTT,
    GenNoisy,
    PickGoodNoisy,
    GenQuiet,
    PickQuiet,
    PickBadNoisy,
    Finished
};

class MovePicker {
  public:
    MovePicker() = default;
    MovePicker(Move ttmove, ThreadData *thread_data, bool qsearch);
    ~MovePicker() = default;

    void init(Move ttmove, ThreadData *thread_data, bool qsearch);
    Move next_move();
    ScoredMove next_move_scored();

  private:
    void sort_next_move();
    void score_moves();

    bool qsearch;
    MovePickerStage stage;
    ScoredMove moves[MaxMoves];
    ScoredMove *curr, *end, *end_bad;
    Move ttmove;
    ThreadData *thread_data;
};

#endif // #ifndef MOVEPICKER_H
