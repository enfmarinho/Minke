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

#include "datagen/datagen.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#include "core/move.h"
#include "core/movegen.h"
#include "core/position.h"
#include "core/types.h"
#include "datagen/packed_position.h"
#include "datagen/viriformat.h"
#include "search/search.h"
#include "search/search_limiter.h"
#include "utils/random.h"

DatagenThread::DatagenThread(int id, int tt_size_mb, std::filesystem::path& dir_path, uint64_t seed)
    : m_id(id), m_stop_flag(false), m_game_count(0), m_position_count(0), prng(seed) {
    std::filesystem::path path = std::filesystem::path(dir_path) / ("minke_data" + std::to_string(m_id) + ".vf");

    // Ensure path is valid for the creation of the output file
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) {
        std::cerr << "Err: Datagen Thread " << m_id << " failed to create directory " << path.parent_path() << ": "
                  << ec.message() << '\n';
        std::exit(EXIT_FAILURE);
    }

    m_file_out.open(path, std::ios::app | std::ios::binary);
    if (!m_file_out.is_open()) {
        std::cerr << "Err: Datagen Thread " << m_id << " failed to open file: " << path << '\n';
        std::exit(EXIT_FAILURE);
    }

    m_engine.report(false);
    m_engine.resize_tt(tt_size_mb);
}
DatagenThread::~DatagenThread() { m_file_out.close(); }

void DatagenThread::run() {
    m_stop_flag.store(false, std::memory_order_relaxed);
    while (!stopped()) {
        play_game();
    }
}

void DatagenThread::stop() {
    m_stop_flag.store(true, std::memory_order_relaxed);
    m_engine.stop_search();
}

void DatagenThread::play_game() {
    init_pos_randomly();

    // Search deeper to verify position before generating data from it
    SearchLimits verification_sl;
    verification_sl.depth = VERIFICATION_MAX_DEPTH;
    verification_sl.optimum_node = VERIFICATION_SOFT_NODE_LIMIT;
    verification_sl.maximum_node = VERIFICATION_HARD_NODE_LIMIT;
    m_engine.prepare_search();
    m_engine.limit_search(verification_sl);

    auto [_, verification_score] = m_engine.search();
    if (std::abs(verification_score) > VERIFICATION_MAX_SCORE) {
        return;
    }

    GameResult result = NO_RESULT;
    int win_count = 0;
    int draw_count = 0;
    uint64_t position_count = 0;

    SearchLimits sl;
    sl.depth = MAX_SEARCH_DEPTH;
    sl.optimum_node = SOFT_NODE_LIMIT;
    sl.maximum_node = HARD_NODE_LIMIT;

    while (!stopped()) {
        m_engine.prepare_search();
        m_engine.limit_search(sl);

        auto [move, score] = m_engine.search();
        const ScoreType normalized_score = normalize_score(score);
        ++position_count;

        if (!move) {
            if (m_engine.position().in_check())
                result = m_engine.position().stm() == WHITE ? LOSS : WIN;
            else
                result = DRAW;

            break;
        }

        if (m_engine.position().stm() == BLACK)
            score *= -1;

        if (std::abs(score) >= MATE_FOUND) {
            result = score > 0 ? WIN : LOSS;
        } else {
            if (std::abs(normalized_score) > WIN_ADJ_SCORE) {
                ++win_count;
                draw_count = 0;
            } else if (std::abs(normalized_score) < DRAW_ADJ_SCORE &&
                       m_engine.position().game_ply() >= DRAW_ADJ_MIN_PLY) {
                win_count = 0;
                ++draw_count;
            } else {
                win_count = 0;
                draw_count = 0;
            }

            if (win_count >= WIN_ADJ_PLY) {
                result = score > 0 ? WIN : LOSS;
            } else if (draw_count >= DRAW_ADJ_PLY) {
                result = DRAW;
            }
        }

        if (m_engine.position().is_draw()) {
            result = DRAW;
            score = 0;
        }

        m_games.push(move, score);

        if (result != NO_RESULT)
            break;

        make_move(m_engine.main_td(), move);
        m_engine.position().update_game_history();
    }

    if (result != NO_RESULT && !stopped()) {
        m_games.write(m_file_out, result);

        m_position_count.fetch_add(position_count, std::memory_order_relaxed);
        m_game_count.fetch_add(1, std::memory_order_relaxed);
    }
}

