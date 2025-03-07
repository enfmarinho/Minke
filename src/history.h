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
    inline History() { init(); }
    ~History() = default;

    inline void init() { std::memset(search_history_table, 0, sizeof(search_history_table)); };

    inline void update(const Position &position, const Move &move, int depth) {
        search_history_table[position.get_stm()][move.from_and_to()] += depth * depth;
    }

    inline int consult(const Position &position, const Move &move) const {
        return search_history_table[position.get_stm()][move.from_and_to()];
    }

  private:
    int search_history_table[ColorNB][64 * 64];
};
#endif // #ifndef HISTORY_H
