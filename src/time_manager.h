/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include "game_elements.h"
#include "position.h"

class TimeManager {
  public:
    TimeManager();
    ~TimeManager() = default;

    void init(const Position &position, CounterType inc, CounterType time, CounterType movestogo, CounterType movetime,
              bool infinite);
    void init();
    bool stop_early() const;
    bool time_over() const;
    TimeType time_passed() const;
    void can_stop();

  private:
    TimeType m_start_time;
    TimeType m_optimum_time;
    TimeType m_maximum_time;

    bool m_time_set;
    bool m_can_stop;
};

#endif // #ifndef TIME_MANAGER_H
