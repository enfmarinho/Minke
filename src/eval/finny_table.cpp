/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "finny_table.h"

#include "../position.h"
#include "arch.h"

static size_t king_bucket(const Square king_sq, const Color side) {
    const size_t pov_king_sq = side == WHITE ? king_sq : king_sq ^ 56;
    return KING_BUCKETS[pov_king_sq];
}

static bool should_flip(const Square king_sq) {
    return false; // STUB
    return get_file(king_sq) > 3;
}

static size_t feature_idx(const Piece piece, const Square sq, const Color stm, const Square king_sq, const bool flip) {
    constexpr size_t COLOR_STRIDE = 64 * 6;
    constexpr size_t PIECE_STRIDE = 64;

    const Color piece_color = get_color(piece);
    int pov_sq = sq;
    if (stm == BLACK) // Convert square to pov, i.e. flip rank if stm is black
        pov_sq = pov_sq ^ 56;
    if (flip) // Flip file
        pov_sq = pov_sq ^ 7;

    size_t idx = king_bucket(king_sq, stm) * INPUT_LAYER_SIZE +      // Input bucket offset
                 (piece_color != stm) * COLOR_STRIDE +               // Perspective offset
                 get_piece_type(piece, piece_color) * PIECE_STRIDE + // Piece type offset
                 pov_sq;
    return idx * HIDDEN_LAYER_SIZE;
}

void FinnyTable::reset() {
    // Reset all cached accumulators
    for (auto &stm_buckets : cache)
        for (auto &flip_buckets : stm_buckets)
            for (auto &king_buckets : flip_buckets)
                king_buckets.reset();
}

PovAccumulator FinnyTable::update(const Position &pos, const Color side) {
    const Square king_sq = pos.get_king_placement(side);
    const bool flip = should_flip(king_sq);

    FinnyTableCache &cached_entry = get_cache(king_sq, side);

    size_t add[32], sub[32];
    size_t add_count = 0, sub_count = 0;
    for (size_t piece_idx = WHITE_PAWN; piece_idx <= BLACK_KING; ++piece_idx) {
        const Piece piece = static_cast<Piece>(piece_idx);
        Bitboard added = pos.get_piece_bb(piece) & ~cached_entry.occupancies[piece_idx];
        Bitboard removed = cached_entry.occupancies[piece_idx] & ~pos.get_piece_bb(piece);
        while (added) {
            Square sq = poplsb(added);
            add[add_count++] = feature_idx(piece, sq, side, king_sq, flip);
        }

        while (removed) {
            Square sq = poplsb(removed);
            sub[sub_count++] = feature_idx(piece, sq, side, king_sq, flip);
        }

        cached_entry.occupancies[piece_idx] = pos.get_piece_bb(piece);
    }

    for (size_t i = 0; i < add_count; ++i) {
        for (size_t column = 0; column < HIDDEN_LAYER_SIZE; ++column) {
            cached_entry.accumulator.neurons[column] += network.hidden_weights[add[i] + column];
        }
    }

    for (size_t i = 0; i < sub_count; ++i) {
        for (size_t column = 0; column < HIDDEN_LAYER_SIZE; ++column) {
            cached_entry.accumulator.neurons[column] -= network.hidden_weights[sub[i] + column];
        }
    }

    return cached_entry.accumulator;
}

void FinnyTable::FinnyTableCache::reset() {
    for (Bitboard &occ : occupancies) {
        occ = 0;
    }
    accumulator.reset(network.hidden_bias);
}

FinnyTable::FinnyTableCache &FinnyTable::get_cache(Square king_sq, Color side) {
    return cache[side][should_flip(king_sq)][king_bucket(king_sq, side)];
}
