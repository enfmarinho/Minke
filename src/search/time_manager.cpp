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

#include "search/time_manager.h"

#include "core/types.h"
#include "search/search.h"
#include "uci/tune.h"

TimeManager::TimeManager() { reset(); }

void TimeManager::reset(CounterType inc, CounterType time, CounterType mtg, CounterType movetime, bool infinite) {
    constexpr int overhead = 50;

    reset();

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

    const TimeType limit = std::max(std::max(time - overhead, time / 2),
                                    1);       // Decrease the overhead from total time and ensure limit its positive
    inc = std::max(inc, 0);                   // Ensure inc is non negative
    mtg = (mtg > 0 ? mtg : tm_default_mtg()); // set mtg to default if invalid, i.e. if non-positive

    const double base_time = limit / static_cast<double>(mtg) + inc * tm_increment_factor() / 100.0;

    m_maximum_time = limit * tm_max_time_factor() / 100.0;
    m_optimum_time = std::min<TimeType>(base_time * tm_opt_time_factor() / 100.0, m_maximum_time);
}

void TimeManager::reset() {
    m_scale = 1;
    m_movetime = false;
    m_can_stop = false;
    m_time_set = false;
    m_start_time = now();
}

void TimeManager::update(const ThreadData &td, CounterType pv_stability, CounterType score_stability) {
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

bool TimeManager::stop_early() const { return m_can_stop && time_passed() > m_optimum_time * m_scale; }

bool TimeManager::time_over() const { return m_can_stop && time_passed() > m_maximum_time; }

TimeType TimeManager::time_passed() const { return now() - m_start_time; }

void TimeManager::can_stop() {
    if (m_time_set) // If time is not set, search should stop only with the stop command
        m_can_stop = true;
}
