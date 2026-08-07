/*
 *  Minke is a UCI chess engine
 *  Copyright (C) 2026 Eduardo Marinho <eduardomarinho@pm.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "eval/nnue/finny_table.h"

#include "core/position.h"
#include "core/types.h"
#include "eval/nnue/accumulator.h"
#include "eval/nnue/arch.h"
#include "eval/nnue/pov_accumulator.h"
#include "utils/utils.h"

void FinnyTable::reset() {
    // Reset all cached accumulators
    for (auto &flip_buckets : cache)
        for (auto &king_buckets : flip_buckets)
            for (auto &side_buckets : king_buckets)
                side_buckets.reset();
}

const PovAccumulator &FinnyTable::update(const Position &pos, const Color pov) {
    const Square king_sq = pos.get_king_placement(pov);
    FinnyTableCache &cached_entry = get_cache(should_flip(king_sq), king_bucket_idx(king_sq, pov), pov);

    for (size_t color_idx = WHITE; color_idx <= BLACK; ++color_idx) {
        const Color color = static_cast<Color>(color_idx);
        for (size_t pt_idx = PAWN; pt_idx <= KING; ++pt_idx) {
            const PieceType pt = static_cast<PieceType>(pt_idx);
            const Piece piece = get_piece(pt, color);
            const Bitboard piece_bb = cached_entry.pt_bb[pt_idx] & cached_entry.color_bb[color_idx];

            Bitboard added = ~piece_bb & pos.get_piece_bb(piece);
            Bitboard removed = piece_bb & ~pos.get_piece_bb(piece);

            // fused updates
            while (added && removed) {
                const Square add_sq = poplsb(added);
                const Square sub_sq = poplsb(removed);
                cached_entry.pov_accumulator.self_add_sub(feature_idx(piece, add_sq, king_sq, pov),
                                                          feature_idx(piece, sub_sq, king_sq, pov));
            }

            while (added) {
                const Square sq = poplsb(added);
                cached_entry.pov_accumulator.self_add(feature_idx(piece, sq, king_sq, pov));
            }

            while (removed) {
                const Square sq = poplsb(removed);
                cached_entry.pov_accumulator.self_sub(feature_idx(piece, sq, king_sq, pov));
            }
        }
    }

    // update cached bbs
    for (size_t color_idx = WHITE; color_idx <= BLACK; ++color_idx) {
        cached_entry.color_bb[color_idx] = pos.get_occupancy(static_cast<Color>(color_idx));
    }
    for (size_t pt_idx = PAWN; pt_idx <= KING; ++pt_idx) {
        cached_entry.pt_bb[pt_idx] = pos.get_piece_bb(static_cast<PieceType>(pt_idx));
    }

    assert(cached_entry.pov_accumulator == PovAccumulator(pos, pov));

    return cached_entry.pov_accumulator;
}

void FinnyTable::FinnyTableCache::reset() {
    for (Bitboard &bb : pt_bb) {
        bb = 0;
    }
    for (Bitboard &bb : color_bb) {
        bb = 0;
    }
    pov_accumulator.reset();
}

FinnyTable::FinnyTableCache &FinnyTable::get_cache(const bool flip, const size_t king_bucket, const Color side) {
    return cache[flip][king_bucket][side];
}
