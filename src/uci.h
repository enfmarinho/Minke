/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef UCI_H
#define UCI_H

#include "position.h"
#include "thread.h"
#include "transposition_table.h"
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
    static const int max_depth_default = 16;
    static const int max_depth_min = 1;
    static const int max_depth_max = 16;

    static const int threads_default = 1;
    static const int threads_min = 1;
    static const int threads_max = 1024;

    static const int hash_default = 16;
    static const int hash_min = 1;
    static const int hash_max = 131072;
    int max_depth;
    int threads;
    int hash;
    EngineOptions()
        : max_depth(max_depth_default), threads(threads_default),
          hash(hash_default) {}
    friend std::ostream &operator<<(std::ostream &os, const EngineOptions &eo) {
      os << "option name MaxDepth type spin default " << max_depth_default
         << " min " << max_depth_min << " max " << max_depth_max << "\n";
      os << "option name Threads type spin default " << threads_default
         << " min " << threads_min << " max " << threads_max << "\n";
      os << "option name Hash type spin default " << hash_default << " min "
         << hash_min << " max " << hash_max << "\n";
      return os;
    }
  };

  void go();
  void eval();
  void reset();
  void position();
  void set_option(std::istringstream &is);

  TranspositionTable m_transposition_table;
  EngineOptions m_engine_options;
  ThreadPool m_threads;
  Position m_position;
};

#endif // #ifndef UCI_H
