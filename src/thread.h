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

#ifndef THREAD_H
#define THREAD_H

#include <cstddef>
#include <cstdint>

#include "correction.h"
#include "history.h"
#include "move.h"
#include "position.h"
#include "pv_list.h"
#include "time_manager.h"
#include "tt.h"
#include "types.h"

struct SearchStack {
    PieceMove curr_pmove;
    Move excluded_move;
    ScoreType static_eval;
    PvList pv_list;

    inline void reset() {
        curr_pmove = PIECE_MOVE_NONE;
        excluded_move = MOVE_NONE;
        static_eval = SCORE_NONE;
        pv_list.clear();
    }
};

struct ThreadData {
    TranspositionTable tt;

    Position position;
    History search_history;
    CorrectionHistory correction_history;
    SearchStack stack[MAX_SEARCH_DEPTH];
    Move best_move;

    SearchLimits search_limits;
    TimeManager time_manager;
    int64_t nodes_searched;
    int64_t node_table[64 * 64];
    int ply;
    bool stop;
    bool datagen;
    bool report;
    bool chess960;

    ThreadData();
    void reset_search_parameters();
    void set_search_limits(const SearchLimits sl);

    bool stop_search() const;
};

#endif // !THREAD_H
