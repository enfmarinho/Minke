/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef THREAD_HPP
#define THREAD_HPP

#include "time_manager.hpp"
#include <cstdint>
#include <thread>

class Thread {
public:
  void stop_search() {
    if (!m_stop) {
      m_stop = true;
      m_thread.join();
    }
  }
  bool should_stop() {
    return m_stop; // TODO remove this, just a STUB
  }
  void search();
  std::thread *get() { return &m_thread; }
  void max_depth(uint8_t new_max_depth) { m_max_depth = new_max_depth; }

private:
  std::thread m_thread;
  bool m_stop{false};
  uint64_t m_nodes_searched{0};
  uint8_t m_game_ply{0};
  uint8_t m_search_ply{0};
  uint64_t m_nodes_counter{0};
  TimeManager m_time_manager;
  uint8_t m_max_depth;
};

// TODO implement a thread pool
// class ThreadPool {
// private:
//   std::vector<Thread *> m_threads;
// };

#endif // #ifndef THREAD_HPP
