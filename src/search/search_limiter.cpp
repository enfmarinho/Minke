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

#include "search/search_limiter.h"

#include <algorithm>
#include <cstdint>
#include <limits>

#include "core/types.h"
#include "search/search.h"
#include "uci/tune.h"

void SearchLimiter::init(const SearchLimits& sl) {
    constexpr uint64_t overhead = 50;

    m_start_time = now();
    m_movetime = false;
    m_can_stop = false;
    m_scale = 1.0;

    const bool infinite = sl.infinite.value_or(false);
    const uint64_t time = sl.time_remaining.value_or(0);
    uint64_t inc = sl.time_increment.value_or(0);
    uint64_t movetime = sl.movetime.value_or(0);
    uint64_t mtg = sl.mtg.value_or(0);

    m_max_depth = sl.depth.value_or(MAX_SEARCH_DEPTH);
    m_optimum_nodes = sl.optimum_node.value_or(std::numeric_limits<uint64_t>::max());
    m_maximum_nodes = sl.maximum_node.value_or(std::numeric_limits<uint64_t>::max());

    // Neither time nor movetime was set, both are non-positive or infinite flag was used, so search until stop command
    m_time_set = (time > 0 || movetime > 0) && !infinite;
    if (!m_time_set)
        return;

    if (movetime > 0) { // Movetime set
        m_movetime = true;
        movetime = std::max(movetime - overhead, movetime / 2);
        m_optimum_time = m_maximum_time = movetime;
        return;
    }

    const TimeType limit = std::max<uint64_t>(std::max(time - overhead, time / 2),
                                              1); // Decrease the overhead from total time and ensure limit its positive
    inc = std::max<uint64_t>(inc, 0);             // Ensure inc is non negative
    mtg = (mtg > 0 ? mtg : tm_default_mtg());     // set mtg to default if invalid, i.e. if non-positive

    const double base_time = limit / static_cast<double>(mtg) + inc * tm_increment_factor() / 100.0;

    m_maximum_time = limit * tm_max_time_factor() / 100.0;
    m_optimum_time = std::min<TimeType>(base_time * tm_opt_time_factor() / 100.0, m_maximum_time);
}

void SearchLimiter::init() {
    m_start_time = now();
    m_optimum_time = std::numeric_limits<uint64_t>::max();
    m_maximum_time = std::numeric_limits<uint64_t>::max();

    m_optimum_nodes = std::numeric_limits<uint64_t>::max();
    m_maximum_nodes = std::numeric_limits<uint64_t>::max();

    m_max_depth = MAX_SEARCH_DEPTH;

    m_movetime = false;
    m_time_set = false;
    m_can_stop = false;
}

void SearchLimiter::update(const ThreadData& td, CounterType pv_stability, CounterType score_stability) {
    if (m_movetime || !m_time_set)
        return;

    const double pv_stability_scale =
        std::max(tm_pv_stability_base() / 1000.0 - pv_stability * tm_pv_stability_factor() / 1000.0,
                 tm_pv_stability_min_scale() / 1000.0);

    const double score_stability_scale =
        std::max(tm_score_stability_base() / 1000.0 - score_stability * tm_score_stability_factor() / 1000.0,
                 tm_score_stability_min_scale() / 1000.0);

    const double node_fraction = td.node_table[td.best_move.from_and_to()] / static_cast<double>(td.nodes_searched);
    const double node_spent_scale = (tm_node_spent_base() / 1000.0 - node_fraction) * (tm_node_spent_factor() / 1000.0);

    m_scale = std::clamp<double>(node_spent_scale * pv_stability_scale * score_stability_scale, tm_min_scale() / 1000.0,
                                 tm_max_scale() / 1000.0);
}

bool SearchLimiter::stop_early(uint64_t nodes) const {
    return nodes > m_optimum_nodes || (m_can_stop && time_passed() > m_optimum_time * m_scale);
}

bool SearchLimiter::time_over(uint64_t nodes) const {
    return nodes > m_maximum_nodes || (m_can_stop && ((nodes & 2047) == 2047 && time_passed() > m_maximum_time));
}

CounterType SearchLimiter::max_depth() const { return m_max_depth; }

TimeType SearchLimiter::time_passed() const { return now() - m_start_time; }

void SearchLimiter::can_stop() {
    if (m_time_set) // If time is not set, search should stop only with the stop command
        m_can_stop = true;
}
