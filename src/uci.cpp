/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "uci.hpp"
#include "evaluation.hpp"
#include "game_elements.hpp"
#include "search.hpp"
#include "transposition_table.hpp"
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

UCI::UCI(int argc, char *argv[]) {
  TranspositionTable::get().resize(EngineOptions::hash_default);
}

void UCI::loop() {
  std::cout << "Minke Chess Engine by Eduardo Marinho" << std::endl;

  std::string input, token;
  do {
    if (!std::getline(std::cin, input)) {
      input = "quit";
    }
    std::istringstream iss(input);

    token.clear();
    iss >> std::skipws >> token;
    if (token == "quit" || token == "stop") {
      m_thread.stop_search();
    } else if (token == "go") {
      parse_go_limits(iss);
      go();
    } else if (token == "position") {
      position(iss);
    } else if (token == "ucinewgame") {
      set_position(StartFEN, std::vector<std::string>());
    } else if (token == "setoption") {
      set_option(iss);
    } else if (token == "eval") {
      eval();
    } else if (token == "uci") {
      std::cout << "id name Minke 0.0.1 \n"
                << "id author Eduardo Marinho \n"
                << m_engine_options << "\n"
                << "uciok" << std::endl;
    } else if (token == "isready") {
      std::cout << "readyok" << std::endl;
    } else if (token == "help" || token == "--help") {
      std::cout << "TODO write help message." << std::endl;
    } else if (token == "d") {
      // TODO Display the current position, with ASCII art and FEN.
    } else if (token == "bench") {
      // TODO run benchmark
    } else if (!token.empty()) {
      std::cout << "Unknown command: '" << token
                << "'. Type help for information." << std::endl;
    }
  } while (token != "quit");
}

void UCI::position(std::istringstream &iss) {
  std::string token, fen, move;
  iss >> token;
  if (token == "startpos") {
    fen = StartFEN;
    iss >> move; // consume the "moves" token, if there is one.
  } else if (token == "fen") {
    while (iss >> token && token != "moves") {
      fen += token + " ";
    }
  } else {
    return;
  }

  std::vector<std::string> move_list;
  while (iss >> move) {
    move_list.push_back(move);
  }
  set_position(fen, move_list);
}

void UCI::set_position(const std::string &fen,
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

void UCI::set_option(std::istringstream &iss) {
  m_thread.wait();
  std::string token, garbage;
  int value;
  iss >> garbage; // Consume the "name" token
  iss >> token;
  iss >> garbage; // Consume the "value" token.
  iss >> value;
  // TODO check if values are valid
  if (token == "Hash") {
    TranspositionTable::get().resize(value);
  }
}

void UCI::eval() {
  std::cout << "The position evaluation is " << eval::evaluate(m_position)
            << std::endl;
}

void UCI::parse_go_limits(std::istringstream &iss) {
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

void UCI::go() { m_thread.search(m_position); }
