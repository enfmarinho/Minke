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

#include "datagen/datagen.h"
#include "init.h"
#include "uci.h"

int main(int argc, char *argv[]) {
    init_all();
    if (argc > 1 && std::string(argv[1]) == "bench") {
        int depth = EngineOptions::BENCH_DEPTH;
        if (argc > 2)
            depth = std::stoi(argv[2]);

        UCI uci;
        uci.bench(depth);
    } else if (argc > 1 && std::string(argv[1]) == "datagen") {
        if (argc != 4) {
            std::cerr << "usage: " << argv[0] << " datagen <threads> <output_directory>\n";
            return EXIT_FAILURE;
        }

        int concurrency = std::stoi(argv[2]);
        std::string directory = std::string(argv[3]);

        DatagenEngine dt_engine;
        dt_engine.datagen_loop(concurrency, EngineOptions::HASH_DEFAULT, directory);
    } else {
        UCI uci;
        uci.loop();
    }

    return EXIT_SUCCESS;
}
