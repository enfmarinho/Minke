/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "time_manager.h"

#include "types.h"

TimeManager::TimeManager() : m_time_set{false}, m_can_stop{false} {}

void TimeManager::init(const Position &position, CounterType inc, CounterType time, CounterType movestogo,
                       CounterType movetime, bool infinite) {
    m_can_stop = false;
    m_time_set = false;
    m_start_time = now();

    if (inc != -1) {
        m_time_set = true; // That is not needed, it is possible to really on m_can_stop
        m_optimum_time = inc;
        m_maximum_time = inc;
    }

    // TODO calculate optimum time and maximum time
}

void TimeManager::init() {
    m_can_stop = false;
    m_time_set = false;
    m_start_time = now();
}

bool TimeManager::stop_early() const { return m_can_stop && m_time_set && time_passed() > m_optimum_time; }

bool TimeManager::time_over() const { return m_can_stop && m_time_set && time_passed() > m_maximum_time; }

TimeType TimeManager::time_passed() const { return now() - m_start_time; }

void TimeManager::can_stop() {
    if (m_time_set) // If time is not set, search should stop only with "stop" flag
        m_can_stop = true;
}
