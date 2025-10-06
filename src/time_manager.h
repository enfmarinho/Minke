/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include "move.h"
#include "types.h"

struct ThreadData;

class TimeManager {
  public:
    TimeManager();
    ~TimeManager() = default;

    void reset(CounterType inc, CounterType time, CounterType movestogo, CounterType movetime, bool infinite);
    void reset();
    void update(const ThreadData &td, const Move &best_move, const int &depth);
    bool stop_early() const;
    bool time_over() const;
    TimeType time_passed() const;
    void can_stop();

  private:
    TimeType m_start_time;
    TimeType m_optimum_base_time;
    TimeType m_optimum_time;
    TimeType m_maximum_time;

    Move prev_best_move;
    int move_stability_factor;

    bool m_movetime;
    bool m_time_set;
    bool m_can_stop;
};

#endif // #ifndef TIME_MANAGER_H
