/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "movepicker.h"

#include "move.h"

MovePicker::MovePicker(Move ttmove, const SearchData *search_data) { init(ttmove, search_data); }

void MovePicker::init(Move ttmove, const SearchData *search_data) {
    this->ttmove = ttmove;
    this->search_data = search_data;
    curr = moves;
    end = moves;
    end_bad = moves;
    stage = (ttmove == MoveNone ? PickTT : GenNoisy);
}

Move MovePicker::next_move(bool skip_quiet) { return next_move_scored(skip_quiet).move; }

ScoredMove MovePicker::next_move_scored(bool skip_quiet) {
    // TODO
    return ScoredMoveNone;
}
