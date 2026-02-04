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

    KeyType key() const { return m_key; }
    IndexType depth() const { return m_depth; }
    Move best_move() const { return m_best_move; }
    ScoreType score() const { return m_score; }
    ScoreType eval() const { return m_eval; }
    IndexType bound() const { return m_age_bound & BOUND_MASK; }
    IndexType age() const { return (m_age_bound & AGE_MASK) >> 2; }
    void save(const HashType &hash, const IndexType &depth, const Move &best_move, const ScoreType &score,
              const ScoreType &eval, const IndexType age, const BoundType &bound);
    void reset();

  private:
    static constexpr IndexType BOUND_MASK = 0b0000'0011;
    static constexpr IndexType AGE_MASK = 0b1111'1100;
    static constexpr IndexType AGE_OFFSET = 2;

    KeyType m_key;         // 2 bytes
    Move m_best_move;      // 2 bytes
    ScoreType m_score;     // 2 bytes
    ScoreType m_eval;      // 2 bytes
    IndexType m_depth;     // 1 byte
    IndexType m_age_bound; // 1 byte: 2 lower bits is for bound, others for AGE_OFFSET
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
    IndexType age() { return m_age; }
    void update_age() { m_age = (m_age + 1) & AGE_MASK; }
    void prefetch(const HashType &key);
    void resize(size_t MB);
    void clear();
    int tt_size_mb() const { return size_mb; }

  private:
    static constexpr IndexType MAX_AGE = 1 << 6;
    static constexpr IndexType AGE_MASK = MAX_AGE - 1;

    size_t size_mb{0};
    size_t m_table_size{0};
    size_t m_table_mask{0};
    IndexType m_age{0};
    std::unique_ptr<TTBucket[]> m_table;
};

#endif // #ifndef TT_H
