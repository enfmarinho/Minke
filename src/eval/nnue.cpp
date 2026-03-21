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

ScoreType NNUE::flatten_screlu_and_affine(const std::array<int16_t, HIDDEN_LAYER_SIZE> &stm,
                                          const std::array<int16_t, HIDDEN_LAYER_SIZE> &ntm) const {
#ifdef USE_SIMD
    vepi32 sum_vec = vepi32_zero();
    for (int i = 0; i < HIDDEN_LAYER_SIZE; i += REGISTER_SIZE) {
        vepi16 player_weights_vec = vepi16_load(&network.output_weights[i]);
        vepi16 player_vec = vepi16_load(&stm[i]);

        player_vec = vepi16_clamp(player_vec, QZERO, QONE);
        vepi32 player_product = vepi16_madd(vepi16_mult(player_vec, player_weights_vec), player_vec);
        sum_vec = vepi32_add(sum_vec, player_product);

        vepi16 adversary_weights_vec = vepi16_load(&network.output_weights[i + HIDDEN_LAYER_SIZE]);
        vepi16 adversary_vec = vepi16_load(&ntm[i]);

        adversary_vec = vepi16_clamp(adversary_vec, QZERO, QONE);
        vepi32 adversary_product = vepi16_madd(vepi16_mult(adversary_vec, adversary_weights_vec), adversary_vec);
        sum_vec = vepi32_add(sum_vec, adversary_product);
    }

    int32_t sum = vepi32_reduce_add(sum_vec);

#else

    int32_t sum = 0;
    for (int neuron_index = 0; neuron_index < HIDDEN_LAYER_SIZE; ++neuron_index) {
        sum += screlu(player[neuron_index]) * network.output_weights[neuron_index];
        sum += screlu(adversary[neuron_index]) * network.output_weights[neuron_index + HIDDEN_LAYER_SIZE];
    }

#endif

    sum = (sum / QA + network.output_bias) * SCALE / QAB;
    return std::clamp(sum, -MATE_FOUND + 1, MATE_FOUND - 1);
}

ScoreType NNUE::eval(const Position &pos) {
    auto stm = finny_table.update(pos, pos.get_stm()).neurons;
    auto ntm = finny_table.update(pos, pos.get_adversary()).neurons;
    return flatten_screlu_and_affine(stm, ntm);
}
