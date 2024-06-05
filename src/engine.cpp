/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "engine.hpp"
#include "evaluation.hpp"
#include "game_elements.hpp"
#include "search.hpp"
#include "transposition_table.hpp"
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>

Engine::Engine(uint8_t max_depth, uint64_t node_limit, int hash_size)
    : m_thread(max_depth, node_limit) {
  TranspositionTable::get().resize(hash_size);
}

void Engine::parse_go_limits(std::istringstream &iss) {
  std::string token;
  while (iss >> token) {
    if (token == "infinite") {
      m_thread.infinite();
      return;
    }

    CounterType option;
    iss >> option;
    if (token == "depth")
      m_thread.max_depth_ply(option);
    else if (token == "nodes")
      m_thread.node_limit(option);
    else if (token == "movetime")
      m_thread.movetime(option);
  }
}

void Engine::go() { m_thread.search(m_position); }

void Engine::stop() { m_thread.stop_search(); }

void Engine::eval() {
  std::cout << "The position evaluation is " << eval::evaluate(m_position)
            << std::endl;
}

void Engine::wait() { m_thread.wait(); }

void Engine::set_position(const std::string &fen,
                          const std::vector<std::string> &move_list) {
  if (!m_position.reset(fen)) {
    std::cerr
        << "Since the FEN string was invalid, the board representation may "
           "have been be corrupted. To be safe, set the game board again!"
        << std::endl;
    return;
  }
  TranspositionTable::get().clear();
  for (const std::string &algebraic_notation : move_list) {
    m_position.move(m_position.get_movement(algebraic_notation));
  }
}

void Engine::set_option(std::istringstream &iss) {
  std::string token, garbage;
  int value;
  iss >> garbage; // Consume the "name" token
  iss >> token;
  iss >> garbage; // Consume the "value" token.
  iss >> value;
  if (token == "MaxDepth") {
    m_thread.max_depth_ply(value);
  } else if (token == "Hash") {
    TranspositionTable::get().resize(value);
  }
}
