/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef UCI_H
#define UCI_H

#include <cstdint>
#include <sstream>
#include <thread>

#include "position.h"
#include "search.h"
#include "types.h"

namespace EngineOptions {
constexpr CounterType BenchDepth = 8;
static constexpr CounterType hash_default = 16;
static constexpr CounterType hash_min = 1;
static constexpr CounterType hash_max = 32768;
void print();
} // namespace EngineOptions

class UCI {
  public:
    UCI() = delete;
    UCI(int argc, char *argv[]);
    ~UCI() = default;
    void loop();

  private:
    void position(std::istringstream &);
    void set_position(const std::string &fen, const std::vector<std::string> &move_list);

    void set_option(std::istringstream &);

    /// Returns true if perft argument was passed and false otherwise.
    bool parse_go(std::istringstream &, bool bench = false);
    int64_t perft(Position &position, CounterType depth, bool root = true);
    void go();

    void print_debug_info();
    void bench();
    void eval();

    std::thread m_thread;
    SearchData m_search_data;
};

#endif // #ifndef UCI_H
