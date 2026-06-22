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

#include "thread.h"

#include <cstring>

#include "types.h"

ThreadData::ThreadData() {
    datagen = false;
    report = true;
    reset_search_parameters();
}

void ThreadData::reset_search_parameters() {
    best_move = MOVE_NONE;
    stop = true;
    ply = 0;
    nodes_searched = -1; // Avoid counting the root
    std::memset(node_table, 0, sizeof(node_table));
    time_manager.reset();
    search_limits.reset();
    // TODO i dont think this is necessary
    for (int i = 0; i < MAX_SEARCH_DEPTH; ++i)
        stack[i].reset();
}

void ThreadData::set_search_limits(const SearchLimits sl) { this->search_limits = sl; }

bool ThreadData::stop_search() const {
    return time_manager.time_over() || stop || nodes_searched > search_limits.maximum_node;
}
