#ifdef DATAGEN_BUILD
#ifndef DATAGEN_H
#define DATAGEN_H

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "move.h"
#include "movegen.h"
#include "position.h"
#include "search.h"
#include "types.h"

static constexpr int DATAGEN_DEFAULT_MAX_DEPTH = 9;
static constexpr int DATAGEN_DEFAULT_OPTIMUM_NODE_LIMIT = 3000;
static constexpr int DATAGEN_DEFAULT_MAXIMUM_NODE_LIMIT = 9000;
static constexpr int DATAGEN_REPORT_INTERVAL = 10000;

static constexpr int DATAGEN_OPENING_RANDOM_MOVES = 8;
static constexpr int DATAGEN_MAX_CONSECUTIVE_WIN_SCORES = 4;
static constexpr int DATAGEN_MAX_CONSECUTIVE_DRAW_SCORES = 6;
static constexpr int DATAGEN_BASELINE_WIN_SCORE = 2000;
static constexpr int DATAGEN_BASELINE_DRAW_SCORE = 10;

struct FenData {
    enum Format {
        BULLET,
        TXT,
    };

    std::string fen;
    ScoreType score;

    FenData(std::string fen, ScoreType score) : fen(fen), score(score) {}

    std::string string_out(Format format, std::string result) {
        switch (format) {
            case BULLET:
                // TODO not implemented yet
            case TXT:
                return fen + " | " + std::to_string(score) + " | " + result + '\n';
            default:
                __builtin_unreachable();
        }
    }
};

// NOTE: This just support epd format
class OpeningBook {
  public:
    OpeningBook() = default;
    OpeningBook(std::string book_path) {
        if (book_path.empty())
            return;

        // Check for file extension
        std::filesystem::path path(book_path);
        if (path.extension() != ".epd") {
            std::cerr << "For the datagen book functionality to work the book must be on epd format and use the .epd "
                         "file extension\n";
            std::cerr << "Stopping execution early!\n";
            exit(1);
        }

        std::ifstream file_in(path);
        if (!file_in)
            return;

        std::string line;
        while (std::getline(file_in, line))
            openings_fen.push_back(line);
    }
    ~OpeningBook() = default;

    std::string random_fen() const {
        if (openings_fen.empty())
            return START_FEN;

        int random_index = rand(0, openings_fen.size() - 1);
        return openings_fen[random_index];
    };

  private:
    std::vector<std::string> openings_fen;
};

class DatagenThread {
  public:
    DatagenThread() = delete;
    DatagenThread(const SearchLimits& limits, int id, const OpeningBook& book)
        : id(id), game_count(0), fen_count(0), stop_flag(false), book(book), search_limits(limits) {}
    DatagenThread(const DatagenThread& rhs)
        : id(rhs.id),
          game_count(rhs.game_count),
          fen_count(rhs.fen_count),
          stop_flag(rhs.stop_flag),
          book(rhs.book),
          search_limits(rhs.search_limits) {}

    void start(int tt_size_mb) {
        std::filesystem::path path("data/minke_data" + std::to_string(id) + ".txt");

        // Assure path is valid for the creation of the output file
        std::filesystem::create_directories(path.parent_path());

        file_out.open(path, std::ios_base::ios_base::app);

        td.report = false;
        td.tt.resize(tt_size_mb);
        run();

        file_out.close();
    }

    void run() {
        stop_flag = false;
        while (!stop_flag) {
            game_count++;
            play_game();
        }

        file_out.flush();
    }

    void stop() {
        stop_flag = true;
        td.stop = true;
    };

    int get_game_count() const { return game_count; }
    int get_id() const { return id; }
    int get_fen_count() const { return fen_count; }

  private:
    void play_game() {
        init_pos_randomly();

        std::string game_result = "";
        int win_count = 0;
        int draw_count = 0;
        while (!stop_flag) {
            ++fen_count;
            bool in_check = td.position.in_check();
            if (td.position.no_legal_moves()) {
                game_result = (in_check ? std::to_string(td.position.get_stm()) + ".0" : "0.5");
                break;
            } else if (td.position.draw()) {
                game_result = "0.5";
                break;
            }

            td.reset_search_parameters();
            td.set_search_limits(search_limits);
            ScoreType score = iterative_deepening(td);

            if (td.position.get_stm() == BLACK)
                score *= -1;

            if (std::abs(score) >= DATAGEN_BASELINE_WIN_SCORE) {
                ++win_count;
                draw_count = 0;
            } else if (std::abs(score) <= DATAGEN_BASELINE_DRAW_SCORE) {
                win_count = 0;
                ++draw_count;
            } else {
                win_count = 0;
                draw_count = 0;
            }

            if (std::abs(score) >= MATE_FOUND || win_count > DATAGEN_MAX_CONSECUTIVE_WIN_SCORES)
                game_result = (score > 0 ? "1.0" : "0.0");
            else if (td.position.get_fifty_move_ply() > 40 || draw_count > DATAGEN_MAX_CONSECUTIVE_DRAW_SCORES)
                game_result = "0.5";

            if (!game_result.empty())
                break;

            if (!td.best_move.is_noisy() && !in_check && !stop_flag)
                collected_fens.push_back({td.position.get_fen(), score});

            td.position.make_move<true>(td.best_move);
        }

        if (!game_result.empty())
            for (FenData& data : collected_fens)
                file_out << data.string_out(FenData::Format::TXT, game_result);

        collected_fens.clear();
    }

