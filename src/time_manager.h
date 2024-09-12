/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include "game_elements.h"
class TimeManager {
  public:
    TimeManager();
    ~TimeManager() = default;

    void init();
    bool nodes_over(const CounterType &nodes);
    bool stop_early();
    bool time_over();

  private:
    TimeType m_start_time;
    TimeType m_optimum_time;
    TimeType m_maximum_time;

    CounterType m_node_limit;
    bool m_node_set;
    bool m_time_set;
};

#endif // #ifndef TIME_MANAGER_H
