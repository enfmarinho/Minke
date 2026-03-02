/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef DATAGEN_H
#define DATAGEN_H

#include <algorithm>
#include <atomic>
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
#include <thread>
#include <vector>

#include "../hash.h"
#include "../move.h"
#include "../movegen.h"
#include "../position.h"
#include "../search.h"
#include "../types.h"
#include "packed_position.h"
#include "viriformat.h"

class DatagenThread {
  private:
    static constexpr int VERIFICATION_MAX_SCORE = 800;
    static constexpr int VERIFICATION_SOFT_NODE_LIMIT = 80'000;
    static constexpr int VERIFICATION_HARD_NODE_LIMIT = 500'000;
    static constexpr int VERIFICATION_MAX_DEPTH = 14;

    static constexpr int SOFT_NODE_LIMIT = 25'000;
    static constexpr int HARD_NODE_LIMIT = 100'000;

    static constexpr int WIN_ADJ_PLY = 4;
    static constexpr int DRAW_ADJ_PLY = 12;
    static constexpr int WIN_ADJ_SCORE = 2000;
    static constexpr int DRAW_ADJ_SCORE = 10;
    static constexpr int DRAW_ADJ_MIN_PLY = 60;

  public:
    DatagenThread() = delete;
    DatagenThread(int id, int tt_size_mb, std::string& dir_path, uint64_t seed)
        : m_id(id), m_game_count(0), m_position_count(0), m_stop_flag(false), prng(seed) {
        m_td = std::make_unique<ThreadData>();
        init(tt_size_mb, dir_path);
    }
    ~DatagenThread() { m_file_out.close(); }

    void init(int tt_size_mb, std::string& dir_path) {
        std::filesystem::path path(dir_path + "/minke_data" + std::to_string(m_id) + ".vf");

        // Ensure path is valid for the creation of the output file
        std::filesystem::create_directories(path.parent_path());

        m_file_out.open(path, std::ios_base::ios_base::app | std::ios_base::ios_base::binary);

        m_td->datagen = true;
        m_td->report = false;
        m_td->tt.resize(tt_size_mb);
    }

