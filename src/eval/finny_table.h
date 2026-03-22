/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef FINNY_TABLE_H
#define FINNY_TABLE_H

#include <array>
#include <cstdint>
#include <cstring>
#include <span>

#include "../types.h"
#include "arch.h"

class Position;

struct alignas(64) PovAccumulator {
    std::array<int16_t, HIDDEN_LAYER_SIZE> neurons;

    PovAccumulator(std::span<const int16_t, HIDDEN_LAYER_SIZE> bias) { reset(bias); }
    PovAccumulator() = delete;
    ~PovAccumulator() = default;

    inline void reset(std::span<const int16_t, HIDDEN_LAYER_SIZE> bias) {
        memcpy(neurons.data(), bias.data(), bias.size_bytes());
    }
};

class FinnyTable {
  public:
    FinnyTable() { reset(); };

    void reset();
    PovAccumulator update(const Position &pos, const Color side);

  private:
    struct FinnyTableCache {
        std::array<Bitboard, 12> occupancies;
        PovAccumulator accumulator;

        FinnyTableCache() : accumulator(network.hidden_bias) { reset(); };
        ~FinnyTableCache() = default;

        void reset();
    };

    FinnyTableCache &get_cache(const Color side, const bool flip, const size_t king_bucket);

    std::array<std::array<std::array<FinnyTableCache, NUM_KING_BUCKETS>, 2>, 2> cache; // [side][flip][king_bucket_idx]
};

#endif // #ifndef FINNY_TABLE_H
