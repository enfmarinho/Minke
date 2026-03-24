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
#include "accumulator.h"
#include "arch.h"
#include "finny_table.h"

[[maybe_unused]] static int32_t screlu(const int32_t &input) {
    const int32_t crelu_out = std::clamp(input, CRELU_MIN, CRELU_MAX);
    return crelu_out * crelu_out;
}

NNUE::NNUE() { m_acc_stack.reserve(MAX_SEARCH_DEPTH); }

void NNUE::init(const Position &pos) {
    m_finny_table.reset();
    const PovAccumulator &white_pov_acc = m_finny_table.update(pos, WHITE);
    const PovAccumulator &black_pov_acc = m_finny_table.update(pos, BLACK);

    m_acc_stack.clear();

    Accumulator &acc =
        m_acc_stack.emplace_back(Accumulator({}, pos.get_king_placement(WHITE), pos.get_king_placement(BLACK)));
    acc.refresh(WHITE, white_pov_acc);
    acc.refresh(BLACK, black_pov_acc);
}

void NNUE::apply_move(const DirtyPiece dp, const Square white_king_sq, const Square black_king_sq) {
    m_acc_stack.emplace_back(Accumulator(dp, white_king_sq, black_king_sq));
}

void NNUE::unapply_move() { m_acc_stack.pop_back(); }

void NNUE::update(const Position &pos) {
    side_relative_update(WHITE, pos);
    side_relative_update(BLACK, pos);
}

ScoreType NNUE::eval(const Position &pos) {
    update(pos);
    auto stm_neurons = m_acc_stack.back().neurons(pos.get_stm()).neurons;
    auto ntm_neurons = m_acc_stack.back().neurons(pos.get_adversary()).neurons;
    return flatten_screlu_and_affine(stm_neurons, ntm_neurons);
}

void NNUE::side_relative_update(const Color &side, const Position &pos) {
    auto head = m_acc_stack.rbegin();

    if (head->updated(side))
        return;

    for (auto iter = m_acc_stack.rbegin() + 1; iter != m_acc_stack.rend(); ++iter) {
        if (iter->needs_refresh(side, pos.get_king_placement(side))) {
            const PovAccumulator &acc = m_finny_table.update(pos, side);
            head->refresh(side, acc);
            break;
        } else if (iter->updated(side)) {
            while (iter != head) {
                (iter - 1)->update(side, pos, *iter);
                --iter;
            }
            break;
        }
    }
}

ScoreType NNUE::flatten_screlu_and_affine(const std::array<int16_t, HIDDEN_LAYER_SIZE> &stm,
                                          const std::array<int16_t, HIDDEN_LAYER_SIZE> &ntm) const {
#ifdef USE_SIMD
    vepi32 sum_vec = vepi32_zero();
    for (int i = 0; i < HIDDEN_LAYER_SIZE; i += REGISTER_SIZE) {
        vepi16 stm_weights_vec = vepi16_load(&network.output_weights[i]);
        vepi16 stm_vec = vepi16_load(&stm[i]);

        stm_vec = vepi16_clamp(stm_vec, QZERO, QONE);
        vepi32 stm_product = vepi16_madd(vepi16_mult(stm_vec, stm_weights_vec), stm_vec);
        sum_vec = vepi32_add(sum_vec, stm_product);

        vepi16 ntm_weights_vec = vepi16_load(&network.output_weights[i + HIDDEN_LAYER_SIZE]);
        vepi16 ntm_vec = vepi16_load(&ntm[i]);

        ntm_vec = vepi16_clamp(ntm_vec, QZERO, QONE);
        vepi32 ntm_product = vepi16_madd(vepi16_mult(ntm_vec, ntm_weights_vec), ntm_vec);
        sum_vec = vepi32_add(sum_vec, ntm_product);
    }

    int32_t sum = vepi32_reduce_add(sum_vec);

#else

    int32_t sum = 0;
    for (int neuron_index = 0; neuron_index < HIDDEN_LAYER_SIZE; ++neuron_index) {
        sum += screlu(stm[neuron_index]) * network.output_weights[neuron_index];
        sum += screlu(ntm[neuron_index]) * network.output_weights[neuron_index + HIDDEN_LAYER_SIZE];
    }

#endif

    sum = (sum / QA + network.output_bias) * SCALE / QAB;
    return std::clamp(sum, -MATE_FOUND + 1, MATE_FOUND - 1);
}
