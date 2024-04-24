/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "uci.hpp"
#include "engine.hpp"
#include "game_elements.hpp"
#include "move_generation.hpp"
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

UCI::UCI(int argc, char *argv[])
    : m_engine(EngineOptions::max_depth_default, EngineOptions::hash_default) {}

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
      m_engine.stop();
    } else if (token == "go") {
      m_engine.go();
    } else if (token == "position") {
      position(iss);
    } else if (token == "ucinewgame") {
      m_engine.set_position(StartFEN, std::vector<std::string>());
    } else if (token == "setoption") {
      set_option(iss);
    } else if (token == "eval") {
      m_engine.eval();
    } else if (token == "uci") {
      std::cout << "id name Minke 0.0.1 \n"
                << "id author Eduardo Marinho \n"
                << m_engine_options << "\n"
                << "uciok" << std::endl;
    } else if (token == "isready") {
      std::cout << "readyok" << std::endl;
    } else if (token == "help" || token == "--help") {
      std::cout << "TODO write help message." << std::endl;
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

  std::vector<std::string> movements;
  while (iss >> move) {
    movements.push_back(move);
  }

  m_engine.set_position(fen, movements);
}

void UCI::set_option(std::istringstream &iss) {
  m_engine.wait();
  m_engine.set_option(iss);
}
