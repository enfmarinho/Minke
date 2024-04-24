/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "engine.hpp"
#include "evaluation.hpp"
#include "game_elements.hpp"
#include "move_generation.hpp"
#include "transposition_table.hpp"
#include <functional>
#include <iostream>
#include <sstream>
#include <thread>

Engine::Engine(uint8_t max_depth, int hash_size) {
  m_thread.max_depth(max_depth);
  TranspositionTable::get().resize(hash_size);
}

void Engine::go() {
  *m_thread.get() = std::thread(move_generation::progressive_deepening,
                                std::ref(m_position), std::ref(m_thread));
}

void Engine::stop() { m_thread.stop_search(); }

void Engine::eval() {
  std::cout << "The position evaluation is " << eval::evaluate(m_position)
            << std::endl;
}

void Engine::wait() { m_thread.get()->join(); }

void Engine::set_position(const std::string &fen,
                          const std::vector<std::string> &move_list) {
  // TODO
  TranspositionTable::get().clear();
}

void Engine::set_option(std::istringstream &iss) {
  std::string token, garbage;
  int value;
  iss >> garbage; // Consume the "name" token
  iss >> token;
  iss >> garbage; // Consume the "value" token.
  iss >> value;
  if (token == "MaxDepth") {
    m_thread.max_depth(value);
  } else if (token == "Hash") {
    TranspositionTable::get().resize(value);
  }
}
