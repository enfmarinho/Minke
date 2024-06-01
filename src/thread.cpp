/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "thread.hpp"
#include "position.hpp"
#include "search.hpp"
#include <cstdint>

Thread::Thread(uint8_t max_depth)
    : m_stop(false), m_nodes_searched(0), m_game_ply(0), m_search_ply(0),
      m_nodes_counter(0), m_max_depth(max_depth) {}

void Thread::stop_search() {
  if (!m_stop) {
    m_stop = true;
    m_thread.join();
  }
}

bool Thread::should_stop() {
  // TODO checks for stop scenarios, such as time management
  return m_stop;
}

void Thread::wait() { m_thread.join(); }

void Thread::search(Position &position) {
  m_stop = false;
  m_thread = std::thread(search::iterative_deepening, std::ref(position),
                         std::ref(*this));
}

void Thread::max_depth(uint8_t new_max_depth) { m_max_depth = new_max_depth; }