    void run() {
        m_stop_flag = false;
        while (!m_stop_flag) {
            play_game();
            if (m_position_count % 10'000 == 0)
                m_file_out.flush();
        }

        m_file_out.flush();
    }

    void stop() {
        m_stop_flag = true;
        m_td->stop = true;
    };

    int get_id() const { return m_id; }
    uint64_t get_game_count() const { return m_game_count.load(std::memory_order_relaxed); }
    uint64_t get_positions_count() const { return m_position_count.load(std::memory_order_relaxed); }

  private:
    void play_game() {
        init_pos_randomly();

        // Search deeper to verify position before generating data from it
        m_td->reset_search_parameters();
        m_td->set_search_limits({VERIFICATION_MAX_DEPTH, VERIFICATION_SOFT_NODE_LIMIT, VERIFICATION_HARD_NODE_LIMIT});

        ScoreType verification_score = iterative_deepening(*m_td);
        if (std::abs(verification_score) > VERIFICATION_MAX_SCORE) {
            return;
        }

        GameResult result = NO_RESULT;
        int win_count = 0;
        int draw_count = 0;
        uint64_t position_count = 0;

        while (!m_stop_flag) {
            m_td->reset_search_parameters();
            m_td->set_search_limits({MAX_SEARCH_DEPTH, SOFT_NODE_LIMIT, HARD_NODE_LIMIT});

            ScoreType score = iterative_deepening(*m_td);
            ScoreType normalized_score = normalize_score(score);
            ++position_count;

            Move move = m_td->best_move;

            if (move == MOVE_NONE) {
                if (m_td->position.in_check())
                    result = m_td->position.get_stm() == WHITE ? LOSS : WIN;
                else
                    result = DRAW;

                break;
            }

            if (m_td->position.get_stm() == BLACK)
                score *= -1;

            if (std::abs(score) >= MATE_FOUND) {
                result = score > 0 ? WIN : LOSS;
            } else {
                if (std::abs(normalized_score) > WIN_ADJ_SCORE) {
                    ++win_count;
                    draw_count = 0;
                } else if (std::abs(normalized_score) < DRAW_ADJ_SCORE &&
                           m_td->position.get_game_ply() >= DRAW_ADJ_MIN_PLY) {
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

            if (m_td->position.draw()) {
                result = DRAW;
                score = 0;
            }

            m_games.push(move, score);

            if (result != NO_RESULT)
                break;

            m_td->position.make_move<true>(move);
            m_td->position.update_game_history();
        }

        if (result != NO_RESULT) {
            m_games.write(m_file_out, result);

            m_position_count.fetch_add(position_count, std::memory_order_relaxed);
            m_game_count.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void init_pos_randomly() {
        m_td->position.set_fen<true>(START_FEN);

        int move_count = rand(8, 12);
        for (int i = 0; i < move_count; ++i) {
            ScoredMove moves[MAX_MOVES_PER_POS];
            ScoredMove* end = gen_moves(moves, m_td->position, MoveGenType::GEN_ALL);

            std::shuffle(moves, end, prng);

            bool legal_found = false;
            for (auto curr = moves; curr != end; ++curr) {
                if (!m_td->position.make_move<false>(curr->move)) {
                    m_td->position.unmake_move<false>(curr->move);
                } else {
                    legal_found = true;
                    break;
                }
            }

            if (!legal_found) {
                m_td->position.set_fen<true>(START_FEN);
                i = -1; // increment is happening after the loop, so this will be 0
            }
        }

        m_td->search_history.reset();
        m_td->tt.clear();
        m_games.reset(m_td->position);
    }

    std::unique_ptr<ThreadData> m_td;

    int m_id;
    std::atomic<uint64_t> m_game_count;
    std::atomic<uint64_t> m_position_count;
    bool m_stop_flag;
    PRNG prng;

    std::ofstream m_file_out;
    Viriformat m_games;
};

class DatagenEngine {
  public:
    void datagen_loop(int thread_count, int tt_size_mb, std::string& dir_path) {
        uint64_t master_seed = SeedGenerator::master_seed();
        std::cout << "Datagen started with " << thread_count << " thread(s) and " << master_seed << " seed\n";
        start(thread_count, tt_size_mb, dir_path, master_seed);

        m_start_time = now();
        std::string input, command;
        while (getline(std::cin, input)) {
            std::istringstream iss(input);
            iss >> command;

            if (command == "stop") {
                break;
            } else if (command == "report") {
                report();
            } else if (command == "pause") {
                stop();
                std::cout << "Datagen paused" << std::endl;
            } else if (command == "resume") {
                run();
                std::cout << "Datagen resumed" << std::endl;
            } else if (command == "isalive") {
                std::cout << "alive" << std::endl;
            }
        }

        stop();
        report();

        std::cout << "Datagen ran successfully!\n";
    }

  private:
    void report() const {
        constexpr char line[] = "+------------+------------+------------+------------+------------+\n";

        TimeType elapsed_time = now() - m_start_time + 1; // plus 1 to avoid divisions by 0

        auto print_info_line = [elapsed_time](std::string id, uint64_t game_count, uint64_t fen_count) {
            std::cout << "|";
            std::cout << std::setw(11) << std::right << id << " |";
            std::cout << std::setw(11) << std::right << game_count << " |";
            std::cout << std::setw(11) << std::right << fen_count << " |";
            std::cout << std::setw(11) << std::right << 3600 * game_count * 1000 / elapsed_time << " |";
            std::cout << std::setw(11) << std::right << 3600 * fen_count * 1000 / elapsed_time << " |";
            std::cout << "\n";
        };

        std::cout << line;
        std::cout << "| thread id  | game count | fen count  |  games/h   |   fens/h   |\n";
        std::cout << line;

        uint64_t game_count = 0;
        uint64_t position_count = 0;
        for (const std::unique_ptr<DatagenThread>& dt_ptr : m_datagen_thread_ptrs) {
            print_info_line(std::to_string(dt_ptr->get_id()), dt_ptr->get_game_count(), dt_ptr->get_positions_count());

            position_count += dt_ptr->get_positions_count();
            game_count += dt_ptr->get_game_count();
        }

        std::cout << line;
        print_info_line("total", game_count, position_count);
        std::cout << line;
    }

    void start(int thread_count, int tt_size_mb, std::string& dir, uint64_t master_seed) {
        m_stop_flag = false;

        SeedGenerator seed_gen(master_seed);
        m_datagen_thread_ptrs.reserve(thread_count);
        for (int id = 0; id < thread_count; ++id)
            m_datagen_thread_ptrs.emplace_back(std::make_unique<DatagenThread>(id, tt_size_mb, dir, seed_gen.next()));

        m_threads.reserve(thread_count);
        for (int id = 0; id < thread_count; ++id)
            m_threads.emplace_back(&DatagenThread::run, m_datagen_thread_ptrs[id].get());
    }

    void run() {
        if (!m_stop_flag)
            return;

        m_stop_flag = false;
        for (unsigned int i = 0; i < m_threads.size(); ++i)
            m_threads[i] = std::thread(&DatagenThread::run, m_datagen_thread_ptrs[i].get());
    }

    void stop() {
        if (m_stop_flag)
            return;

        m_stop_flag = true;
        for (std::unique_ptr<DatagenThread>& dt_ptr : m_datagen_thread_ptrs)
            dt_ptr->stop();

        for (auto& thread : m_threads)
            thread.join();
    }

    bool m_stop_flag = true;
    TimeType m_start_time;

    std::vector<std::unique_ptr<DatagenThread>> m_datagen_thread_ptrs;
    std::vector<std::thread> m_threads;
};

#endif // !DATAGEN_H
