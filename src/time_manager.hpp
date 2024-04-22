/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef TIME_MANAGER_HPP
#define TIME_MANAGER_HPP

#include <chrono>

class TimeManager {
  using TimeType = std::chrono::steady_clock::time_point;

  void reset() { /* TODO  */ }

  TimeType maximum() const { return m_maximum_time; }
  TimeType optimum() const { return m_optimum_time; }
  TimeType elapsed() const { /* TODO */ }

private:
  TimeType m_maximum_time;
  TimeType m_optimum_time;
  TimeType m_start_time;
};

#endif // #endif TIME_MANAGER_HPP
