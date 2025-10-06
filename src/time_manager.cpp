/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "time_manager.h"

#include "move.h"
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
        m_optimum_time = m_maximum_time = m_start_time + movetime;
        return;
    }

    time = std::min(time - overhead, time / 2); // Decrease the overhead on total time
    time = std::max(time, 1);                   // Ensure time is positive
    inc = std::max(inc, 0);                     // Ensure inc is non negative
    mtg = (mtg > 0 ? std::min(mtg, 50) : tm_default_mtg());

    m_maximum_time = m_start_time + (tm_max_time_scale() / 100.0) * time;

    double base_time = time / static_cast<double>(mtg) + (tm_inc_scale() / 100.0) * inc;
    m_optimum_base_time = base_time * (tm_opt_time_scale() / 100.0);
    m_optimum_time = m_start_time + m_optimum_base_time;
    m_optimum_time = std::min(m_optimum_time, m_maximum_time);
}

void TimeManager::reset() {
    m_movetime = false;
    m_can_stop = false;
    m_time_set = false;
    m_maximum_time = m_optimum_time = m_start_time = now();
    m_optimum_base_time = 0;

    prev_best_move = MOVE_NONE;
    move_stability_factor = 0;
}

void TimeManager::update(const ThreadData &td, const Move &best_move, const int &depth) {
    const double MOVE_STABILITY_FACTORS[5] = {
        tm_move_stability_factor1() / 100.0, tm_move_stability_factor2() / 100.0, tm_move_stability_factor3() / 100.0,
        tm_move_stability_factor4() / 100.0, tm_move_stability_factor5() / 100.0,
    };
    if (m_movetime || !m_time_set)
        return;

    if (best_move == prev_best_move) {
        move_stability_factor = std::min(move_stability_factor + 1, 4);
    } else {
        move_stability_factor = 0;
    }

    if (depth > tm_update_min_depth()) {
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
