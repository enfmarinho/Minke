/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "thread.h"
#include "game_elements.h"
#include "game_state.h"
#include "search.h"
#include <chrono>
#include <cstdint>
#include <limits>

Thread::Thread() { reset(); }

void Thread::reset() {
  m_stop = true;
  m_infinite = false;
  m_nodes_searched = 0;
  m_node_limit = m_max_depth = std::numeric_limits<CounterType>::max();
  m_movetime = std::numeric_limits<TimeType>::max();
}

void Thread::stop_search() {
  if (!m_stop) {
    reset();
    m_thread.join();
  }
}

bool Thread::should_stop() const {
  return m_stop || (!m_infinite && (m_nodes_searched >= m_node_limit ||
                                    now() - m_start_time >= m_movetime));
}

bool Thread::should_stop(CounterType depth) const {
  return m_stop || should_stop() || (!m_infinite && depth > m_max_depth);
}

void Thread::wait() {
  if (!m_stop) {
    m_thread.join();
    m_stop = true;
  }
}

void Thread::search(GameState &game_state) {
  if (m_stop) {
    m_stop = false;
    m_start_time = now();
    m_thread =
        std::thread(search::search, std::ref(game_state), std::ref(*this));
  }
}

void Thread::movetime(CounterType movetime) {
  m_movetime = std::chrono::milliseconds(movetime).count();
}

void Thread::max_depth_ply(CounterType new_max_depth) {
  m_max_depth = new_max_depth;
}

void Thread::node_limit(uint64_t new_node_limit) {
  m_node_limit = new_node_limit;
}

void Thread::increase_nodes_searched_counter() { ++m_nodes_searched; }

uint64_t Thread::nodes_searched() { return m_nodes_searched; }

void Thread::infinite() { m_infinite = true; }
