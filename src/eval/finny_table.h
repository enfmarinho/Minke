/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef FINNY_TABLE_H
#define FINNY_TABLE_H

#include <array>
#include <cstring>

#include "../types.h"
#include "accumulator.h"
#include "arch.h"

class Position;

class FinnyTable {
  public:
    FinnyTable() { reset(); };
    ~FinnyTable() = default;

    void reset();

    const PovAccumulator &update(const Position &pos, const Color side);

    inline const PovAccumulator &consult(const Color side, const Square king_sq) {
        const Square king_pov_sq = static_cast<Square>(side == WHITE ? king_sq : king_sq ^ 56);
        return get_cache(should_flip(king_pov_sq), KING_BUCKETS_LAYOUT[king_pov_sq], side).pov_accumulator;
    };

  private:
    struct FinnyTableCache {
        std::array<Bitboard, 12> bbs; // [piece_type]
        PovAccumulator pov_accumulator;

        FinnyTableCache() { reset(); };
        ~FinnyTableCache() = default;

        void reset();
    };

    FinnyTableCache &get_cache(const bool flip, const size_t king_bucket, const Color side);

    std::array<std::array<std::array<FinnyTableCache, 2>, NUM_KING_BUCKETS>, 2> cache; // [flip][king_bucket_idx][side]
};

#endif // #ifndef FINNY_TABLE_H
