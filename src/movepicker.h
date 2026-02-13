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
    TT_SCORE = 100'000,
    QUEEN_PROMOTION_SCORE = 90'000,
    NON_QUEEN_PROMOTION_SCORE = -90'000,
    CAPTURE_SCORE = 20'000,
    KILLER_1_SCORE = 19'000,
    KILLER_2_SCORE = 18'000,
    COUNTER_SCORE = 17'000,
};

enum MovePickerStage {
    PICK_TT,
    GEN_NOISY,
    PICK_GOOD_NOISY,
    GEN_QUIET,
    PICK_QUIET,
    PICK_BAD_NOISY,
    FINISHED
};

class MovePicker {
  public:
    MovePicker() = default;
    MovePicker(Move ttmove, ThreadData &td, bool qsearch, ScoreType threshold = 0);
    ~MovePicker() = default;

    void init(Move ttmove, ThreadData &td, bool qsearch, ScoreType threshold = 0);
    Move next_move(const bool &skip_quiets);
    ScoredMove next_move_scored(const bool &skip_quiets);

    MovePickerStage picker_stage() const { return m_stage; }

  private:
    void sort_next_move();
    void score_moves();

    bool m_qsearch;
    MovePickerStage m_stage;
    ScoredMove m_moves[MAX_MOVES_PER_POS];
    ScoredMove *m_curr, *m_end, *m_end_bad;
    Move m_ttmove, m_killer1, m_killer2, m_counter;
    ThreadData *m_td;
    ScoreType m_threshold;
};

#endif // #ifndef MOVEPICKER_H
