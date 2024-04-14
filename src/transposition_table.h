/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include "game_elements.h"

class TranspositionTable {
public:
  TranspositionTable() {}
  // TODO
private:
  int m_key;
  IndexType m_depth;
  Movement m_movement;
  WeightType m_evaluation;
};

#endif // #ifndef TRANSPOSITION_TABLE_H
