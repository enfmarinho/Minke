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

#include "nnue.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "../incbin.h"
#include "../position.h"
#include "../types.h"
#include "accumulator.h"
#include "simd.h"

[[maybe_unused]] static inline int32_t crelu(const int32_t &input) { return std::clamp(input, CRELU_MIN, CRELU_MAX); }

[[maybe_unused]] static inline int32_t screlu(const int32_t &input) {
    const int32_t crelu_out = crelu(input);
    return crelu_out * crelu_out;
}

void NNUE::init(const Position &position) {
    m_accumulators.clear();
    m_accumulators.emplace_back(Accumulator{position});
}

void NNUE::apply_move(const DirtyPiece &dp) { m_accumulators.emplace_back(Accumulator{dp}); }

void NNUE::unapply_move() { m_accumulators.pop_back(); }

ScoreType NNUE::eval(const Position &pos) {
    update();
    assert(m_accumulators.back() == Accumulator(pos));
    PovAccumulator stm = m_accumulators.back().pov(pos.get_stm());
    PovAccumulator ntm = m_accumulators.back().pov(pos.get_adversary());
    return flatten_screlu_and_affine(stm.neurons(), ntm.neurons());
}

void NNUE::update() {
    pov_update(WHITE);
    pov_update(BLACK);
}

void NNUE::pov_update(const Color pov) {
    auto head = m_accumulators.rbegin();

    if (head->updated(pov))
        return;

    assert(m_accumulators.size() > 1);

    for (auto iter = m_accumulators.rbegin() + 1; iter != m_accumulators.rend(); ++iter) {
        if (iter->updated(pov)) {
            while (iter != head) {
                (iter - 1)->update(pov, iter->pov(pov));
                --iter;
            }
            break;
        }
    }

    assert(head->updated(pov));
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
