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

#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include "types.h"

struct ThreadData;

class TimeManager {
  public:
    TimeManager();
    ~TimeManager() = default;

    void reset(CounterType inc, CounterType time, CounterType movestogo, CounterType movetime, bool infinite);
    void reset();
    void update(const ThreadData &td);
    bool stop_early() const;
    bool time_over() const;
    TimeType time_passed() const;
    void can_stop();

  private:
    TimeType m_start_time;
    TimeType m_optimum_time;
    TimeType m_maximum_time;

    double m_scale;
    bool m_movetime;
    bool m_time_set;
    bool m_can_stop;
};

#endif // #ifndef TIME_MANAGER_H
