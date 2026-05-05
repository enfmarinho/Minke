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
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "../incbin.h"
#include "../position.h"
#include "../types.h"
#include "nnue/pov_accumulator.h"

[[maybe_unused]] static inline constexpr int32_t crelu(const int32_t &input) {
    return std::clamp(input, CRELU_MIN, CRELU_MAX);
}

[[maybe_unused]] static inline constexpr int32_t screlu(const int32_t &input) {
    const int32_t crelu_out = crelu(input);
    return crelu_out * crelu_out;
}

void NNUE::refresh(const Position &position) {
    m_accumulators.clear();
    m_accumulators.emplace_back(position);
}

void NNUE::pop() { m_accumulators.pop_back(); }

void NNUE::push(const DirtyPiece &dp) {
    assert(!m_accumulators.empty()); // NNUE must have been initialized with 'init' method before pushing
    m_accumulators.emplace_back(dp);
}

ScoreType NNUE::eval(const Color &stm) {
    update();

    const Color ntm = static_cast<Color>(stm ^ 1);
    return flatten_screlu_and_affine(m_accumulators.back().pov(stm), m_accumulators.back().pov(ntm));
}

void NNUE::update() {
    update_pov(WHITE);
    update_pov(BLACK);
}

void NNUE::update_pov(const Color &pov) {
    if (m_accumulators.back().updated(pov))
        return;

    // Get pointer to most recently updated accumulator
    auto r_it = std::find_if(m_accumulators.rbegin(), m_accumulators.rend(),
                             [&pov](const auto &acc) { return acc.updated(pov); });

    assert(r_it != m_accumulators.rend()); // There must be at least one accumulator up-to date in this branch

    // Update from last updated Accumulator until most recent one
    for (auto forward_it = r_it.base(); forward_it != m_accumulators.end(); ++forward_it) {
        forward_it->update(pov, (forward_it - 1)->pov(pov));
    }
}

ScoreType NNUE::flatten_screlu_and_affine(const PovAccumulator &player, const PovAccumulator &adversary) const {
#ifdef USE_SIMD
    vepi32 sum_vec = vepi32_zero();
    for (int i = 0; i < HIDDEN_LAYER_SIZE; i += REGISTER_SIZE) {
        vepi16 player_weights_vec = vepi16_load(&network.output_weights[i]);
        vepi16 player_vec = vepi16_load(&player.neurons()[i]);

        player_vec = vepi16_clamp(player_vec, QZERO, QONE);
        vepi32 player_product = vepi16_madd(vepi16_mult(player_vec, player_weights_vec), player_vec);
        sum_vec = vepi32_add(sum_vec, player_product);

        vepi16 adversary_weights_vec = vepi16_load(&network.output_weights[i + HIDDEN_LAYER_SIZE]);
        vepi16 adversary_vec = vepi16_load(&adversary.neurons()[i]);

        adversary_vec = vepi16_clamp(adversary_vec, QZERO, QONE);
        vepi32 adversary_product = vepi16_madd(vepi16_mult(adversary_vec, adversary_weights_vec), adversary_vec);
        sum_vec = vepi32_add(sum_vec, adversary_product);
    }

    int32_t sum = vepi32_reduce_add(sum_vec);

#else

    int32_t sum = 0;
    for (int neuron_index = 0; neuron_index < HIDDEN_LAYER_SIZE; ++neuron_index) {
        sum += screlu(player.neurons()[neuron_index]) * network.output_weights[neuron_index];
        sum += screlu(adversary.neurons()[neuron_index]) * network.output_weights[neuron_index + HIDDEN_LAYER_SIZE];
    }

#endif

    sum = (sum / QA + network.output_bias) * SCALE / QAB;
    return std::clamp(sum, -MATE_FOUND + 1, MATE_FOUND - 1);
}
