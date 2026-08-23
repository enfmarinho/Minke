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

namespace UCI {

void run();

class UciHandler {
  public:
    UciHandler();
    ~UciHandler() = default;

    void run();

  private:
    ///=== standard UCI commands
    void handle_isready();
    void handle_uci();
    void handle_position(std::istringstream &);
    void handle_go(std::istringstream &);
    void handle_ucinewgame();
    void handle_setoption(std::istringstream &);
    ///===

    ///=== non-standard UCI commands
    void handle_bench(std::istringstream &);
    void handle_perft(std::istringstream &);
    void handle_tuneinfo();
    void handle_debug();
    void handle_eval();
    ///===

    void set_position(const std::string &fen, const std::vector<std::string> &move_list);

    /// Returns perft depth or 0 if should not perft
    CounterType parse_go(std::istringstream &, bool bench = false);
    int64_t perft(Position &position, CounterType depth, bool root = true);
    void go();

    bool stopped();

    std::thread m_thread;
    Position m_pos;
    Engine m_engine;
};

} // namespace UCI
