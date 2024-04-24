/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef THREAD_HPP
#define THREAD_HPP

#include "position.hpp"
#include "time_manager.hpp"
#include <cstdint>
#include <thread>

class Thread {
public:
  Thread(uint8_t max_depth);
  void stop_search();
  bool should_stop() {
    return m_stop; // TODO remove this, just a STUB
  }
  void wait() { m_thread.join(); }
  void search(Position &position);
  void max_depth(uint8_t new_max_depth) { m_max_depth = new_max_depth; }

private:
  std::thread m_thread;
  bool m_stop;
  uint64_t m_nodes_searched;
  uint8_t m_game_ply;
  uint8_t m_search_ply;
  uint64_t m_nodes_counter;
  TimeManager m_time_manager;
  uint8_t m_max_depth;
};

// TODO implement a thread pool
// class ThreadPool {
// private:
//   std::vector<Thread *> m_threads;
// };

#endif // #ifndef THREAD_HPP
