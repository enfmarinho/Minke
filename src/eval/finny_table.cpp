/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "finny_table.h"

#include "../position.h"
#include "accumulator.h"
#include "arch.h"

void FinnyTable::reset() {
    // Reset all cached accumulators
    for (auto &flip_buckets : cache)
        for (auto &king_buckets : flip_buckets)
            for (auto &side_buckets : king_buckets)
                side_buckets.reset();
}

const PovAccumulator &FinnyTable::update(const Position &pos, const Color side) {
    const Square king_sq = pos.get_king_placement(side);
    const Square king_pov_sq = static_cast<Square>(side == WHITE ? king_sq : king_sq ^ 56);
    const size_t king_bucket = KING_BUCKETS_LAYOUT[king_pov_sq];
    const bool flip = should_flip(king_pov_sq);

    FinnyTableCache &cached_entry = get_cache(flip, king_bucket, side);

    for (size_t piece_idx = WHITE_PAWN; piece_idx <= BLACK_KING; ++piece_idx) {
        const Piece piece = static_cast<Piece>(piece_idx);
        Bitboard added = pos.get_piece_bb(piece) & ~cached_entry.bbs[piece_idx];
        Bitboard removed = cached_entry.bbs[piece_idx] & ~pos.get_piece_bb(piece);

        // TODO fused updates
        // while (added && removed) {
        //     const Square add0 = poplsb(added);
        //     const Square sub0 = poplsb(removed);
        //     cached_entry.pov_accumulator.self_add_sub(add0, sub0);
        // }

        while (added) {
            const Square sq = poplsb(added);
            cached_entry.pov_accumulator.self_add(feature_idx(piece, sq, side, king_bucket, flip));
        }

        while (removed) {
            const Square sq = poplsb(removed);
            cached_entry.pov_accumulator.self_sub(feature_idx(piece, sq, side, king_bucket, flip));
        }

        cached_entry.bbs[piece_idx] = pos.get_piece_bb(piece);
    }

    return cached_entry.pov_accumulator;
}

void FinnyTable::FinnyTableCache::reset() {
    for (Bitboard &bb : bbs) {
        bb = 0;
    }
    pov_accumulator.reset();
}

FinnyTable::FinnyTableCache &FinnyTable::get_cache(const bool flip, const size_t king_bucket, const Color side) {
    return cache[flip][king_bucket][side];
}
