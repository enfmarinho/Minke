/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef UCI_H
#define UCI_H

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <thread>

#include "position.h"
#include "search.h"
#include "types.h"

namespace EngineOptions {
constexpr CounterType BENCH_DEPTH = 12;
static constexpr CounterType HASH_DEFAULT = 16;
static constexpr CounterType HASH_MIN = 1;
static constexpr CounterType HASH_MAX = 2097152;
static constexpr CounterType THREADS_DEFAULT = 1;
static constexpr CounterType THREADS_MIN = 1;
static constexpr CounterType THREADS_MAX = 1024;
void print();
} // namespace EngineOptions

class UCI {
  public:
    UCI() = delete;
    UCI(int argc, char *argv[]);
    ~UCI();
    void loop();

  private:
    void position(std::istringstream &);
    void set_position(const std::string &fen, const std::vector<std::string> &move_list);
    void ucinewgame();

    void set_option(std::istringstream &);

    /// Returns true if perft argument was passed and false otherwise.
    bool parse_go(std::istringstream &, bool bench = false);
    int64_t perft(Position &position, CounterType depth, bool root = true);
    void go();

    void print_debug_info();
    void bench(int depth);
    void eval();

    size_t m_num_threads; //!< Number of searcher threads to use
    std::thread m_thread;
    ThreadData *m_td;
};

#endif // #ifndef UCI_H
