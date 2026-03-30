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

#include "time_manager.h"

#include "search.h"
#include "tune.h"
#include "types.h"

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

    time = std::min(time - overhead, time / 2); // Decrease the overhead on total time
    time = std::max(time, 1);                   // Ensure time is positive
    inc = std::max(inc, 0);                     // Ensure inc is non negative
    mtg = (mtg > 0 ? std::min(mtg, 50) : 50);   // Ensure movestogo is at most 50 if positive, else set it to 50

    double base_time = 0.8 * time / static_cast<double>(mtg) + inc;
    m_optimum_time = base_time;
    m_maximum_time = 4 * base_time;

    // Limit time usage to 80% of total game time
    TimeType max_time = 0.8 * time;
    m_optimum_time = std::min(m_optimum_time, max_time);
    m_maximum_time = std::min(m_maximum_time, max_time);
}

void TimeManager::reset() {
    m_scale = 1;
    m_movetime = false;
    m_can_stop = false;
    m_time_set = false;
    m_start_time = now();
}

void TimeManager::update(const ThreadData &td) {
    if (m_movetime || !m_time_set)
        return;

    const double node_fraction = td.node_table[td.best_move.from_and_to()] / static_cast<double>(td.nodes_searched);
    const double node_scaling_factor = (node_tm_base() / 100.0 - node_fraction) * (node_tm_scale() / 100.0);
    m_scale = std::clamp<double>(node_scaling_factor, tm_min_scale() / 100.0, tm_max_scale() / 100.0);
}

bool TimeManager::stop_early() const { return m_can_stop && time_passed() > m_optimum_time * m_scale; }

bool TimeManager::time_over() const { return m_can_stop && time_passed() > m_maximum_time; }

TimeType TimeManager::time_passed() const { return now() - m_start_time; }

void TimeManager::can_stop() {
    if (m_time_set) // If time is not set, search should stop only with the stop command
        m_can_stop = true;
}
