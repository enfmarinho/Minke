
/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef PV_LIST_H
#define PV_LIST_H

#include <array>

#include "types.h"

class PvList {
  public:
    void update(Move new_move, const PvList &list);
    void print() const;
    PvList &operator=(const PvList &other);

  private:
    std::array<Move, MaxSearchDepth> m_pv;
    CounterType m_size{0};
};

#endif // #ifndef PV_LIST_H
