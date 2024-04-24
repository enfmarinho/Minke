/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef UCI_HPP
#define UCI_HPP

#include "engine.hpp"
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
    static constexpr int max_depth_default = 16;
    static constexpr int max_depth_min = 1;
    static constexpr int max_depth_max = 16;

    static constexpr int threads_default = 1;
    static constexpr int threads_min = 1;
    static constexpr int threads_max = 1024;

    static constexpr int hash_default = 16;
    static constexpr int hash_min = 1;
    static constexpr int hash_max = 131072;
    friend std::ostream &operator<<(std::ostream &os, const EngineOptions &eo) {
      os << "option name MaxDepth type spin default " << max_depth_default
         << " min " << max_depth_min << " max " << max_depth_max << "\n";
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