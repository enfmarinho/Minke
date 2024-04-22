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
#include <vector>

class Engine {
public:
  void go();
  void stop();
  void reset();
  void set_position(const std::string &fen,
                    const std::vector<std::string> &move_list);

private:
  Thread m_thread;
  Position m_position;
};

#endif // #ifndef ENGINE_HPP