    void init_pos_randomly() {
        td.position.set_fen<true>(book.random_fen());
        td.search_history.reset();

        for (int i = 0; i < DATAGEN_OPENING_RANDOM_MOVES; ++i) {
            ScoredMove moves[MAX_MOVES_PER_POS];
            ScoredMove* end = gen_moves(moves, td.position, MoveGenType::GEN_ALL);

            std::vector<Move> legal_moves;
            for (auto curr = moves; curr != end; ++curr) {
                if (td.position.make_move<false>(curr->move))
                    legal_moves.push_back(curr->move);

                td.position.unmake_move<false>(curr->move);
            }

            if (!legal_moves.empty()) {
                int random_idx = rand(0, legal_moves.size() - 1);
                td.position.make_move<true>(legal_moves[random_idx]);
            } else {
                td.position.set_fen<true>(book.random_fen());
                i = -1; // increment is happening after the loop, so this will be 0
            }
        }
    }

    ThreadData td;

    int id;
    int game_count;
    int fen_count;
    bool stop_flag;
    const OpeningBook& book;
    const SearchLimits& search_limits;

    std::ofstream file_out;
    std::vector<FenData> collected_fens;
};

class DatagenEngine {
  public:
    void datagen_loop(const SearchLimits& limits, const std::string book_path, int thread_count, int tt_size_mb) {
        OpeningBook book(book_path);
        start(limits, book, thread_count, tt_size_mb);
        std::cout << "Datagen started with " << thread_count << " thread!" << std::endl;

        start_time = now();
        std::string input, command;
        while (getline(std::cin, input)) {
            std::istringstream iss(input);
            iss >> command;

            if (command == "stop") {
                std::cout << "Stopping... You will wait, at most, " << DATAGEN_REPORT_INTERVAL / 1000 << " seconds"
                          << std::endl;
                break;
            } else if (command == "pause") {
                std::cout << "Pausing... You will wait, at most, " << DATAGEN_REPORT_INTERVAL / 1000 << " seconds"
                          << std::endl;
                stop();
                std::cout << "Datagen paused" << std::endl;

            } else if (command == "resume") {
                run();
                std::cout << "Datagen resumed" << std::endl;
            }
        }

        stop();
        report();

        std::cout << "Datagen ran successfully!\n";
    }

  private:
    void report() const {
        constexpr char line[] = "+------------+------------+------------+------------+------------+\n";

        TimeType elapsed_time = now() - start_time;

        auto print_info_line = [elapsed_time](std::string id, int game_count, int fen_count) {
            std::cout << "|";
            std::cout << std::setw(11) << std::right << id << " |";
            std::cout << std::setw(11) << std::right << game_count << " |";
            std::cout << std::setw(11) << std::right << fen_count << " |";
            std::cout << std::setw(11) << std::right << game_count * 1000 / elapsed_time << " |";
            std::cout << std::setw(11) << std::right << fen_count * 1000 / elapsed_time << " |";
            std::cout << "\n";
        };

        std::cout << line;
        std::cout << "| thread id  | game count | fen count  |  games/s   |   fens/s   |\n";
        std::cout << line;

        int game_count = 0;
        int fen_count = 0;
        for (const DatagenThread& dt_thread : datagen_threads) {
            print_info_line(std::to_string(dt_thread.get_id()), dt_thread.get_game_count(), dt_thread.get_fen_count());

            fen_count += dt_thread.get_fen_count();
            game_count += dt_thread.get_game_count();
        }

        std::cout << line;
        print_info_line("total", game_count, fen_count);
        std::cout << line;
    }

    void start(const SearchLimits& limits, const OpeningBook& book, int thread_count, int tt_size_mb) {
        stop_flag = false;
        for (int i = 0; i < thread_count; ++i) {
            datagen_threads.emplace_back(limits, i, book);
            threads.emplace_back(&DatagenThread::start, &datagen_threads[i], tt_size_mb);
        }

        run_report_thread();
    }

    void run_report_thread() { report_thread = std::thread(&DatagenEngine::report_loop, this); }

    void report_loop() const {
        while (!stop_flag) {
            std::this_thread::sleep_for(std::chrono::milliseconds(DATAGEN_REPORT_INTERVAL));
            if (!stop_flag) // May have stopped while sleeping
                report();
        }
    }

    void run() {
        if (!stop_flag)
            return;

        stop_flag = false;
        for (unsigned int i = 0; i < threads.size(); ++i)
            threads[i] = std::thread(&DatagenThread::run, &datagen_threads[i]);

        run_report_thread();
    }

    void stop() {
        if (stop_flag)
            return;

        stop_flag = true;
        for (DatagenThread& dt_thread : datagen_threads)
            dt_thread.stop();

        for (auto& thread : threads)
            thread.join();

        report_thread.join();
    }

    bool stop_flag = true;
    TimeType start_time;

    std::vector<DatagenThread> datagen_threads;
    std::vector<std::thread> threads;
    std::thread report_thread;
};

#endif // !DATAGEN_H
#endif // DATAGEN_BUILD
