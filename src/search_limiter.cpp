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

#include "search_limiter.h"

#include <cstddef>

#include "thread.h"
#include "tune.h"
#include "types.h"

void SearchLimiter::reset(const SearchLimits &limits) {
    constexpr int overhead = 50;

    m_movetime_set = false;
    m_can_stop = false;

    m_scale = 1.0;
    m_start_time = now();
    m_optimum_time = NO_TIME_LIMIT;
    m_maximum_time = NO_TIME_LIMIT;

    m_optimum_nodes = limits.optimum_node;
    m_maximum_nodes = limits.maximum_node;
    m_depth_limit = std::min(limits.depth, MAX_SEARCH_DEPTH - 1); // depth limit cannot be higher than MAX_SEARCH_DEPTH

    // Neither time nor movetime was set, both are non-positive or infinite flag was used, so search until stop command
    if ((limits.time <= 0 && limits.movetime <= 0) || limits.infinite)
        return;

    if (limits.movetime > 0) { // Movetime set
        m_movetime_set = true;
        m_optimum_time = m_maximum_time = std::max(limits.movetime - overhead, limits.movetime / 2);
        return;
    }

    TimeType time = std::min(limits.time - overhead, limits.time / 2); // Decrease the overhead on total time
    time = std::max<TimeType>(time, 1);                                // Ensure time is positive

    TimeType inc = std::max<TimeType>(limits.inc, 0); // Ensure inc is non negative
    TimeType movestogo = (limits.movestogo > 0 ? std::min(limits.movestogo, 50)
                                               : 50); // Ensure movestogo is at most 50 if positive, else set it to 50

    const double base_time = 0.8 * time / static_cast<double>(movestogo) + inc;
    m_optimum_time = base_time;
    m_maximum_time = 4 * base_time;

    // Limit time usage to 80% of total game time
    TimeType max_time = 0.8 * time;
    m_optimum_time = std::min(m_optimum_time, max_time);
    m_maximum_time = std::min(m_maximum_time, max_time);
}

void SearchLimiter::update(const ThreadData &td) {
    if (m_movetime_set || m_optimum_time != NO_TIME_LIMIT)
        return;

    const double node_fraction = td.node_table[td.best_move.from_and_to()] / static_cast<double>(td.nodes_searched);
    const double node_scaling_factor = (node_tm_base() / 100.0 - node_fraction) * (node_tm_scale() / 100.0);
    m_scale = std::clamp<double>(node_scaling_factor, tm_min_scale() / 100.0, tm_max_scale() / 100.0);
}

[[nodiscard]] CounterType SearchLimiter::depth_limit() const { return m_depth_limit; }

bool SearchLimiter::stop_early(const size_t nodes_searched) const {
    return m_can_stop && (nodes_searched >= m_optimum_nodes || (now() - m_start_time >= m_optimum_time * m_scale));
}

bool SearchLimiter::exceeded(const size_t nodes_searched) const {
    return m_can_stop && (nodes_searched >= m_maximum_nodes || (time_passed() >= m_maximum_time));
}

TimeType SearchLimiter::time_passed() const { return now() - m_start_time; }

void SearchLimiter::allow_stopping() { m_can_stop = true; }
