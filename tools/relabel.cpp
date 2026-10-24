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

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "core/position.h"
#include "core/types.h"
#include "datagen/packed_position.h"
#include "eval/eval.h"
#include "eval/nnue/accumulator/stack.h"
#include "uci/init.h"

namespace fs = std::filesystem;

struct GlobalProgress {
    std::atomic<uint64_t> completed_bytes{0};
    std::atomic<uint64_t> completed_positions{0};
    std::atomic<bool> done{false};
    uint64_t total_bytes{0};
};

struct Task {
    std::filesystem::path in_path;
    std::filesystem::path out_path;
    uintmax_t file_size;
};

struct MmapInput {
    int fd = -1;
    size_t size = 0;
    const uint8_t *data = nullptr;

    MmapInput(const fs::path &path) {
        fd = open(path.c_str(), O_RDONLY);
        if (fd < 0) {
            throw std::runtime_error("failed to open input: " + path.string());
        }

        struct stat st;
        if (fstat(fd, &st) < 0 || st.st_size == 0) {
            close(fd);
            throw std::runtime_error("Empty or invalid file: " + path.string());
        }
        size = st.st_size;

        void *map = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED) {
            close(fd);
            throw std::runtime_error("Failed to mmap input: " + path.string());
        }

        posix_madvise(map, size, POSIX_MADV_SEQUENTIAL);
        data = static_cast<uint8_t *>(map);
    }

    ~MmapInput() {
        if (data && data != MAP_FAILED) {
            posix_madvise(const_cast<uint8_t *>(data), size, POSIX_MADV_DONTNEED);
            munmap(const_cast<uint8_t *>(data), size);
        }
        if (fd >= 0) {
            close(fd);
        }
    }

    MmapInput(const MmapInput &) = delete;
    MmapInput &operator=(const MmapInput &) = delete;
};

struct MmapOutput {
    int fd = -1;
    size_t size = 0;
    uint8_t *data = nullptr;

    MmapOutput(const fs::path &path, size_t target_size) : size(target_size) {
        fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            throw std::runtime_error("Failed to create output: " + path.string());
        }

        if (ftruncate(fd, size) != 0) {
            close(fd);
            throw std::runtime_error("Failed to fallocate output: " + path.string());
        }

        void *map = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED) {
            close(fd);
            throw std::runtime_error("Failed to mmap output: " + path.string());
        }

        data = static_cast<uint8_t *>(map);
    }

    ~MmapOutput() {
        if (data && data != MAP_FAILED) {
            munmap(data, size);
        }
        if (fd >= 0) {
            close(fd);
        }
    }

    MmapOutput(const MmapOutput &) = delete;
    MmapOutput &operator=(const MmapOutput &) = delete;
};

void relabel_file(const Task &task, GlobalProgress &progress) {
    using namespace minke;

    MmapInput in_file(task.in_path);
    MmapOutput out_file(task.out_path, task.file_size);

    const uint8_t *in_cursor = in_file.data;
    const uint8_t *const in_end = in_file.data + in_file.size;
    uint8_t *out_cursor = out_file.data;

    uint64_t position_count = 0;
    uint64_t bytes_count = 0;

    nnue::accumulator::Stack acc_stack;
    while (in_cursor + sizeof(datagen::PackedPosition) < in_end) {
        datagen::PackedPosition packed_pos;
        std::memcpy(&packed_pos, in_cursor, sizeof(packed_pos));
        std::memcpy(out_cursor, &packed_pos, sizeof(packed_pos));

        in_cursor += sizeof(packed_pos);
        out_cursor += sizeof(packed_pos);
        bytes_count += sizeof(packed_pos);

        Position pos(packed_pos);
        acc_stack.refresh(pos);

        while (true) {
            if (in_cursor + sizeof(uint32_t) > in_end) {
                throw std::runtime_error("File is incorrectly truncated: " + task.in_path.string());
            }

            uint32_t raw_packed_move_score;
            std::memcpy(&raw_packed_move_score, in_cursor, sizeof(uint32_t));

            in_cursor += sizeof(uint32_t);
            bytes_count += sizeof(uint32_t);

            if (raw_packed_move_score == 0) { // null terminator
                std::memcpy(out_cursor, &raw_packed_move_score, sizeof(uint32_t));
                out_cursor += sizeof(uint32_t);
                break;
            }

            int16_t eval = eval::evaluate(pos, acc_stack.update_top(pos));
            if (pos.stm() != WHITE) { // eval should be white relative
                eval = -eval;
            }

            const uint16_t packed_move = static_cast<uint16_t>(raw_packed_move_score);
            std::memcpy(out_cursor, &packed_move, sizeof(packed_move));
            out_cursor += sizeof(uint16_t);
            std::memcpy(out_cursor, &eval, sizeof(eval));
            out_cursor += sizeof(int16_t);

            position_count += 1;

            // apply move to state tracker
            const Move move(packed_move, pos);
            const DirtyPiece dp = pos.make_move(move);
            acc_stack.push(dp, pos.king_sq(WHITE), pos.king_sq(BLACK));
            pos.update_game_history();
        }

        // flush progress after every game
        progress.completed_bytes.fetch_add(bytes_count, std::memory_order_relaxed);
        progress.completed_positions.fetch_add(position_count, std::memory_order_relaxed);
        bytes_count = 0;
        position_count = 0;
    }

    if (in_cursor != in_end) {
        throw std::runtime_error("Unexpected trailing bytes at EOF in " + task.in_path.string());
    }
}

