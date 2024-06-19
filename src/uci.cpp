/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "uci.h"
#include "evaluate.h"
#include "game_elements.h"
#include "movegen.h"
#include "search.h"
#include "tt.h"
#include <cassert>
#include <cstdint>
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
      m_thread.wait();
      m_thread.reset();
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
      m_game_state.position().print();
      bool found;
      auto entry =
          TranspositionTable::get().probe(m_game_state.position(), found);
      Move ttmove = move_none;
      if (found)
        ttmove = entry->best_move();
      MoveList move_list(m_game_state, ttmove);
      std::cout << "Move list (" << move_list.size()
                << " pseudo-legal moves): ";
      while (!move_list.empty()) {
        Move move = move_list.next_move();
        Position copy = m_game_state.position(); // Avoid update the NN
        if (copy.move(move))
          std::cout << move.get_algebraic_notation() << ' ';
        else
          std::cout << "(" << move.get_algebraic_notation() << ") ";
      }
      std::cout << std::endl;
    } else if (token == "bench") {
      bench();
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
  if (!m_game_state.reset(fen)) {
    std::cerr << "Since the FEN string was invalid, the game state "
                 "representation may have been be corrupted. To be safe, set "
                 "the game board again!"
              << std::endl;
    return;
  }
  TranspositionTable::get().clear();
  for (const std::string &algebraic_notation : move_list) {
    assert(m_game_state.make_move(
        m_game_state.position().get_movement(algebraic_notation)));
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

void UCI::bench() {
  m_thread.wait();
  m_thread.reset();
  m_thread.max_depth_ply(8); // TODO make search limits to be defined by args
  TranspositionTable::get().clear();
  TimeType start_time = now();

  std::vector<std::string> fen_list = {StartFEN};
  for (const std::string &fen : fen_list) {
    m_game_state.reset(fen);
    go();
  }
  m_thread.wait();
  TimeType time_taken = now() - start_time;

  std::cerr << "\n==========================\n";
  std::cerr << "Total time: " << time_taken << "ms\n";
  std::cerr << "Nodes searched: " << m_thread.nodes_searched() << "\n";
  std::cerr << "Nodes per second: "
            << m_thread.nodes_searched() * 1000 / time_taken;
  std::cerr << "\n==========================";
  std::cerr << std::endl;
}

void UCI::eval() {
  std::cout << "The position evaluation is "
            << eval::evaluate(m_game_state.position()) << std::endl;
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

void UCI::go() { m_thread.search(m_game_state); }
