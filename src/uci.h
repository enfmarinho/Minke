/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef UCI_H
#define UCI_H

#include "game_elements.h"
#include "game_state.h"
#include "position.h"
#include "thread.h"
#include <cstdint>
#include <ostream>
#include <sstream>

class UCI {
public:
  UCI() = delete;
  UCI(int argc, char *argv[]);
  ~UCI() = default;
  void loop();

private:
  struct EngineOptions {
    static constexpr CounterType hash_default = 16;
    static constexpr CounterType hash_min = 1;
    static constexpr CounterType hash_max = 131072;
    friend std::ostream &operator<<(std::ostream &os, const EngineOptions &eo) {
      os << "option name Hash type spin default " << hash_default << " min "
         << hash_min << " max " << hash_max << "\n";
      return os;
    }
  };

  void position(std::istringstream &);
  void set_position(const std::string &fen,
                    const std::vector<std::string> &move_list);

  void set_option(std::istringstream &);

  /// Returns true if perft argument was passed and false otherwise.
  bool parse_go(std::istringstream &, bool bench = false);
  int64_t perft(Position &position, CounterType depth, bool root = true);
  void go();

  void print_debug_info() const;
  void bench(std::istringstream &);
  void eval();

  EngineOptions m_engine_options;
  Thread m_thread;
  GameState m_game_state;
};

#endif // #ifndef UCI_H
