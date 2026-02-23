/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include <cstdlib>
#include <optional>

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
        if (argc < 4) {
            std::cerr << "usage: " << argv[0] << " datagen <threads> <output_directory> [syzygy_path]\n";
            return EXIT_FAILURE;
        }

        int concurrency = std::stoi(argv[2]);
        std::string directory = std::string(argv[3]);
        const auto syzygy_path = [&]() -> std::optional<std::string> {
            if (argc > 4)
                return {std::string(argv[4])};
            return {};
        }();

        DatagenEngine dt_engine;
        dt_engine.datagen_loop(concurrency, EngineOptions::HASH_DEFAULT, directory, syzygy_path);
    } else {
        UCI uci;
        uci.loop();
    }

    return EXIT_SUCCESS;
}
