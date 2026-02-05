/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "tt.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "move.h"
#include "position.h"
#include "types.h"
#include "utils.h"

inline static KeyType key_from_hash(const HashType &hash) { return static_cast<KeyType>(hash); }

void TTEntry::save(const HashType &hash, const IndexType &depth, const Move &best_move, const ScoreType &score,
                   const ScoreType &eval, const BoundType &bound, const bool was_pv) {
    m_key = key_from_hash(hash);
    m_depth = depth;
    m_best_move = best_move;
    m_score = score;
    m_eval = eval;
    m_pv_bound = (was_pv << 2) + bound;
}

void TTEntry::reset() {
    m_key = 0;
    m_depth = 0;
    m_best_move = MOVE_NONE;
    m_score = SCORE_NONE;
    m_eval = SCORE_NONE;
    m_pv_bound = 0;
}

size_t TranspositionTable::table_index_from_hash(const HashType hash) {
    using u128 = unsigned __int128;
    return static_cast<uint64_t>((static_cast<u128>(hash) * static_cast<u128>(m_table_size)) >> 64);
}

TTEntry *TranspositionTable::probe(const Position &position, bool &found) {
    size_t table_index = table_index_from_hash(position.get_hash());
    for (TTEntry &entry : m_table[table_index].entry) {
        if (entry.key() == key_from_hash(position.get_hash()))
            return found = true, &entry;
    }

    TTBucket *bucket = &m_table[table_index];
    TTEntry *replace = &bucket->entry[0];
    for (IndexType index = 1; index < BUCKET_SIZE; ++index) {
        if (replace->depth() > bucket->entry[index].depth())
            replace = &bucket->entry[index];
    }
    return found = false, replace;
}

void TranspositionTable::prefetch(const HashType &key) {
    size_t table_index = table_index_from_hash(key);
    __builtin_prefetch(&m_table[table_index]);
}

void TranspositionTable::resize(size_t MB) {
    if (m_table != nullptr)
        aligned_free(m_table);

    size_mb = MB;
    m_table_size = MB * 1024 * 1024 / sizeof(TTBucket);
    m_table = static_cast<TTBucket *>(aligned_malloc(sizeof(TTBucket), m_table_size * sizeof(TTBucket)));
    if (!m_table) {
        std::cerr << "Failed to allocated required memory for the transposition table" << std::endl;
        exit(EXIT_FAILURE);
    }

    clear();
}

void TranspositionTable::clear() {
    for (size_t index = 0; index < m_table_size; ++index) {
        for (TTEntry &entry : m_table[index].entry) {
            entry.reset();
        }
    }
}
