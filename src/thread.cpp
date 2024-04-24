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
      m_nodes_counter(0), m_time_manager(), m_max_depth(max_depth) {}

void Thread::stop_search() {
  if (!m_stop) {
    m_stop = true;
    m_thread.join();
  }
}

void Thread::search(Position &position) {
  m_thread = std::thread(search::progressive_deepening, std::ref(position),
                         std::ref(*this));
}
