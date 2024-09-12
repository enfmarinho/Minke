/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "time_manager.h"

#include "game_elements.h"

TimeManager::TimeManager() {
    m_node_set = false;
    m_time_set = false;
}

void TimeManager::init() {
    m_start_time = now();

    // TODO calculate optimum time and maximum time
}

bool TimeManager::nodes_over(const CounterType &nodes) { return m_node_set && nodes > m_node_limit; }

bool TimeManager::stop_early() { return m_time_set && now() - m_start_time > m_optimum_time; }

bool TimeManager::time_over() { return m_time_set && now() - m_start_time > m_maximum_time; }
