/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "uci.h"
#include "evaluation.h"
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
      // TODO stop search thread and print best movement found so far
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
  std::cout << "The position evaluation is " << eval::evaluate(m_position)
            << std::endl;
}

void UCI::reset() {
  m_position.reset();
  // TODO reset transposition table
}

void UCI::position() {
  // TODO set up positions.
}

void UCI::set_option(std::istringstream &is) {
  // TODO
}
