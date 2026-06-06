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
#include "nnue/arch.h"
#include "nnue/pov_accumulator.h"

[[maybe_unused]] static inline constexpr int32_t crelu(const int32_t &input) {
    return std::clamp(input, CRELU_MIN, CRELU_MAX);
}

[[maybe_unused]] static inline constexpr int32_t screlu(const int32_t &input) {
    const int32_t crelu_out = crelu(input);
    return crelu_out * crelu_out;
}

void NNUE::refresh(const Position &pos) {
    const auto &white_pov_acc = m_finny_table.update(pos, WHITE);
    const auto &black_pov_acc = m_finny_table.update(pos, BLACK);

    m_accumulators.clear();
    m_accumulators.emplace_back(pos.get_king_placement(WHITE), pos.get_king_placement(BLACK), white_pov_acc,
                                black_pov_acc);

    assert(m_accumulators.back().updated(WHITE));
    assert(m_accumulators.back().updated(BLACK));
    assert(m_accumulators.back().pov(WHITE) == PovAccumulator(pos, WHITE));
    assert(m_accumulators.back().pov(BLACK) == PovAccumulator(pos, BLACK));
}

void NNUE::pop() { m_accumulators.pop_back(); }

void NNUE::push(const DirtyPiece &dp, const Square white_king_sq, const Square black_king_sq) {
    assert(!m_accumulators.empty()); // NNUE must have been initialized with the 'refresh' method before pushing
    m_accumulators.emplace_back(dp, white_king_sq, black_king_sq);
}

ScoreType NNUE::eval(const Position &pos) {
    update(pos);
    const size_t output_bucket = (pos.get_piece_count() - 2) / (32 / OUTPUT_BUCKET_COUNT);
    return flatten_screlu_and_affine(m_accumulators.back().pov(pos.get_stm()),
                                     m_accumulators.back().pov(pos.get_adversary()), output_bucket);
}

void NNUE::update(const Position &pos) {
    update_pov(pos, WHITE);
    update_pov(pos, BLACK);
}

void NNUE::update_pov(const Position &pos, const Color &pov) {
    auto head = m_accumulators.rbegin();

    if (head->updated(pov))
        return;

    for (auto iter = m_accumulators.rbegin() + 1; iter != m_accumulators.rend(); ++iter) {
        if (iter->needs_refresh(pov, pos.get_king_placement(pov))) {
            const PovAccumulator &acc = m_finny_table.update(pos, pov);
            head->refresh(pov, acc);
            break;
        } else if (iter->updated(pov)) {
            while (iter != head) {
                (iter - 1)->update(pov, iter->pov(pov));
                --iter;
            }
            break;
        }
    }
    assert(head->updated(pov));
    assert(head->pov(pov) == PovAccumulator(pos, pov));
}

ScoreType NNUE::flatten_screlu_and_affine(const PovAccumulator &player, const PovAccumulator &adversary,
                                          const size_t output_bucket) const {
    const size_t stm_offset = output_bucket * 2 * HIDDEN_LAYER_SIZE;
    const size_t ntm_offset = stm_offset + HIDDEN_LAYER_SIZE;
#ifdef USE_SIMD
    vepi32 sum_vec = vepi32_zero();
    for (int i = 0; i < HIDDEN_LAYER_SIZE; i += REGISTER_SIZE) {
        vepi16 player_weights_vec = vepi16_load(&network.output_weights[i + stm_offset]);
        vepi16 player_vec = vepi16_load(&player.neurons()[i]);

        player_vec = vepi16_clamp(player_vec, QZERO, QONE);
        vepi32 player_product = vepi16_madd(vepi16_mult(player_vec, player_weights_vec), player_vec);
        sum_vec = vepi32_add(sum_vec, player_product);

        vepi16 adversary_weights_vec = vepi16_load(&network.output_weights[i + ntm_offset]);
        vepi16 adversary_vec = vepi16_load(&adversary.neurons()[i]);

        adversary_vec = vepi16_clamp(adversary_vec, QZERO, QONE);
        vepi32 adversary_product = vepi16_madd(vepi16_mult(adversary_vec, adversary_weights_vec), adversary_vec);
        sum_vec = vepi32_add(sum_vec, adversary_product);
    }

    int32_t sum = vepi32_reduce_add(sum_vec);

#else

    int32_t sum = 0;
    for (int neuron_index = 0; neuron_index < HIDDEN_LAYER_SIZE; ++neuron_index) {
        sum += screlu(player.neurons()[neuron_index]) * network.output_weights[neuron_index + stm_offset];
        sum += screlu(adversary.neurons()[neuron_index]) * network.output_weights[neuron_index + ntm_offset];
    }

#endif

    sum = (sum / QA + network.output_bias[output_bucket]) * SCALE / QAB;
    return std::clamp(sum, -MATE_FOUND + 1, MATE_FOUND - 1);
}
