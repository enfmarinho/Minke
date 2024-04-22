/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "transposition_table.hpp"
#include "game_elements.hpp"
#include "hashing.hpp"
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <strings.h>

IndexType TTEntry::relative_age(IndexType half_move_counter) const {
  return half_move_counter - m_half_move_count;
}

int TTEntry::replace_factor(const IndexType &half_move_counter) const {
  return int(m_depth) - int(relative_age(half_move_counter)) * 2;
}

void TTEntry::save(const HashType &key, const IndexType &depth,
                   const Movement &movement, const WeightType &evaluation,
                   const IndexType &half_move, const BoundType &bound) {
  m_key = key;
  m_depth = depth;
  m_best_movement = movement;
  m_evaluation = evaluation;
  m_half_move_count = half_move;
  m_bound = bound;
}

TTEntry *TranspositionTable::probe(const HashType &key, bool &found,
                                   const IndexType &half_move_counter) {
  HashType table_index = key & m_table_mask;
  for (TTEntry &entry : m_table[table_index].entry) {
    if (entry.key() == key) {
      return found = true, &entry;
    }
  }

  TTBucket *bucket = &m_table[table_index];
  TTEntry *replace = &(bucket->entry[0]);
  for (IndexType index = 0; index < bucket_size; ++index) {
    if (replace->replace_factor(half_move_counter) >
        bucket->entry[index].replace_factor(half_move_counter)) {
      replace = &m_table[table_index].entry[index];
    }
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
