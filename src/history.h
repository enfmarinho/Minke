/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef HISTORY_H
#define HISTORY_H

#include <cassert>
#include <cstring>

#include "move.h"
#include "position.h"
#include "types.h"

class History {
  public:
    inline History() { reset(); }
    ~History() = default;

    inline void reset() {
        std::memset(search_history_table, 0, sizeof(search_history_table));
        std::memset(killer_moves, MoveNone, sizeof(killer_moves));
    };

    inline void update(const Position &position, const Move &move, int depth) {
        if (move.is_quiet())
            save_killer(move, depth);

        search_history_table[position.get_stm()][move.from_and_to()] += depth * depth;
    }

    inline int consult(const Position &position, const Move &move) const {
        return search_history_table[position.get_stm()][move.from_and_to()];
    }

    inline Move consult_killer1(const int depth) { return killer_moves[0][depth]; }
    inline Move consult_killer2(const int depth) { return killer_moves[1][depth]; }

  private:
    inline void save_killer(const Move &move, const int depth) {
        killer_moves[1][depth] = killer_moves[0][depth];
        killer_moves[0][depth] = move;
    }

    int search_history_table[ColorNB][64 * 64];
    Move killer_moves[2][MaxPly];
};
#endif // #ifndef HISTORY_H
