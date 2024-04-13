/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef UCI_H
#define UCI_H

#include "board.h"

class UCI {
public:
  UCI(int argc, char *argv[]);
  ~UCI() = default;
  void game_loop();

private:
  Board m_board;
};

#endif // #ifndef UCI_H
