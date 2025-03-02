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
#include "move.h"
#include "movegen.h"
#include "movepicker.h"
#include "position.h"
#include "search.h"
#include "tt.h"
#include "types.h"

UCI::UCI(int argc, char *argv[]) {
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
                perft(m_search_data.position, m_search_data.depth_limit);
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
    m_search_data.position.print();
    bool found;
    auto entry = TT.probe(m_search_data.position, found);
    Move ttmove = MoveNone;
    if (found) {
        ttmove = entry->best_move();
        std::cout << "Best move: " << ttmove.get_algebraic_notation() << std::endl;
    }
    MovePicker move_picker(ttmove, &m_search_data);
    std::cout << "Move list: ";
    while (!move_picker.finished()) {
        ScoredMove scored_move = move_picker.next_move_scored(false);
        if (!m_search_data.position.make_move<false>(scored_move.move))
            std::cout << "*";
        std::cout << scored_move.move.get_algebraic_notation() << ' ' << "[" << scored_move.score << "] ";
        m_search_data.position.unmake_move<false>(scored_move.move);
    }
    std::cout << "\nNNUE eval: " << m_search_data.position.eval() << std::endl;
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
    if (!m_search_data.position.set_fen<true>(fen)) {
        std::cerr << "Invalid FEN!" << std::endl;
        return;
    }
    TT.clear();
    for (const std::string &algebraic_notation : move_list) {
        m_search_data.position.make_move<false>(m_search_data.position.get_movement(algebraic_notation));
    }
    m_search_data.position.reset_nnue();
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
        TT.resize(value);
    }
}

void UCI::bench() {
    TimeType total_time = 0;
    for (const std::string &fen : benchmark_fen_list) {
        m_search_data.position.set_fen<true>(fen);
        m_search_data.time_manager.init();
        TT.clear();
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

    ScoredMove moves[MaxMoves];
    ScoredMove *end = gen_moves(moves, m_search_data.position, GenAll);
    for (ScoredMove *begin = moves; begin != end; ++begin) {
        Move move = begin->move;
        if (!position.make_move<false>(move)) {
            position.unmake_move<false>(move);
            continue;
        }

        if (root && depth <= 1)
            count = 1, ++nodes;
        else {
            count = is_leaf ? position.legal_move_amount() : perft(position, depth - 1, false);
            nodes += count;
        }
        position.unmake_move<false>(move);

        if (root)
            std::cout << move.get_algebraic_notation() << ": " << count << std::endl;
    }

    if (root)
        std::cout << "\nNodes searched: " << nodes << std::endl;
    return nodes;
}

void UCI::eval() { std::cout << "The position evaluation is " << m_search_data.position.eval() << std::endl; }

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
        } else if (token == "wtime" && m_search_data.position.get_stm() == White) {
            time = option;
        } else if (token == "btime" && m_search_data.position.get_stm() == Black) {
            time = option;
        } else if (token == "winc" && m_search_data.position.get_stm() == White) {
            inc = option;
        } else if (token == "binc" && m_search_data.position.get_stm() == Black) {
            inc = option;
        } else if (token == "movestogo") {
            movestogo = option;
        }
    }

    m_search_data.time_manager.init(m_search_data.position, inc, time, movestogo, movetime, infinite);
    return false;
}

void UCI::go() { m_thread = std::thread(iterative_deepening, std::ref(m_search_data)); }

void EngineOptions::print() {
    std::cout << "option name Hash type spin default " << hash_default << " min " << hash_min << " max " << hash_max
              << "\n";
}
