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
#include <memory>

#include "move.h"
#include "position.h"
#include "types.h"

CounterType TTEntry::relative_age(const CounterType &half_move_counter) const {
    return half_move_counter - m_half_move_count;
}

CounterType TTEntry::replace_factor(const CounterType &half_move_counter) const {
    return m_depth - relative_age(half_move_counter) * 2;
}

void TTEntry::save(const HashType &hash, const IndexType &depth, const Move &best_move, const ScoreType &evaluation,
                   const CounterType &half_move_counter, const BoundType &bound) {
    m_hash = hash;
    m_depth = depth;
    m_best_move = best_move;
    m_score = evaluation;
    m_half_move_count = half_move_counter;
    m_bound = bound;
}

void TTEntry::reset() {
    m_hash = 0;
    m_depth = 0;
    m_best_move = MOVE_NONE;
    m_score = 0;
    m_half_move_count = 0;
    m_bound = BOUND_EMPTY;
}

TTEntry *TranspositionTable::probe(const Position &position, bool &found) {
    HashType table_index = position.get_hash() & m_table_mask;
    for (TTEntry &entry : m_table[table_index].entry) {
        if (entry.hash() == position.get_hash())
            return found = true, &entry;
    }

    TTBucket *bucket = &m_table[table_index];
    TTEntry *replace = &bucket->entry[0];
    for (IndexType index = 1; index < BUCKET_SIZE; ++index) {
        if (replace->replace_factor(position.get_game_ply()) >
            bucket->entry[index].replace_factor(position.get_game_ply()))
            replace = &bucket->entry[index];
    }
    return found = false, replace;
}

void TranspositionTable::prefetch(const HashType &key) {
    HashType table_index = key & m_table_mask;
    __builtin_prefetch(&m_table[table_index]);
}

void TranspositionTable::resize(size_t MB) {
    size_mb = MB;
    m_table_size = MB * 1024 * 1024 / sizeof(TTBucket);
    m_table_mask = m_table_size - 1;
    m_table = std::make_unique<TTBucket[]>(m_table_size);
    if (!m_table) {
        std::cerr << "Failed to allocated required memory for the transposition table" << std::endl;
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
