/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "uci.h"
#include <iostream>
#include <ostream>
#include <sstream>

UCI::UCI(int argc, char *argv[]) {
  // TODO built object.
}

void UCI::loop() {
  std::cout << "Minke Chess Engine by Eduardo Marinho" << std::endl;

  std::istringstream iss;
  std::string token;
  do {
    // TODO fix input entry
    token.clear();
    iss >> std::skipws >> token;
    // TODO check for command "d"
    if (token == "quit" || token == "stop") {
      // stop thread of the search and print best movement found so far
    } else if (token == "go") {
      go();
    } else if (token == "position") {
      position();
    } else if (token == "ucinewgame") {
      reset();
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
    } else if (!token.empty()) {
      std::cout << "Unknown command: '" << token
                << "'. Type help for information." << std::endl;
    }
  } while (token != "quit");
}

void UCI::go() {
  // TODO start move search.
}

void UCI::eval() {
  std::cout << "The position evaluation is " << m_position.evaluate()
            << std::endl;
}

void UCI::reset() {
  // TODO reset position
}

void UCI::position() {
  // TODO set up positions.
}

void UCI::set_option(std::istringstream &is) {
  // TODO
}
