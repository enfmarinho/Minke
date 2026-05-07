/*
 *  Minke is a UCI chess engine
 *  Copyright (C) 2026 Eduardo Marinho <eduardomarinho@pm.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef FINNY_TABLE_H
#define FINNY_TABLE_H

#include <array>
#include <cstring>

#include "../../types.h"
#include "accumulator.h"
#include "arch.h"

class Position;

class FinnyTable {
  public:
    FinnyTable() { reset(); };
    ~FinnyTable() = default;

    void reset();

    const PovAccumulator &update(const Position &pos, const Color pov);

  private:
    struct FinnyTableCache {
        std::array<Bitboard, 12> bbs; // [piece_type]
        PovAccumulator pov_accumulator;

        FinnyTableCache() { reset(); };
        ~FinnyTableCache() = default;

        void reset();
    };

    FinnyTableCache &get_cache(const bool flip, const size_t king_bucket, const Color pov);

    std::array<std::array<std::array<FinnyTableCache, 2>, NUM_KING_BUCKETS>, 2>
        cache; // [flip][king_bucket_idx][pov_color]
};

#endif // #ifndef FINNY_TABLE_H
