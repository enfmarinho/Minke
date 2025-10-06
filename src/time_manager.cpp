/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "time_manager.h"

#include "move.h"
#include "search.h"
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
        m_optimum_time = m_maximum_time = m_start_time + movetime;
        return;
    }

    time = std::min(time - overhead, time / 2); // Decrease the overhead on total time
    time = std::max(time, 1);                   // Ensure time is positive
    inc = std::max(inc, 0);                     // Ensure inc is non negative
    mtg = (mtg > 0 ? std::min(mtg, 50) : 50);   // Ensure movestogo is at most 50 if positive, else set it to 50

    double base_time = 0.8 * time / static_cast<double>(mtg) + inc;
    m_optimum_base_time = base_time;
    m_optimum_time = m_start_time + base_time;
    m_maximum_time = m_start_time + 4 * base_time;

    // Limit time usage to 80% of total game time
    TimeType max_time = m_start_time + 0.8 * time;
    m_optimum_time = std::min(m_optimum_time, max_time);
    m_maximum_time = std::min(m_maximum_time, max_time);
}

void TimeManager::reset() {
    m_movetime = false;
    m_can_stop = false;
    m_time_set = false;
    m_start_time = now();
    m_optimum_base_time = 0;
}

void TimeManager::update(const ThreadData &td, const Move &best_move, const int &depth) {
    constexpr double MOVE_STABILITY_FACTORS[5] = {1.5, 1.2, 1, 0.9, 0.8};
    if (m_movetime || !m_time_set)
        return;

    if (best_move == prev_best_move) {
        move_stability_factor = std::min(move_stability_factor + 1, 4);
    } else {
        move_stability_factor = 0;
    }

    if (depth > 4) {
        // double best_move_node_fraction = static_cast<double>(td.nodes_searched_table[best_move.from_and_to()]) /
        //                                  static_cast<double>(td.nodes_searched);
        // double scale = (tm_node_base() / 100.0 - best_move_node_fraction) * (tm_node_scale() / 100.0);

        double scale = MOVE_STABILITY_FACTORS[move_stability_factor];

        m_optimum_base_time *= scale;
        m_optimum_time = m_start_time + m_optimum_base_time;
    }
}

bool TimeManager::stop_early() const { return m_can_stop && now() > m_optimum_time; }

bool TimeManager::time_over() const { return m_can_stop && now() > m_maximum_time; }

TimeType TimeManager::time_passed() const { return now() - m_start_time; }

void TimeManager::can_stop() {
    if (m_time_set) // If time is not set, search should stop only with the stop command
        m_can_stop = true;
}
