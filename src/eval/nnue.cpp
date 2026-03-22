/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "nnue.h"

#include <algorithm>
#include <cstdint>

#include "../incbin.h"
#include "../position.h"
#include "../types.h"
#include "arch.h"
#include "finny_table.h"

void NNUE::reset() { finny_table.reset(); }

[[maybe_unused]] static int32_t screlu(const int32_t &input) {
    const int32_t crelu_out = std::clamp(input, CRELU_MIN, CRELU_MAX);
    return crelu_out * crelu_out;
}

ScoreType NNUE::eval(const Position &pos) {
    int32_t sum =
        finny_table.accumulate_activate_affine(pos, pos.get_stm(), &network.output_weights[0]) +
        finny_table.accumulate_activate_affine(pos, pos.get_adversary(), &network.output_weights[HIDDEN_LAYER_SIZE]);

    return std::clamp((sum / QA + network.output_bias) * SCALE / QAB, -MATE_FOUND + 1, MATE_FOUND - 1);
}
