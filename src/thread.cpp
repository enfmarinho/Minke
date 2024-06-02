/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "thread.hpp"
#include "game_elements.hpp"
#include "position.hpp"
#include "search.hpp"
#include <cstdint>

Thread::Thread(uint8_t max_depth, uint64_t node_limit)
    : m_stop(true), m_nodes_searched(0), m_depth_ply_searched(0),
      m_node_limit(node_limit), m_max_depth(max_depth) {}

void Thread::stop_search() {
  if (!m_stop) {
    m_stop = true;
    m_thread.join();
  }
}

bool Thread::should_stop() const {
  // TODO do the time management.
  return m_stop && m_nodes_searched > m_node_limit;
}

bool Thread::should_stop(CounterType depth) const {
  return should_stop() && depth > m_max_depth;
}

void Thread::wait() { m_thread.join(); }

void Thread::search(Position &position) {
  m_stop = false;
  m_thread = std::thread(search::iterative_deepening, std::ref(position),
                         std::ref(*this));

void Thread::max_depth(CounterType new_max_depth) {
  m_max_depth = new_max_depth;
}

void Thread::node_limit(uint64_t new_node_limit) {
  m_node_limit = new_node_limit;
}

void Thread::increase_nodes_searched_counter() { ++m_nodes_searched; }
