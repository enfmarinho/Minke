/*
 *  Minke is a UCI chess engine
 *  Copyright (C) 2026 Eduardo Marinho <eduardomarinho@pm.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "uci/uci.h"

#include <cassert>
#include <cstdint>
#include <exception>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "core/move.h"
#include "core/movegen.h"
#include "core/position.h"
#include "core/types.h"
#include "search/search.h"
#include "search/search_limiter.h"
#include "search/tt.h"
#include "uci/benchmark.h"

#ifdef TRACK_ACTIVATIONS
#include <fstream>
#endif
#ifdef TUNE
#include "uci/init.h"
#include "uci/tune.h"
#include "utils/utils.h"
#endif

namespace EngineOptions {

constexpr CounterType HASH_DEFAULT = 16;
constexpr CounterType HASH_MIN = 1;
constexpr CounterType HASH_MAX = 2097152;
constexpr CounterType THREADS_DEFAULT = 1;
constexpr CounterType THREADS_MIN = 1;
constexpr CounterType THREADS_MAX = 2048;

void print() {
    std::cout << "option name Hash type spin default " << HASH_DEFAULT << " min " << HASH_MIN << " max " << HASH_MAX
              << "\n";
    std::cout << "option name Threads type spin default " << THREADS_DEFAULT << " min " << THREADS_MIN << " max "
              << THREADS_MAX << "\n";
    std::cout << "option name UCI_Chess960 type check default false\n";

#ifdef TUNE
    for (const TunableParam &tunable_param : TunableParamList::get()) {
        tunable_param.print();
    }
#endif
}

} // namespace EngineOptions

namespace UCI {

UciHandler::UciHandler() {
    m_pos.set_fen(START_FEN);
    m_engine.resize_tt(EngineOptions::HASH_DEFAULT);
    m_engine.new_game();
    m_engine.prepare_search(m_pos);
    m_engine.report(true);
}

void UciHandler::run() {
    std::cout << "Minke Chess Engine by Eduardo Marinho" << std::endl;

    ucinewgame();
    std::string input, token;
    do {
        if (!std::getline(std::cin, input))
            input = "quit";
        std::istringstream iss(input);

        token.clear();
        iss >> std::skipws >> token;
        if (token == "quit" || token == "stop") {
            m_engine.stop_search();
        } else if (token == "go") {
#ifdef TUNE
            init_search_params();
#endif
            if (!m_engine.stopped())
                continue;
            else if (m_thread.joinable())
                m_thread.join();
            m_engine.prepare_search();
            const CounterType perft_depth = parse_go(iss);
            if (perft_depth != 0) {
                perft(m_pos, perft_depth);
            } else {
                go();
            }
        } else if (token == "position") {
            position(iss);
        } else if (token == "ucinewgame") {
            ucinewgame();
        } else if (token == "setoption") {
            if (!m_engine.stopped()) {
                std::cerr << "Can not set an option while searching" << std::endl;
                continue;
            } else if (m_thread.joinable()) {
                m_thread.join();
            }
            set_option(iss);
        } else if (token == "eval") {
            eval();
        } else if (token == "uci") {
            std::cout << "id name Minke 6.0.0 \n"
                      << "id author Eduardo Marinho \n";
            EngineOptions::print();
            std::cout << "uciok" << std::endl;
        } else if (token == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (token == "d") {
            print_debug_info();
        } else if (token == "bench") {
            if (!m_engine.stopped())
                continue;
            else if (m_thread.joinable())
                m_thread.join();

            int bench_depth = Benchmark::DEFAULT_BENCH_DEPTH;
            iss >> std::skipws >> bench_depth;
            Benchmark::run(bench_depth);
        }
#ifdef TUNE
        else if (token == "tuneinfo") {
            for (const TunableParam &tunable_param : TunableParamList::get()) {
                tunable_param.print_ob_format();
            }
        }
#endif
        else if (!token.empty()) {
            std::cout << "Unknown command: '" << token << "'. Type help for information." << std::endl;
        }
    } while (token != "quit");

    if (m_thread.joinable())
        m_thread.join();
}

void UciHandler::print_debug_info() {
    m_pos.print();
    TTEntry tte;
    Movegen::ScoredMoveList move_list;
    Movegen::all(move_list, m_pos);
    std::cout << "Move list(" << move_list.size() << "): ";
    for (ScoredMove scored_move : move_list) {
        std::cout << m_pos.move_to_uci(scored_move.move) << " ";
    }
    std::cout << "\nNNUE eval: " << m_engine.static_eval() << std::endl;
}

void UciHandler::position(std::istringstream &iss) {
    std::string token, fen, move;
    iss >> token;
    if (token == "startpos") {
        fen = START_FEN;
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

void UciHandler::set_position(const std::string &fen, const std::vector<std::string> &moves) {
    if (!m_pos.set_fen(fen)) {
        std::cerr << "Invalid FEN!" << std::endl;
        return;
    }

    for (unsigned int index = 0; index < moves.size(); ++index) {
        // Make sure to only save the game history for the last 100 positions, more than that is completely unnecessary
        // Moreover, the second conditional assures that the history stacks don't overflow
        if (moves.size() - index == 100 || m_pos.history_ply() > 100)
            m_pos.reset_history();

        Movegen::ScoredMoveList move_list;
        Movegen::all(move_list, m_pos);

        for (auto scored_move : move_list) {
            if (moves[index] == m_pos.move_to_uci(scored_move.move)) {
                m_pos.make_move(scored_move.move);
                break;
            }
        }
    }
    m_engine.prepare_search(m_pos);
}

void UciHandler::ucinewgame() { m_engine.new_game(); }

void UciHandler::set_option(std::istringstream &iss) {
    std::string value;
    int value_int;
    bool value_bool;
    auto valid_int_value = [&](int min, int max) -> bool {
        try {
            value_int = std::stoi(value);
            return min <= value_int && value_int <= max;
        } catch (const std::exception &) {
            return false;
        }
    };
    auto valid_bool_value = [&]() -> bool {
        if (value == "true") {
            value_bool = true;
            return true;
        } else if (value == "false") {
            value_bool = false;
            return true;
        }
        return false;
    };

    std::string token, garbage;
    iss >> garbage; // Consume the "name" token
    iss >> token;
    iss >> garbage; // Consume the "value" token.
    iss >> value;
    if (token == "Hash" && valid_int_value(EngineOptions::HASH_MIN, EngineOptions::HASH_MAX)) {
        m_engine.resize_tt(value_int);
    } else if (token == "Threads" && valid_int_value(EngineOptions::THREADS_MIN, EngineOptions::THREADS_MAX)) {
        m_engine.resize_threads(value_int);
    } else if (token == "UCI_Chess960" && valid_bool_value()) {
        m_pos.chess960(value_bool);
    }
#ifdef TUNE
    else if (TunableParam *param_ptr = TunableParamList::get().find(token)) {
        param_ptr->curr_value = std::stoi(value);
    }
#endif
    else {
        std::cout << "Trying to set unknown option: " << token << "\n";
    }
}

void UciHandler::bench(int depth) {
    TimeType total_time = 0;
    int64_t nodes_searched = 0;
    m_engine.report(false);
    for (const std::string &fen : BENCHMARK_FEN_LIST) {
        ucinewgame();
        m_pos.set_fen(fen);
        m_engine.prepare_search(m_pos);

        SearchLimits sl;
        sl.depth = depth;
        m_engine.limit_search(sl);

        TimeType start_time = now();
        go();
        m_thread.join();
        nodes_searched += m_engine.nodes_searched();
        total_time += now() - start_time;
    }
    m_engine.report(true);

    std::cout << "info time " << total_time << "ms\n";
    std::cout << nodes_searched << " nodes " << nodes_searched * 1000 / total_time << " nps\n";

#ifdef TRACK_ACTIVATIONS
    std::ofstream out_file("activations_table.txt");
    if (!out_file) {
        std::cerr << "Failed to open file to write activations table data\n";
        return;
    }

    const auto table = m_engine.main_td().nnue.activation_table();
    bool first = true;
    for (auto e : table) {
        if (!first)
            out_file << ", ";
        out_file << e;

        first = false;
    }
#endif // TRACK_ACTIVATIONS
}

int64_t UciHandler::perft(Position &position, CounterType depth, bool root) {
    const bool is_leaf = (depth == 2);
    int64_t count = 0, nodes = 0;

    Movegen::ScoredMoveList move_list;
    Movegen::all(move_list, position);
    for (ScoredMove score_move : move_list) {
        const Move move = score_move.move;
        position.make_move(move);

        if (root && depth <= 1) {
            count = 1;
        } else if (is_leaf) {
            Movegen::ScoredMoveList tmp;
            Movegen::all(tmp, position);
            count = tmp.size();
        } else {
            count = perft(position, depth - 1, false);
        }
        nodes += count;

        position.unmake_move(move);

        if (root)
            std::cout << position.move_to_uci(move) << ": " << count << std::endl;
    }

    if (root)
        std::cout << "\nNodes searched: " << nodes << std::endl;
    return nodes;
}

void UciHandler::eval() { std::cout << "The position evaluation is " << m_engine.static_eval() << std::endl; }

CounterType UciHandler::parse_go(std::istringstream &iss, bool bench) {
    std::string token;
    SearchLimits limits;

    while (iss >> token) {
        if (token == "infinite" && !bench) {
            limits.infinite = true;
            break;
        }

        CounterType option;
        iss >> option;
        if (token == "perft" && !iss.fail()) { // Don't "perft" if depth hasn't been passed
            return option;
        } else if (token == "depth") {
            limits.depth = option;
        } else if (token == "nodes") {
            limits.maximum_node = option;
        } else if (token == "movetime") {
            limits.movetime = option;
        } else if (token == "wtime" && m_pos.stm() == WHITE) {
            limits.time_remaining = option;
        } else if (token == "btime" && m_pos.stm() == BLACK) {
            limits.time_remaining = option;
        } else if (token == "winc" && m_pos.stm() == WHITE) {
            limits.time_increment = option;
        } else if (token == "binc" && m_pos.stm() == BLACK) {
            limits.time_increment = option;
        } else if (token == "movestogo") {
            limits.mtg = option;
        }
    }

    m_engine.limit_search(limits);
    return 0;
}

void UciHandler::go() { m_thread = std::thread(&Engine::search, std::ref(m_engine)); }

} // namespace UCI
