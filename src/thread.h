/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef THREAD_H
#define THREAD_H

#include "game_elements.h"
#include "game_state.h"
#include <cstdint>
#include <thread>

class Thread {
public:
  Thread();
  ~Thread() = default;

  void reset();
  void stop_search();
  bool should_stop() const;
  bool should_stop(CounterType depth) const;
  void wait();
  void search(GameState &game_state);
  CounterType max_depth_ply() const;
  void max_depth_ply(CounterType new_max_depth_ply);
  void node_limit(uint64_t new_node_limit);
  void increase_nodes_searched_counter();
  void infinite();
  TimeType time_passed() const;
  uint64_t nodes_searched() const;
  void set_search_time(TimeType search_time);

private:
  std::thread m_thread;
  bool m_stop;
  bool m_infinite;
  uint64_t m_nodes_searched;
  uint64_t m_node_limit;
  CounterType m_max_depth;
  TimeType m_start_time;
  TimeType m_search_time;
};

#endif // #ifndef THREAD_H
