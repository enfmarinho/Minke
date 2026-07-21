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
#include "nnue/accumulator.h"
#include "nnue/arch.h"
#include "nnue/pov_accumulator.h"
#include "nnue/simd.h"

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
    update(pos); // ensure accumulator is up-to date

    const Accumulator &acc = m_accumulators.back();
    const int bucket = (pos.get_material_count() - 2) / BUCKET_SIZE;

    return propagate(acc.pov(pos.get_stm()).neurons(), acc.pov(pos.get_adversary()).neurons(), bucket);
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

int32_t NNUE::propagate(std::span<const int16_t, L1_SIZE> stm_inputs, std::span<const int16_t, L1_SIZE> ntm_inputs,
                        const int bucket) {
    alignas(64) uint8_t ft_outputs[L1_SIZE];
    alignas(64) int32_t l1_outputs[L2_SIZE];
    alignas(64) int32_t l2_outputs[L3_SIZE];
    int32_t l3_output;

    activate_ft(stm_inputs, ntm_inputs, ft_outputs);
    propagate_l1(bucket, ft_outputs, l1_outputs);
    propagate_l2(bucket, l1_outputs, l2_outputs);
    propagate_l3(bucket, l2_outputs, l3_output);

    return l3_output;
}

void NNUE::activate_ft(std::span<const int16_t, L1_SIZE> stm_acc, std::span<const int16_t, L1_SIZE> ntm_acc,
                       std::span<uint8_t, L1_SIZE> outputs) {
    constexpr size_t PAIR_COUNT = L1_SIZE / 2;
    const auto pov_activate = [&](std::span<const int16_t, L1_SIZE> acc, int output_offset) {
#if USE_SIMD
        using namespace simd;

        vepi16 zero = zero_i16();
        vepi16 one = set_i16(QA);

        for (size_t idx = 0; idx < PAIR_COUNT; idx += 4 * CHUNK_SIZE_16BIT) {
            vepi16 i0_left = clamp_i16(load_i16(&acc[idx + CHUNK_SIZE_16BIT * 0]), zero, one);
            vepi16 i1_left = clamp_i16(load_i16(&acc[idx + CHUNK_SIZE_16BIT * 1]), zero, one);
            vepi16 i2_left = clamp_i16(load_i16(&acc[idx + CHUNK_SIZE_16BIT * 2]), zero, one);
            vepi16 i3_left = clamp_i16(load_i16(&acc[idx + CHUNK_SIZE_16BIT * 3]), zero, one);

            vepi16 i0_right = clamp_i16(load_i16(&acc[idx + PAIR_COUNT + CHUNK_SIZE_16BIT * 0]), zero, one);
            vepi16 i1_right = clamp_i16(load_i16(&acc[idx + PAIR_COUNT + CHUNK_SIZE_16BIT * 1]), zero, one);
            vepi16 i2_right = clamp_i16(load_i16(&acc[idx + PAIR_COUNT + CHUNK_SIZE_16BIT * 2]), zero, one);
            vepi16 i3_right = clamp_i16(load_i16(&acc[idx + PAIR_COUNT + CHUNK_SIZE_16BIT * 3]), zero, one);

            vepi16 pw0 = mulhi_i16(shiftleft_i16(i0_left, FT_SCALE_BITS), i0_right);
            vepi16 pw1 = mulhi_i16(shiftleft_i16(i1_left, FT_SCALE_BITS), i1_right);
            vepi16 pw2 = mulhi_i16(shiftleft_i16(i2_left, FT_SCALE_BITS), i2_right);
            vepi16 pw3 = mulhi_i16(shiftleft_i16(i3_left, FT_SCALE_BITS), i3_right);

            vepu8 packed0 = packus_i16(pw0, pw1);
            vepu8 packed1 = packus_i16(pw2, pw3);

            store_u8(&outputs[output_offset + idx + CHUNK_SIZE_8BIT * 0], packed0);
            store_u8(&outputs[output_offset + idx + CHUNK_SIZE_8BIT * 1], packed1);
        }

#else
        for (int i = 0; i < PAIR_COUNT; ++i) {
            int32_t i0_left = std::clamp<int16_t>(acc[i], 0, QA);
            int32_t i0_right = std::clamp<int16_t>(acc[i + PAIR_COUNT], 0, QA);

            // simulate mulhi
            outputs[i + output_offset] = ((i0_left << FT_SCALE_BITS) * i0_right) >> 16;
        }
#endif
    };

    pov_activate(stm_acc, 0);
    pov_activate(ntm_acc, PAIR_COUNT);
}

