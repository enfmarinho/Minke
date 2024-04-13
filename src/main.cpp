/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "uci.h"

int main(int argc, char *argv[]) {
  UCI uci(argc, argv);
  uci.game_loop();
  return 0;
}
