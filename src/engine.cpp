#include "engine.hpp"
#include "evaluation.hpp"
#include "move_generation.hpp"
#include "transposition_table.hpp"
#include <functional>
#include <iostream>
#include <thread>

void Engine::go() {
  *m_thread.get() =
      std::thread(move_generation::progressive_deepening, std::ref(m_position));
}

void Engine::reset() {
  m_position.reset();
  TranspositionTable::get().clear();
}

void Engine::stop() {
  m_thread.stop_search();
  m_thread.get()->join();
}

void Engine::eval() {
  std::cout << "The position evaluation is " << eval::evaluate(m_position)
            << std::endl;
}

void Engine::set_position(const std::string &fen,
                          const std::vector<std::string> &move_list) {
  // TODO
}
