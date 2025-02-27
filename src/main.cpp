/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include <cstdlib>

#include "init.h"
#include "uci.h"

int main(int argc, char *argv[]) {
    init_all();
    UCI uci(argc, argv);
    uci.loop();
    return EXIT_SUCCESS;
}
