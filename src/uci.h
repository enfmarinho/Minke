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

#ifndef UCI_H
#define UCI_H

#include <cstdint>
#include <sstream>
#include <thread>

#include "game_state.h"
#include "search.h"
#include "types.h"

namespace EngineOptions {
constexpr CounterType BENCH_DEPTH = 12;
static constexpr CounterType HASH_DEFAULT = 16;
static constexpr CounterType HASH_MIN = 1;
static constexpr CounterType HASH_MAX = 2097152;
static constexpr CounterType THREADS_DEFAULT = 1;
static constexpr CounterType THREADS_MIN = 1;
static constexpr CounterType THREADS_MAX = 1;
void print();
} // namespace EngineOptions

class UCI {
  public:
    UCI();
    ~UCI();
    void loop();
    void bench(int depth);

  private:
    void position(std::istringstream &);
    void set_position(const std::string &fen, const std::vector<std::string> &move_list);
    void ucinewgame();

    void set_option(std::istringstream &);

    /// Returns true if perft argument was passed and false otherwise.
    bool parse_go(std::istringstream &, bool bench = false);
    int64_t perft(GameState &position, CounterType depth, bool root = true);
    void go();

    void print_debug_info();
    void eval();

    std::thread m_thread;
    ThreadData *m_td;
};

#endif // #ifndef UCI_H
