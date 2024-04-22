/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef THREAD_HPP
#define THREAD_HPP

#include "hashing.hpp"
#include "time_manager.hpp"
#include <cstdint>
#include <thread>

class Thread {
public:
  void init(HashType hash) { m_hash = hash; /* TODO */ }
  void stop_search() {
    m_stop = true;
    m_thread.join();
  }
  void search();
  std::thread *get() { return &m_thread; }

private:
  std::thread m_thread;
  bool m_stop{false};
  HashType m_hash;
  uint64_t m_nodes_searched{0};
  uint8_t m_game_ply{0};
  uint8_t m_search_ply{0};
  uint64_t m_nodes_counter{0};
  TimeManager m_time_manager;
};

// TODO implement a thread pool
// class ThreadPool {
// private:
//   std::vector<Thread *> m_threads;
// };

#endif // #ifndef THREAD_HPP
