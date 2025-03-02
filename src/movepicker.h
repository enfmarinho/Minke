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

enum MoveGenStage {
    PickTT,
    GenNoisy,
    PickGoodNoisy,
    GenQuiet,
    PickQuiet,
    PickBadNoisy,
    Finished,
};

class MovePicker {
  public:
    MovePicker() = default;
    MovePicker(Move ttmove, const SearchData *search_data);
    ~MovePicker() = default;

    void init(Move ttmove, const SearchData *search_data);
    Move next_move(bool skip_quiet);
    ScoredMove next_move_scored(bool skip_quiet);
    inline bool finished() const { return stage == Finished; }

  private:
    MoveGenStage stage;
    ScoredMove moves[MaxMoves];
    ScoredMove *curr, *end, *end_bad;
    Move ttmove;
    const SearchData *search_data;
};

#endif // #ifndef MOVEPICKER_H
