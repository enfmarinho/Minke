/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef PV_LIST_H
#define PV_LIST_H

#include <array>

#include "move.h"
#include "types.h"

class PvList {
  public:
    void update(Move new_move, const PvList &list);
    void print(const bool chess960, const Bitboard castle_rooks) const;
    void clear();
    PvList &operator=(const PvList &other);

  private:
    std::array<Move, MAX_SEARCH_DEPTH> m_pv;
    CounterType m_size{0};
};

#endif // #ifndef PV_LIST_H
