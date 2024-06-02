/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "position.hpp"
#include "thread.hpp"
#include <cstdint>
#include <sstream>
#include <vector>

class Engine {
public:
  Engine(uint8_t max_depth, uint64_t node_limit, int hash_size);
  ~Engine() = default;
  void go();
  void stop();
  void eval();
  void wait();
  void set_position(const std::string &fen,
                    const std::vector<std::string> &move_list);
  void set_option(std::istringstream &iss);

private:
  Thread m_thread;
  Position m_position;
};

#endif // #ifndef ENGINE_HPP
