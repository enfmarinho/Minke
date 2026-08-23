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
#include <sstream>
#include <thread>

#include "core/position.h"
#include "core/types.h"
#include "search/search.h"

namespace EngineOptions {
static constexpr CounterType HASH_DEFAULT = 16;
static constexpr CounterType HASH_MIN = 1;
static constexpr CounterType HASH_MAX = 2097152;
static constexpr CounterType THREADS_DEFAULT = 1;
static constexpr CounterType THREADS_MIN = 1;
static constexpr CounterType THREADS_MAX = 2048;
void print();
} // namespace EngineOptions

namespace UCI {

void run();

class UciHandler {
  public:
    UciHandler();
    ~UciHandler() = default;
    void run();

  private:
    void position(std::istringstream &);
    void set_position(const std::string &fen, const std::vector<std::string> &move_list);
    void ucinewgame();

    void set_option(std::istringstream &);

    /// Returns perft depth or 0 if should not perft
    CounterType parse_go(std::istringstream &, bool bench = false);
    int64_t perft(Position &position, CounterType depth, bool root = true);
    void go();

    void print_debug_info();
    void eval();

    std::thread m_thread;
    Position m_pos;
    Engine m_engine;
};

} // namespace UCI
