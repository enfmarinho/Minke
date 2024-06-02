/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef UCI_HPP
#define UCI_HPP

#include "engine.hpp"
#include "game_elements.hpp"
#include <cstdint>
#include <limits>
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
    static constexpr CounterType max_depth_default = 16;
    static constexpr CounterType max_depth_min = 1;
    static constexpr CounterType max_depth_max = 16;

    static constexpr uint64_t node_limit_default =
        std::numeric_limits<uint64_t>::max();
    static constexpr uint64_t node_limit_min = 1;
    static constexpr uint64_t node_limit_max =
        std::numeric_limits<uint64_t>::max();

    static constexpr CounterType threads_default = 1;
    static constexpr CounterType threads_min = 1;
    static constexpr CounterType threads_max = 1024;

    static constexpr CounterType hash_default = 16;
    static constexpr CounterType hash_min = 1;
    static constexpr CounterType hash_max = 131072;
    friend std::ostream &operator<<(std::ostream &os, const EngineOptions &eo) {
      os << "option name MaxDepth type spin default " << max_depth_default
         << " min " << max_depth_min << " max " << max_depth_max << "\n";
      os << "option name NodeLimit type spin default " << node_limit_default
         << " min " << node_limit_min << " max " << node_limit_max << "\n";
      os << "option name Hash type spin default " << hash_default << " min "
         << hash_min << " max " << hash_max << "\n";
      return os;
    }
  };

  void position(std::istringstream &);
  void set_option(std::istringstream &);

  Engine m_engine;
  EngineOptions m_engine_options;
};

#endif // #ifndef UCI_HPP
