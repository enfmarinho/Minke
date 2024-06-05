/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef THREAD_HPP
#define THREAD_HPP

#include "game_elements.hpp"
#include "position.hpp"
#include <cstdint>
#include <thread>

class Thread {
public:
  Thread(uint8_t max_depth, uint64_t node_limit);
  ~Thread() = default;

  void reset();
  void stop_search();
  bool should_stop() const;
  bool should_stop(CounterType depth) const;
  void wait();
  void search(Position &position);
  void max_depth_ply(CounterType new_max_depth_ply);
  void movetime(CounterType mivetime);
  void node_limit(uint64_t new_node_limit);
  void increase_nodes_searched_counter();
  void infinite();

private:
  std::thread m_thread;
  bool m_stop;
  bool m_infinite;
  uint64_t m_nodes_searched;
  uint64_t m_node_limit;
  CounterType m_max_depth;
  TimePoint m_start_time;
  TimePoint m_movetime;
};

#endif // #ifndef THREAD_HPP
