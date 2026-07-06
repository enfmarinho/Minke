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

#ifndef MOVEPICKER_H
#define MOVEPICKER_H

#include <cstddef>

#include "movegen.h"
#include "search.h"
#include "types.h"

enum MovePickerStage {
    PICK_TT,
    GEN_NOISY,
    PICK_GOOD_NOISY,
    GEN_QUIET,
    PICK_QUIET,
    PICK_BAD_NOISY,
    FINISHED
};

enum MovePickerType {
    SEARCH,
    QSEARCH,
    PROBCUT,
};

class MovePicker {
  public:
    MovePicker() = default;
    MovePicker(Move ttmove, ThreadData &td, MovePickerType mp_type, ScoreType threshold = 0);
    ~MovePicker() = default;

    void init(Move ttmove, ThreadData &td, MovePickerType mp_type, ScoreType threshold = 0);
    Move next_move(const bool skip_quiets);
    ScoredMove next_move_scored(const bool skip_quiets);

    MovePickerStage picker_stage() const { return m_stage; }

  private:
    size_t sort_next_move();
    void score_quiet_moves();
    void score_noisy_moves();

    MovePickerType m_mp_type;
    MovePickerStage m_stage;
    Movegen::ScoredMoveList m_move_list;
    size_t m_idx, m_end, m_bad_noisy_end;
    Move m_ttmove, m_killer1, m_killer2, m_counter;
    ThreadData *m_td;
    ScoreType m_threshold;
};

#endif // #ifndef MOVEPICKER_H
