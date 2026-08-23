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

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string_view>

#include "datagen/datagen.h"
#include "uci/benchmark.h"
#include "uci/init.h"
#include "uci/uci.h"

int main(int argc, char *argv[]) {
    init_all();
    if (argc > 1 && std::string_view(argv[1]) == "bench") {
        int depth = Benchmark::DEFAULT_BENCH_DEPTH;
        if (argc > 2)
            depth = std::stoi(argv[2]);

        Benchmark::run(depth);
    } else if (argc > 1 && std::string_view(argv[1]) == "datagen") {
        if (argc != 4 && argc != 5) {
            std::cerr << "usage: " << argv[0] << " datagen <threads> <output_directory> [opening_book.epd]\n";
            return EXIT_FAILURE;
        }

        int concurrency = std::stoi(argv[2]);
        std::filesystem::path directory = argv[3];
        std::optional<std::filesystem::path> opening_book = (argc == 5 ? std::optional(argv[4]) : std::nullopt);

        DatagenEngine dt_engine;
        dt_engine.datagen_loop(concurrency, directory, opening_book);
    } else {
        UCI::run();
    }

    return EXIT_SUCCESS;
}