void DatagenThread::init_pos_randomly() {
    Position& pos = m_engine.position();
    pos.set_fen(START_FEN);

    // apply `move_count` random moves to opening. If not reached `move_count` and there is no legal moves restart
    const int move_count = 8 + (prng.rand<uint32_t>() % 5);
    for (int i = 0; i < move_count; ++i) {
        Movegen::ScoredMoveList move_list;
        Movegen::all(move_list, pos);

        if (move_list.empty()) { // no legal moves, restart from new opening
            pos.set_fen(START_FEN);
            i = -1; // increment is happening after the loop, so this will be 0
        } else {
            // apply random move
            const Move move = move_list[prng.rand<size_t>() % move_list.size()].move;
            pos.make_move(move);
        }
    }

    // initialize engine for search
    m_engine.main_td().nnue.refresh(pos);
    m_engine.new_game();
    m_games.reset(pos);
}

DatagenEngine::~DatagenEngine() { stop(); }

void DatagenEngine::datagen_loop(int thread_count, int tt_size_mb, std::filesystem::path& dir_path) {
    const uint64_t master_seed = SeedGenerator::master_seed();
    start(thread_count, tt_size_mb, dir_path, master_seed);
    std::cout << "Datagen started with " << thread_count << " thread(s) and " << master_seed << " seed\n";

    m_start_time = now();
    std::string input, command;
    while (getline(std::cin, input)) {
        std::istringstream iss(input);
        iss >> command;

        if (command == "stop") {
            break;
        } else if (command == "report" || command == "r") {
            report();
        } else if (command == "isalive") {
            std::cout << "alive" << std::endl;
        }
    }

    stop();
    report();

    std::cout << "Datagen ran successfully!\n";
}

void DatagenEngine::report() const {
    constexpr char line[] = "+------------+------------+------------+------------+------------+\n";

    TimeType elapsed_time = now() - m_start_time + 1; // plus 1 to avoid divisions by 0

    auto print_info_line = [elapsed_time](std::string id, uint64_t game_count, uint64_t fen_count) {
        std::cout << "|";
        std::cout << std::setw(11) << std::right << id << " |";
        std::cout << std::setw(11) << std::right << game_count << " |";
        std::cout << std::setw(11) << std::right << fen_count << " |";
        std::cout << std::setw(11) << std::right << 3600ull * game_count * 1000ull / elapsed_time << " |";
        std::cout << std::setw(11) << std::right << 3600ull * fen_count * 1000ull / elapsed_time << " |";
        std::cout << "\n";
    };

    std::cout << line;
    std::cout << "| thread id  | game count | fen count  |  games/h   |   fens/h   |\n";
    std::cout << line;

    uint64_t game_count = 0;
    uint64_t position_count = 0;
    for (const auto& dt_ptr : m_datagen_threads) {
        print_info_line(std::to_string(dt_ptr->id()), dt_ptr->game_count(), dt_ptr->positions_count());

        position_count += dt_ptr->positions_count();
        game_count += dt_ptr->game_count();
    }

    std::cout << line;
    print_info_line("total", game_count, position_count);
    std::cout << line;
}

void DatagenEngine::start(int thread_count, int tt_size_mb, std::filesystem::path& dir, uint64_t master_seed) {
    SeedGenerator seed_gen(master_seed);
    m_datagen_threads.reserve(thread_count);
    for (int id = 0; id < thread_count; ++id)
        m_datagen_threads.emplace_back(std::make_unique<DatagenThread>(id, tt_size_mb, dir, seed_gen.next()));

    m_threads.reserve(thread_count);
    for (int id = 0; id < thread_count; ++id)
        m_threads.emplace_back(&DatagenThread::run, m_datagen_threads[id].get());
}

void DatagenEngine::stop() {
    for (auto& datagen_thread : m_datagen_threads) {
        if (datagen_thread) {
            datagen_thread->stop();
        }
    }

    for (auto& thread : m_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    m_threads.clear();
    m_datagen_threads.clear();
}