void NNUE::propagate_l1(int bucket, std::span<const uint8_t, L1_SIZE> inputs, std::span<int32_t, L2_SIZE> outputs) {
    constexpr int shift = 8;

#if USE_SIMD
    using namespace simd;

    const int32_t *packed_input = reinterpret_cast<const int32_t *>(inputs.data());

    // Number of registers needed for L2 propagation
    constexpr size_t NUM_REGISTERS = L2_SIZE / CHUNK_SIZE_32BIT;
    vepi32 l2_regs[NUM_REGISTERS];

    // Init registers with L2 biases
    for (size_t i = 0; i < NUM_REGISTERS; ++i) {
        l2_regs[i] = load_i32(&network.l1_biases[bucket][i * CHUNK_SIZE_32BIT]);
    }

    // Accumulate dot products
    for (size_t chunk_idx = 0; chunk_idx < L1_SIZE / 4; ++chunk_idx) {
        vepi32 input = set_i32(packed_input[chunk_idx]);

        for (size_t i = 0; i < NUM_REGISTERS; ++i) {
            vepi8 weights = load_i8(&network.l1_weights[bucket][chunk_idx][i * CHUNK_SIZE_32BIT][0]);
            l2_regs[i] = dpbusd_i32(l2_regs[i], input, weights);
        }
    }

    // Apply shift, SCReLU activation and store
    const vepi32 zero = zero_i32();
    const vepi32 one = set_i32(QC);

    for (size_t i = 0; i < NUM_REGISTERS; ++i) {
        vepi32 reg = shiftright_i32(l2_regs[i], shift);

        reg = clamp_i32(reg, zero, one);
        reg = mullo_i32(reg, reg);

        store_i32(&outputs[i * CHUNK_SIZE_32BIT], reg);
    }
#else
    // Initialize accumulators with biases
    for (size_t output_idx = 0; output_idx < L2_SIZE; ++output_idx) {
        outputs[output_idx] = network.l1_biases[bucket][output_idx];
    }

    // Accumulate dot products, simulating dpbusd_i32
    for (size_t chunk = 0; chunk < L1_SIZE / 4; ++chunk) {
        const int32_t in0 = inputs[chunk * 4 + 0];
        const int32_t in1 = inputs[chunk * 4 + 1];
        const int32_t in2 = inputs[chunk * 4 + 2];
        const int32_t in3 = inputs[chunk * 4 + 3];

        for (size_t output_idx = 0; output_idx < L2_SIZE; ++output_idx) {
            const int32_t w0 = network.l1_weights[bucket][chunk][output_idx][0];
            const int32_t w1 = network.l1_weights[bucket][chunk][output_idx][1];
            const int32_t w2 = network.l1_weights[bucket][chunk][output_idx][2];
            const int32_t w3 = network.l1_weights[bucket][chunk][output_idx][3];

            outputs[output_idx] += (in0 * w0) + (in1 * w1) + (in2 * w2) + (in3 * w3);
        }
    }

    // Apply shift, SCReLU activation and store
    for (size_t output_idx = 0; output_idx < L2_SIZE; ++output_idx) {
        int32_t v = outputs[output_idx] >> shift;
        v = std::clamp(v, 0, QC);
        outputs[output_idx] = v * v;
    }
#endif
}

/// Does not activate outputs, that's done on 'propagate_l3'
void NNUE::propagate_l2(int bucket, std::span<const int32_t, L2_SIZE> inputs, std::span<int32_t, L3_SIZE> outputs) {
    // L2 biases
    std::memcpy(outputs.data(), &network.l2_biases[bucket], outputs.size_bytes());

    // L2 matmul
#if USE_SIMD
    using namespace simd;

    for (size_t input_idx = 0; input_idx < L2_SIZE; ++input_idx) {
        vepi32 input = set_i32(inputs[input_idx]);

        for (size_t output_idx = 0; output_idx < L3_SIZE; output_idx += CHUNK_SIZE_32BIT) {
            vepi32 weights = load_i32(&network.l2_weights[bucket][input_idx][output_idx]);
            vepi32 output = load_i32(&outputs[output_idx]);

            vepi32 product = mullo_i32(input, weights);
            output = add_i32(output, product);

            store_i32(&outputs[output_idx], output);
        }
    }
#else
    for (size_t input_idx = 0; input_idx < L2_SIZE; ++input_idx) {
        const int32_t input = inputs[input_idx];
        const int32_t *weights = &network.l2_weights[bucket][input_idx][0];
        for (size_t output_idx = 0; output_idx < L3_SIZE; ++output_idx) {
            outputs[output_idx] += weights[output_idx] * input;
        }
    }
#endif
}

void NNUE::propagate_l3(int bucket, std::span<const int32_t, L3_SIZE> inputs, int32_t &output) {
    // Activate L2 outputs ('inputs') and L3 matmul
#if USE_SIMD
    using namespace simd;

    const vepi32 zero = zero_i32();
    const vepi32 one = set_i32(QC * QC * QC);

    vepi32 output_vec = zero;
    for (size_t idx = 0; idx < L3_SIZE; idx += CHUNK_SIZE_32BIT) {
        vepi32 input = load_i32(&inputs[idx]);
        vepi32 weights = load_i32(&network.l3_weights[bucket][idx]);

        input = clamp_i32(input, zero, one); // Activate L2 outputs
        output_vec = add_i32(output_vec, mullo_i32(input, weights));
    }

    output = hsum_i32(output_vec);
#else
    output = 0;
    for (size_t idx = 0; idx < L3_SIZE; ++idx) {
        const int32_t l2_out = std::clamp(inputs[idx], 0, QC * QC * QC); // Activate L2 output
        output += l2_out * network.l3_weights[bucket][idx];
    }
#endif

    // Add L3 bias
    output += network.l3_biases[bucket];

    int64_t rescaled_out = static_cast<int64_t>(output);
    rescaled_out *= SCALE;
    rescaled_out /= QC * QC * QC * QC;

    output = static_cast<int32_t>(rescaled_out);
}
