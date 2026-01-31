/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef TT_H
#define TT_H

#include <array>
#include <cstddef>
#include <memory>

#include "position.h"
#include "types.h"

class TTEntry {
  public:
    TTEntry() = default;
    ~TTEntry() = default;

    HashType hash() const { return m_hash; }
    IndexType depth() const { return m_depth; }
    Move best_move() const { return m_best_move; }
    ScoreType score() const { return m_score; }
    BoundType bound() const { return m_bound; }
    void save(const HashType &hash, const IndexType &depth, const Move &best_move, const ScoreType &evaluation,
              const CounterType &half_move_counter, const BoundType &bound);
    void reset();

  private:
    HashType m_hash;                 // 8 bytes
    Move m_best_move;                // 2 bytes
    ScoreType m_score;               // 2 bytes
    IndexType m_depth;               // 1 byte
    BoundType m_bound = BOUND_EMPTY; // 1 byte
};

class TranspositionTable {
    constexpr static IndexType BUCKET_SIZE = 4;
    struct TTBucket {
        TTBucket() = default;
        ~TTBucket() = default;
        std::array<TTEntry, BUCKET_SIZE> entry;
    };

    static_assert(sizeof(TTEntry) == 16, "TTEntry is not 16 bytes");
    static_assert(sizeof(TTBucket) == 64, "TTBucket is not 64 bytes");

  public:
    TranspositionTable() = default;
    ~TranspositionTable() = default;
    TranspositionTable(const TranspositionTable &) = delete;
    TranspositionTable &operator=(const TranspositionTable &) = delete;

    /*!
     * Search the transposition table for the TTEntry with position hash and sets
     * "found" to true if it was found, otherwise sets "found" to false and
     * returns a pointer to the TTEntry with the least value, i.e. the one that
     * should be replaced.
     */
    TTEntry *probe(const Position &position, bool &found);
    void prefetch(const HashType &key);
    void resize(size_t MB);
    void clear();
    int tt_size_mb() const { return size_mb; }

  private:
    size_t size_mb{0};
    size_t m_table_size{0};
    size_t m_table_mask{0};
    std::unique_ptr<TTBucket[]> m_table;
};

#endif // #ifndef TT_H
