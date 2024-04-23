/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "uci.hpp"
#include "move_generation.hpp"
#include <iostream>
#include <sstream>
#include <string>

UCI::UCI(int argc, char *argv[]) {
  // TODO built object.
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
      m_engine.stop();
    } else if (token == "go") {
      m_engine.go();
    } else if (token == "position") {
      position(iss);
    } else if (token == "ucinewgame") {
      m_engine.reset();
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
  // TODO set up positions.
}

void UCI::set_option(std::istringstream &iss) {
  // TODO
}
