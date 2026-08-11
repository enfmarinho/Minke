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

#pragma once

#include <cstdint>
#include <optional>

#include "core/types.h"

struct ThreadData;

struct SearchLimits {
    std::optional<uint64_t> time_remaining;
    std::optional<uint64_t> time_increment;
    std::optional<uint64_t> movetime;
    std::optional<uint64_t> mtg;

    std::optional<uint64_t> optimum_node;
    std::optional<uint64_t> maximum_node;
    std::optional<int> depth;

    std::optional<bool> infinite;
};

class SearchLimiter {
  public:
    SearchLimiter();
    ~SearchLimiter() = default;

    void init();
    void init(const SearchLimits& sl);
    void update(const ThreadData& td, CounterType pv_stability, CounterType score_stability);

    bool stop_early(uint64_t nodes) const;
    bool time_over(uint64_t nodes) const;
    CounterType max_depth() const;

    TimeType time_passed() const;
    void can_stop();

  private:
    TimeType m_start_time;
    TimeType m_optimum_time;
    TimeType m_maximum_time;
    double m_scale;

    uint64_t m_optimum_nodes;
    uint64_t m_maximum_nodes;

    CounterType m_max_depth;

    bool m_movetime;
    bool m_time_set;
    bool m_can_stop;
};
