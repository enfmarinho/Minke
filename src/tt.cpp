/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "tt.h"
#include "game_elements.h"
#include "position.h"
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <strings.h>

CounterType TTEntry::relative_age(const CounterType &half_move_counter) const {
  return half_move_counter - m_half_move_count;
}

CounterType
TTEntry::replace_factor(const CounterType &half_move_counter) const {
  return m_depth_ply - relative_age(half_move_counter) * 2;
}

void TTEntry::save(const HashType &hash, const IndexType &depth_ply,
                   const Move &best_move, const WeightType &evaluation,
                   const CounterType &half_move_counter,
                   const BoundType &bound) {
  m_hash = hash;
  m_depth_ply = depth_ply;
  m_best_move = best_move;
  m_evaluation = evaluation;
  m_half_move_count = half_move_counter;
  m_bound = bound;
}

void TTEntry::reset() {
  m_hash = 0;
  m_depth_ply = 0;
  m_best_move = Move();
  m_evaluation = 0;
  m_half_move_count = 0;
  m_bound = BoundType::Empty;
}

TTEntry *TranspositionTable::probe(const Position &position, bool &found) {
  HashType table_index = position.get_hash() & m_table_mask;
  for (TTEntry &entry : m_table[table_index].entry) {
    if (entry.key() == position.get_hash())
      return found = true, &entry;
  }

  TTBucket *bucket = &m_table[table_index];
  TTEntry *replace = &bucket->entry[0];
  for (IndexType index = 1; index < bucket_size; ++index) {
    if (replace->replace_factor(position.get_half_move_counter()) >
        bucket->entry[index].replace_factor(position.get_half_move_counter()))
      replace = &bucket->entry[index];
  }
  return found = false, replace;
}

void TranspositionTable::resize(size_t MB) {
  m_table_size = MB * 1024 * 1024 / sizeof(TTBucket);
  m_table_mask = m_table_size - 1;
  m_table = std::make_unique<TTBucket[]>(m_table_size);
  if (!m_table) {
    std::cerr
        << "Failed to allocated required memory for the transposition table"
        << std::endl;
    exit(EXIT_FAILURE);
  }
}

void TranspositionTable::clear() {
  for (size_t index = 0; index < m_table_size; ++index) {
    for (TTEntry &entry : m_table[index].entry) {
      entry.reset();
    }
  }
}
