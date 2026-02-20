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

#include "position.h"
#include "types.h"
#include "utils.h"

class TTEntry {
  public:
    TTEntry() = default;
    ~TTEntry() = default;

    KeyType key() const { return m_key; }
    IndexType depth() const { return m_depth; }
    Move best_move() const { return m_best_move; }
    ScoreType score() const { return m_score; }
    ScoreType eval() const { return m_eval; }
    IndexType bound() const { return m_pv_bound & BOUND_MASK; }
    bool was_pv() const { return m_pv_bound & PV_MASK; }
    void save(const HashType &hash, const IndexType &depth, const Move &best_move, const ScoreType &score,
              const ScoreType &eval, const BoundType &bound, const bool was_pv, const bool &tthit);
    void reset();

  private:
    static constexpr IndexType BOUND_MASK = 0b0000'0011;
    static constexpr IndexType PV_MASK = 0b0000'0100;

    KeyType m_key;        // 2 bytes
    Move m_best_move;     // 2 bytes
    ScoreType m_score;    // 2 bytes
    ScoreType m_eval;     // 2 bytes
    IndexType m_depth;    // 1 byte
    IndexType m_pv_bound; // 1 byte
};

class TranspositionTable {
    constexpr static IndexType BUCKET_SIZE = 3;
    struct TTBucket {
        TTBucket() = default;
        ~TTBucket() = default;
        std::array<TTEntry, BUCKET_SIZE> entry;
        char padding[2];
    };

    static_assert(sizeof(TTEntry) == 10, "TTEntry is not 10 bytes");
    static_assert(sizeof(TTBucket) == 32, "TTBucket is not 32 bytes");

    int table_index_from_hash(const HashType hash);

  public:
    TranspositionTable() = default;
    ~TranspositionTable();
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
    TTBucket *m_table{nullptr};
};

#endif // #ifndef TT_H
