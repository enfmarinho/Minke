/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "tt.h"

#include <cstddef>
#include <cstdlib>
#include <iostream>

#include "move.h"
#include "position.h"
#include "types.h"
#include "utils.h"

inline static KeyType key_from_hash(const HashType &hash) { return static_cast<KeyType>(hash >> 48); }

void TTEntry::store(const HashType &hash, const IndexType &depth, const Move &best_move, const ScoreType &score,
                    const ScoreType &eval, const BoundType &bound, const bool was_pv, const bool &tthit) {
    if (best_move != MOVE_NONE || !tthit)
        m_best_move = best_move;

    m_key = key_from_hash(hash);
    m_depth = depth;
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

size_t TranspositionTable::table_index_from_hash(const HashType hash) { return hash & m_table_mask; }

bool TranspositionTable::probe(const Position &position, TTEntry &tte) {
    size_t table_index = table_index_from_hash(position.get_hash());
    for (TTEntry &entry : m_table[table_index].entry) {
        tte = entry;
        if (entry.key() == key_from_hash(position.get_hash()))
            return true;
    }
    return false;
}

void TranspositionTable::store(const HashType &hash, const IndexType &depth, const Move &best_move,
                               const ScoreType &score, const ScoreType &eval, const BoundType &bound, const bool was_pv,
                               const bool &tthit) {
    size_t table_index = table_index_from_hash(hash);
    KeyType target_key = key_from_hash(hash);
    TTBucket *bucket = &m_table[table_index];
    TTEntry *replace = &bucket->entry[0];
    for (IndexType index = 0; index < BUCKET_SIZE; ++index) {
        if (bucket->entry[index].key() == target_key) {
            replace = &bucket->entry[index];
            break;
        }
        if (replace->depth() > bucket->entry[index].depth())
            replace = &bucket->entry[index];
    }
    replace->store(hash, depth, best_move, score, eval, bound, was_pv, tthit);
}

void TranspositionTable::prefetch(const HashType &key) {
    size_t table_index = table_index_from_hash(key);
    __builtin_prefetch(&m_table[table_index]);
}

TranspositionTable::~TranspositionTable() {
    if (m_table != nullptr)
        aligned_free(m_table);

    m_table = nullptr;
}

void TranspositionTable::resize(size_t MB) {
    if (m_table != nullptr)
        aligned_free(m_table);

    size_mb = MB;
    m_table_size = MB * 1024 * 1024 / sizeof(TTBucket);
    m_table_mask = m_table_size - 1;
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
