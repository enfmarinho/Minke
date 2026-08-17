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

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "core/types.h"
#include "datagen/viriformat.h"
#include "search/search.h"
#include "utils/random.h"

class DatagenThread {
  private:
    static constexpr int VERIFICATION_MAX_SCORE = 800;
    static constexpr int VERIFICATION_SOFT_NODE_LIMIT = 50'000;
    static constexpr int VERIFICATION_HARD_NODE_LIMIT = 5 * VERIFICATION_SOFT_NODE_LIMIT;
    static constexpr int VERIFICATION_MAX_DEPTH = 14;

    static constexpr int SOFT_NODE_LIMIT = 5'000;
    static constexpr int HARD_NODE_LIMIT = 20 * SOFT_NODE_LIMIT;

    static constexpr int WIN_ADJ_PLY = 4;
    static constexpr int DRAW_ADJ_PLY = 12;
    static constexpr int WIN_ADJ_SCORE = 2000;
    static constexpr int DRAW_ADJ_SCORE = 10;
    static constexpr int DRAW_ADJ_MIN_PLY = 60;

  public:
    DatagenThread() = delete;
    DatagenThread(int id, int tt_size_mb, std::string& dir_path, uint64_t seed);
    ~DatagenThread();

    void run();
    void stop();

    inline int id() const { return m_id; }
    inline uint64_t game_count() const { return m_game_count.load(std::memory_order_relaxed); }
    inline uint64_t positions_count() const { return m_position_count.load(std::memory_order_relaxed); }
    inline bool stopped() const { return m_stop_flag; }

  private:
    void init_pos_randomly();
    void play_game();

    Engine m_engine;

    int m_id;
    bool m_stop_flag;
    std::atomic<uint64_t> m_game_count;
    std::atomic<uint64_t> m_position_count;
    PRNG prng;

    std::ofstream m_file_out;
    Viriformat m_games;
};

class DatagenEngine {
  public:
    void datagen_loop(int thread_count, int tt_size_mb, std::string& dir_path);

  private:
    void report() const;

    void start(int thread_count, int tt_size_mb, std::string& dir, uint64_t master_seed);
    void stop();

    TimeType m_start_time;

    std::vector<std::unique_ptr<DatagenThread>> m_datagen_threads;
    std::vector<std::thread> m_threads;
};
