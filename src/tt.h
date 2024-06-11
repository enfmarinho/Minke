/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef TT_H
#define TT_H

#include "game_elements.h"
#include "position.h"
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

  HashType key() const { return m_hash; }
  IndexType depth_ply() const { return m_depth_ply; }
  Move best_move() const { return m_best_move; }
  WeightType evaluation() const { return m_evaluation; }
  BoundType bound() const { return m_bound; }
  CounterType relative_age(const CounterType &half_move_count) const;
  CounterType replace_factor(const CounterType &half_move_count) const;
  void save(const HashType &hash, const IndexType &depth_ply,
            const Move &movement, const WeightType &evaluation,
            const CounterType &half_move_counter, const BoundType &bound);
  void reset() { m_bound = BoundType::Empty; }

private:
  HashType m_hash;                      // 8 bytes
  IndexType m_depth_ply;                // 1 byte
  Move m_best_move;                     // 3 bytes
  int16_t m_evaluation;                 // 2 bytes // TODO change to WeightType
  IndexType m_half_move_count;          // 1 byte
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
  static TranspositionTable &get() {
    static TranspositionTable instance;
    return instance;
  }
  TranspositionTable(const TranspositionTable &) = delete;
  TranspositionTable &operator=(const TranspositionTable &) = delete;

  /*!
   * Search the transposition table for the TTEntry with position hash and sets
   * "found" to true if it was found, otherwise sets "found" to false and
   * returns a pointer to the TTEntry with the least value, i.e. the one that
   * should be replaced.
   */
  TTEntry *probe(const Position &position, bool &found);
  void resize(size_t MB);
  void clear();

private:
  TranspositionTable() = default;
  ~TranspositionTable() = default;

  size_t m_table_size{0};
  size_t m_table_mask{0};
  std::unique_ptr<TTBucket[]> m_table;
};

#endif // #ifndef TT_H
