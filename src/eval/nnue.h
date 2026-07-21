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

#ifndef NNUE_H
#define NNUE_H

#include <cassert>
#include <cstdint>
#include <span>
#include <vector>

#include "../types.h"
#include "nnue/accumulator.h"
#include "nnue/arch.h"
#include "nnue/finny_table.h"
#include "nnue/pov_accumulator.h"

class Position;

// NOTE: NNUE must be initialized using refresh().
class NNUE {
  public:
    NNUE() = default;
    ~NNUE() = default;

    void refresh(const Position &pos);

    void pop();
    void push(const DirtyPiece &dp, const Square white_king_sq, const Square black_king_sq);

    ScoreType eval(const Position &pos);

  private:
    void update(const Position &pos);
    void update_pov(const Position &pos, const Color &pov);

    int32_t propagate(std::span<const int16_t, L1_SIZE> stm_inputs, std::span<const int16_t, L1_SIZE> ntm_inputs,
                      const int bucket);

    void activate_ft(std::span<const int16_t, L1_SIZE> stm_acc, std::span<const int16_t, L1_SIZE> ntm_acc,
                     std::span<uint8_t, L1_SIZE> outputs);
    void propagate_l1(int bucket, std::span<const uint8_t, L1_SIZE> inputs, std::span<int32_t, L2_SIZE> outputs);
    void propagate_l2(int bucket, std::span<const int32_t, L2_SIZE> inputs, std::span<int32_t, L3_SIZE> outputs);
    void propagate_l3(int bucket, std::span<const int32_t, L3_SIZE> inputs, int32_t &output);

    FinnyTable m_finny_table;
    std::vector<Accumulator> m_accumulators;
};

#endif // #ifndef NNUE_H
