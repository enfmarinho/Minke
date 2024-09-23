/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "uci.h"

#include <cassert>
#include <cstdint>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "benchmark.h"
#include "game_elements.h"
#include "movegen.h"
#include "position.h"
#include "search.h"
#include "tt.h"

UCI::UCI(int argc, char *argv[]) {
    TranspositionTable::get().resize(EngineOptions::hash_default);
    m_search_data.reset();
    if (argc > 1 && std::string(argv[1]) == "bench") {
        if (argc > 2)
            m_search_data.depth_limit = std::stoi(argv[2]);
        else
            m_search_data.depth_limit = EngineOptions::BenchDepth;

        bench();
        exit(0);
    }
}

void UCI::loop() {
    std::cout << "Minke Chess Engine by Eduardo Marinho" << std::endl;

    std::string input, token;
    do {
        if (!std::getline(std::cin, input))
            input = "quit";
        std::istringstream iss(input);

        token.clear();
        iss >> std::skipws >> token;
        if (token == "quit" || token == "stop") {
            m_search_data.stop = true;
        } else if (token == "go") {
            if (!m_search_data.stop)
                continue;
            else if (m_thread.joinable())
                m_thread.join();
            m_search_data.reset();
            if (parse_go(iss))
                perft(m_search_data.game_state.top(), m_search_data.depth_limit);
            else
                go();
        } else if (token == "position") {
            position(iss);
        } else if (token == "ucinewgame") {
            set_position(StartFEN, std::vector<std::string>());
        } else if (token == "setoption") {
            if (!m_search_data.stop) {
                std::cerr << "Can not set an option while searching" << std::endl;
                return;
            } else if (m_thread.joinable()) {
                m_thread.join();
            }
            set_option(iss);
        } else if (token == "eval") {
            eval();
        } else if (token == "uci") {
            std::cout << "id name Minke 0.0.1 \n"
                      << "id author Eduardo Marinho \n";
            EngineOptions::print();
            std::cout << "uciok" << std::endl;
        } else if (token == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (token == "help" || token == "--help") {
            std::cout << "TODO write help message." << std::endl;
        } else if (token == "d") {
            print_debug_info();
        } else if (token == "bench") {
            if (!m_search_data.stop)
                continue;
            else if (m_thread.joinable())
                m_thread.join();
            m_search_data.reset();
            m_search_data.depth_limit = EngineOptions::BenchDepth;
            parse_go(iss, true);
            bench();
        } else if (!token.empty()) {
            std::cout << "Unknown command: '" << token << "'. Type help for information." << std::endl;
        }
    } while (token != "quit");
}

void UCI::print_debug_info() {
    m_search_data.game_state.top().print();
    bool found;
    auto entry = TranspositionTable::get().probe(m_search_data.game_state.top(), found);
    Move ttmove = MoveNone;
    if (found) {
        ttmove = entry->best_move();
        std::cout << "Best move: " << ttmove.get_algebraic_notation() << std::endl;
    }
    MoveList move_list(m_search_data.game_state, ttmove);
    int n_legal_moves = move_list.n_legal_moves(m_search_data.game_state.top());
    std::cout << "Move list (" << n_legal_moves << "|" << move_list.size() - n_legal_moves << "): ";
    while (!move_list.empty()) {
        Move move = move_list.next_move();
        if (m_search_data.game_state.make_move(move)) {
            std::cout << move.get_algebraic_notation() << '[' << -m_search_data.game_state.eval() << "] ";
            m_search_data.game_state.undo_move();
        } else {
            std::cout << "(" << move.get_algebraic_notation() << ") ";
        }
    }
    std::cout << "\nEval: " << m_search_data.game_state.eval() << std::endl;
}

void UCI::position(std::istringstream &iss) {
    std::string token, fen, move;
    iss >> token;
    if (token == "startpos") {
        fen = StartFEN;
        iss >> move; // consume the "moves" token, if there is one.
    } else if (token == "fen") {
        while (iss >> token && token != "moves")
            fen += token + " ";
    } else {
        return;
    }

    std::vector<std::string> move_list;
    while (iss >> move)
        move_list.push_back(move);
    set_position(fen, move_list);
}

void UCI::set_position(const std::string &fen, const std::vector<std::string> &move_list) {
    if (!m_search_data.game_state.reset(fen)) {
        std::cerr << "Invalid FEN!" << std::endl;
        return;
    }
    TranspositionTable::get().clear();
    for (const std::string &algebraic_notation : move_list) {
        assert(m_search_data.game_state.make_move(m_search_data.game_state.top().get_movement(algebraic_notation)));
    }
}

void UCI::set_option(std::istringstream &iss) {
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
    TimeType total_time = 0;
    for (const std::string &fen : benchmark_fen_list) {
        m_search_data.game_state.reset(fen);
        m_search_data.time_manager.init();
        TranspositionTable::get().clear();
        TimeType start_time = now();
        go();
        m_thread.join();
        total_time += now() - start_time;
    }

    std::cout << "\n==========================\n";
    std::cout << "Total time: " << total_time << "ms\n";
    std::cout << "Nodes searched: " << m_search_data.nodes_searched << "\n";
    std::cout << "Nodes per second: " << m_search_data.nodes_searched * 1000 / total_time;
    std::cout << "\n==========================";
    std::cout << std::endl;
}

int64_t UCI::perft(Position &position, CounterType depth, bool root) {
    bool is_leaf = (depth == 2);
    int64_t count = 0, nodes = 0;

    MoveList move_list(position);
    while (!move_list.empty()) {
        Move move = move_list.next_move();
        Position position_copy = position;
        if (!position_copy.move(move))
            continue;

        if (root && depth <= 1)
            count = 1, ++nodes;
        else {
            count =
                is_leaf ? MoveList(position_copy).n_legal_moves(position_copy) : perft(position_copy, depth - 1, false);
            nodes += count;
        }

        if (root)
            std::cout << move.get_algebraic_notation() << ": " << count << std::endl;
    }

    if (root)
        std::cout << "\nNodes searched: " << nodes << std::endl;
    return nodes;
}

void UCI::eval() { std::cout << "The position evaluation is " << m_search_data.game_state.eval() << std::endl; }

bool UCI::parse_go(std::istringstream &iss, bool bench) {
    std::string token;
    CounterType time = -1;
    CounterType movestogo = -1;
    CounterType movetime = -1;
    CounterType inc = -1;
    bool infinite = false;
    while (iss >> token) {
        if (token == "infinite" && !bench) {
            infinite = true;
            return false;
        }

        CounterType option;
        iss >> option;
        if (token == "perft" && !iss.fail()) { // Don't "perft" if depth hasn't been passed
            m_search_data.depth_limit = option;
            return true;
        } else if (token == "depth") {
            m_search_data.depth_limit = option;
        } else if (token == "nodes") {
            m_search_data.node_limit = option;
        } else if (token == "movetime") {
            movetime = option;
        } else if (token == "wtime" && m_search_data.game_state.top().side_to_move() == Player::White) {
            time = option;
        } else if (token == "btime" && m_search_data.game_state.top().side_to_move() == Player::Black) {
            time = option;
        } else if (token == "winc" && m_search_data.game_state.top().side_to_move() == Player::White) {
            inc = option;
        } else if (token == "binc" && m_search_data.game_state.top().side_to_move() == Player::Black) {
            inc = option;
        } else if (token == "movestogo") {
            movestogo = option;
        }
    }

    m_search_data.time_manager.init(inc, time, movestogo, movetime, infinite);
    return false;
}

void UCI::go() { m_thread = std::thread(iterative_deepening, std::ref(m_search_data)); }

void EngineOptions::print() {
    std::cout << "option name Hash type spin default " << hash_default << " min " << hash_min << " max " << hash_max
              << "\n";
}