void monitor_thread_func(GlobalProgress &progress) {
    using clock = std::chrono::steady_clock;
    const auto start_time = clock::now();
    auto prev_time = start_time;

    uint64_t prev_bytes = 0;
    uint64_t prev_pos = 0;
    const double total_gb = progress.total_bytes / (1024.0 * 1024.0 * 1024.0);
    while (!progress.done.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        auto now = clock::now();
        const std::chrono::duration<double> dt = now - prev_time;
        const std::chrono::duration<double> total_dt = now - start_time;

        const uint64_t cur_bytes = progress.completed_bytes.load(std::memory_order_relaxed);
        const uint64_t cur_pos = progress.completed_positions.load(std::memory_order_relaxed);

        const uint64_t delta_bytes = cur_bytes - prev_bytes;
        const uint64_t delta_pos = cur_pos - prev_pos;

        prev_time = now;
        prev_bytes = cur_bytes;
        prev_pos = cur_pos;

        const double mpos_sec = static_cast<double>(delta_pos) / 1e6 / dt.count();
        const double mib_sec = (static_cast<double>(delta_bytes) / (1024.0 * 1024.0)) / dt.count();
        const double cur_gb = static_cast<double>(cur_bytes) / (1024.0 * 1024.0 * 1024.0);
        const double cur_progress =
            progress.total_bytes > 0 ? (static_cast<double>(cur_bytes) / progress.total_bytes) * 100 : 0;

        const double avg_bytes_sec = static_cast<double>(cur_bytes) / total_dt.count();
        const int eta_s = (avg_bytes_sec > 0 ? (progress.total_bytes - cur_bytes) / avg_bytes_sec : 0);

        std::cout << "\r\033[K"
                  << "[" << std::fixed << std::setprecision(1) << cur_progress << "%] " << cur_gb << " / " << total_gb
                  << " GB | " << std::setprecision(2) << mpos_sec << "M pos/s | " << std::setprecision(1) << mib_sec
                  << " MB/s | "
                  << "ETA: " << std::setfill('0') << std::setw(2) << eta_s / 3600 << ":" << std::setfill('0')
                  << std::setw(2) << (eta_s % 3600) / 60 << ":" << std::setfill('0') << std::setw(2) << (eta_s % 60)
                  << std::flush;
    }
    std::cout << "\n";
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        std::cerr << "usage: " << argv[0] << " <input> <output> [num_threads]\n";
        return 1;
    }

    const fs::path in_path = argv[1];
    const fs::path out_path = argv[2];

    if (!fs::exists(in_path)) {
        std::cerr << "Err: input path does not exist\n";
        return 1;
    }

    minke::init_all();

    GlobalProgress progress;
    std::vector<Task> tasks;

    if (fs::exists(out_path)) {
        std::cerr << "Err: output path alreday exists\n";
        return 1;
    }

    if (!fs::is_directory(in_path)) {
        const uintmax_t file_size = fs::file_size(in_path);

        if (file_size == 0) {
            std::cerr << "Err: input file is empty\n";
            return 1;
        }
        progress.total_bytes += file_size;

        std::thread monitor_thread(monitor_thread_func, std::ref(progress));

        const Task task{in_path, out_path, file_size};
        try {
            relabel_file(task, progress);
        } catch (const std::exception &ex) {
            std::cerr << "\n[Worker Err] Failed on " << task.in_path << ": " << ex.what() << "\n";
        }
        progress.done.store(true, std::memory_order_relaxed);
        if (monitor_thread.joinable()) {
            monitor_thread.join();
        }

        return 0;
    }
    fs::create_directories(out_path);

    for (const auto &entry : fs::directory_iterator(in_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".vf") {
            const uintmax_t file_size = entry.file_size();
            if (file_size > 0) {
                tasks.emplace_back(entry.path(), out_path / entry.path().filename(), file_size);
                progress.total_bytes += file_size;
            }
        } else {
            std::cout << "skipped file: " << entry.path() << std::endl;
        }
    }

    if (tasks.empty()) {
        std::cerr << "No file found to processed\n";
        return 0;
    }

    // LPT, process bigger files first to reduce stranglers at the end
    std::sort(tasks.begin(), tasks.end(),
              [](const Task &lhs, const Task &rhs) { return lhs.file_size > rhs.file_size; });

    size_t num_threads = std::thread::hardware_concurrency();
    if (argc > 3) {
        num_threads = std::max(1, std::stoi(argv[3]));
    }
    num_threads = std::min(num_threads, tasks.size());

    std::cout << "Found " << tasks.size() << " files (" << std::setprecision(1)
              << progress.total_bytes / (1024.0 * 1024.0 * 1024.0) << " GB).\n";
    std::cout << "Launching " << num_threads << " workers...\n";

    std::thread monitor_thread(monitor_thread_func, std::ref(progress));

    std::atomic<size_t> next_task_id{0};
    std::vector<std::thread> workers;
    workers.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        workers.emplace_back([&]() {
            while (true) {
                const size_t idx = next_task_id.fetch_add(1, std::memory_order_relaxed);
                if (idx >= tasks.size()) {
                    break;
                }
                try {
                    relabel_file(tasks[idx], progress);
                } catch (const std::exception &ex) {
                    std::cerr << "\n[Worker Err] Failed on " << tasks[idx].in_path << ": " << ex.what() << "\n";
                }
            }
        });
    }

    for (auto &w : workers) {
        if (w.joinable()) {
            w.join();
        }
    }

    progress.done.store(true, std::memory_order_relaxed);
    if (monitor_thread.joinable()) {
        monitor_thread.join();
    }

    std::cout << "Finished relabeling all files successfully\n";
    return 0;
}
