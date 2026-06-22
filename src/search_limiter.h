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

#ifndef SEARCH_LIMITER_H
#define SEARCH_LIMITER_H

#include <cstddef>

#include "types.h"

struct ThreadData;

constexpr size_t NO_NODE_LIMIT = std::numeric_limits<size_t>::max();
constexpr TimeType NO_TIME_LIMIT = std::numeric_limits<TimeType>::max();

struct SearchLimits {
    size_t optimum_node{NO_NODE_LIMIT};
    size_t maximum_node{NO_NODE_LIMIT};
    CounterType depth{MAX_SEARCH_DEPTH - 1};

    CounterType time{0};
    CounterType movestogo{0};
    CounterType movetime{0};
    CounterType inc{0};

    bool infinite{true};
};

class SearchLimiter {
  public:
    void reset(const SearchLimits& limits);
    void update(const ThreadData& td);

    [[nodiscard]] CounterType depth_limit() const;
    [[nodiscard]] bool stop_early(const ThreadData& td) const;
    [[nodiscard]] bool exceeded(const ThreadData& td) const;

    void allow_stopping();

    TimeType time_passed() const;

  private:
    bool m_movetime_set{false};
    bool m_can_stop{false}; // Prevent stopping before depth 1 completes

    // Time tracking
    double m_scale{1.0};
    TimeType m_start_time{0};
    TimeType m_optimum_time{NO_TIME_LIMIT};
    TimeType m_maximum_time{NO_TIME_LIMIT};

    // Node tracking
    size_t m_optimum_nodes{NO_NODE_LIMIT};
    size_t m_maximum_nodes{NO_NODE_LIMIT};
    CounterType m_depth_limit{MAX_SEARCH_DEPTH - 1};
};

#endif // #ifndef SEARCH_LIMITER_H
