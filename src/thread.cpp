/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "thread.hpp"
#include "move_generation.hpp"
#include "position.hpp"

void Thread::stop_search() {
  if (!m_stop) {
    m_stop = true;
    m_thread.join();
  }
}

void Thread::search(Position &position) {
  m_thread = std::thread(move_generation::progressive_deepening,
                         std::ref(position), std::ref(*this));
}
