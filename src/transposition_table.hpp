/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include "game_elements.h"
#include "hashing.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

class TTEntry {
public:
  enum class BoundType : char {
    Empty,
    Exact,
    LowerBound,
    UpperBound,
  };

  HashType key() const { return m_key; }
  IndexType depth() const { return m_depth; }
  Movement movement() const { return m_best_movement; }
  WeightType evaluation() const { return m_evaluation; }
  BoundType bound() const { return m_bound; }
  int relative_age(IndexType half_move_count) const;
  int replace_factor(const IndexType &half_move_count) const;
  void save(const HashType &key, const IndexType &depth,
            const Movement &movement, const WeightType &evaluation,
            const IndexType &half_move, const BoundType &bound);
  void reset() { m_bound = BoundType::Empty; }

private:
  HashType m_key;           // 8 bytes
  int8_t m_depth;           // 1 byte // TODO change to indextype
  Movement m_best_movement; // 3 bytes
  int16_t m_evaluation;     // 2 bytes // TODO change to WeightType
  int8_t m_half_move_count; // 1 byte // TOOD check if it can be CounterType
  BoundType m_bound = BoundType::Empty; // 1 byte
};

class TranspositionTable {
  constexpr static IndexType bucket_size = 4;
  struct TTBucket {
    std::array<TTEntry, bucket_size> entry;
  };

  static_assert(sizeof(TTEntry) == 16, "TTEntry is not 16 bytes");
  static_assert(sizeof(TTBucket) == 64, "TTBucket is not 64 bytes");

public:
  static TranspositionTable &get_instance() {
    static TranspositionTable instance;
    return instance;
  }
  TranspositionTable(const TranspositionTable &) = delete;
  TranspositionTable &operator=(const TranspositionTable &) = delete;

  /*!
   * Search the transposition table for the TTEntry with "key" and sets "found"
   * to true if it was found, otherwise sets "found" to false and returns a
   * pointer to the TTEntry with the least value, i.e. the one that should be
   * replaced.
   */
  TTEntry *probe(const HashType &key, bool &found,
                 const IndexType &half_move_count);
  void resize(size_t MB);
  void clear();

private:
  TranspositionTable() = default;
  ~TranspositionTable() = default;

  size_t m_table_size{0};
  size_t m_table_mask{0};
  std::unique_ptr<TTBucket[]> m_table;
};

#endif // #ifndef TRANSPOSITION_TABLE_H
