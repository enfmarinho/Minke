/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "finny_table.h"

#include <cstdint>

#include "../position.h"
#include "arch.h"
#include "simd.h"

[[maybe_unused]] static int32_t screlu(const int32_t &input) {
    const int32_t crelu_out = std::clamp(input, CRELU_MIN, CRELU_MAX);
    return crelu_out * crelu_out;
}

static size_t feature_idx(const Piece piece, const Square sq, const Color stm, const size_t king_bucket,
                          const bool flip) {
    constexpr size_t COLOR_STRIDE = 64 * 6;
    constexpr size_t PIECE_STRIDE = 64;

    const Color piece_color = get_color(piece);
    int pov_sq = sq;
    if (stm == BLACK) // Convert square to pov, i.e. flip rank if stm is black
        pov_sq = pov_sq ^ 56;
    if (flip) // Flip file
        pov_sq = pov_sq ^ 7;

    const size_t idx = king_bucket * INPUT_LAYER_SIZE +                    // Input bucket offset
                       (piece_color != stm) * COLOR_STRIDE +               // Perspective offset
                       get_piece_type(piece, piece_color) * PIECE_STRIDE + // Piece type offset
                       pov_sq;
    return idx * HIDDEN_LAYER_SIZE;
}

void FinnyTable::reset() {
    // Reset all cached accumulators
    for (auto &stm_buckets : cache)
        for (auto &flip_buckets : stm_buckets)
            for (auto &king_buckets : flip_buckets)
                king_buckets.reset();
}

int32_t FinnyTable::accumulate_activate_affine(const Position &pos, const Color side, const int16_t *output_weights) {
    const Square king_sq = pos.get_king_placement(side);
    const Square king_pov_sq = static_cast<Square>(side == WHITE ? king_sq : king_sq ^ 56);
    const size_t king_bucket = KING_BUCKETS_LAYOUT[king_pov_sq];
    const bool flip = get_file(king_pov_sq) > 3;

    FinnyTableCache &cached_entry = get_cache(side, flip, king_bucket);

    size_t add[32], sub[32];
    size_t add_count = 0, sub_count = 0;
    for (size_t piece_idx = WHITE_PAWN; piece_idx <= BLACK_KING; ++piece_idx) {
        const Piece piece = static_cast<Piece>(piece_idx);
        Bitboard added = pos.get_piece_bb(piece) & ~cached_entry.occupancies[piece_idx];
        Bitboard removed = cached_entry.occupancies[piece_idx] & ~pos.get_piece_bb(piece);
        while (added) {
            const Square sq = poplsb(added);
            add[add_count++] = feature_idx(piece, sq, side, king_bucket, flip);
        }

        while (removed) {
            const Square sq = poplsb(removed);
            sub[sub_count++] = feature_idx(piece, sq, side, king_bucket, flip);
        }

        cached_entry.occupancies[piece_idx] = pos.get_piece_bb(piece);
    }

#ifdef USE_SIMD
    constexpr int NUM_REGI = 8;
    constexpr int TILE_SIZE = NUM_REGI * sizeof(vepi16) / sizeof(int16_t);
    vepi16 regs[NUM_REGI];
    vepi32 sum_vec = vepi16_zero();
    for (int i = 0; i < HIDDEN_LAYER_SIZE; i += TILE_SIZE) {
        vepi16 *entryVec = reinterpret_cast<vepi16 *>(&cached_entry.accumulator.neurons[i]);
        for (int j = 0; j < NUM_REGI; ++j) {
            regs[j] = vepi16_load(&entryVec[j]);
        }

        for (size_t j = 0; j < add_count; ++j) {
            const auto *addedVec = reinterpret_cast<const vepi16 *>(&network.hidden_weights[add[j] + i]);
            for (int k = 0; k < NUM_REGI; ++k) {
                regs[k] = vepi16_add(regs[k], addedVec[k]);
            }
        }

        for (size_t j = 0; j < sub_count; ++j) {
            const auto *removedVec = reinterpret_cast<const vepi16 *>(&network.hidden_weights[sub[j] + i]);
            for (int k = 0; k < NUM_REGI; ++k) {
                regs[k] = vepi16_sub(regs[k], removedVec[k]);
            }
        }

        for (int j = 0; j < NUM_REGI; ++j) {
            vepi16_store(&entryVec[j], regs[j]);
        }

        const vepi16 *weights_vec = reinterpret_cast<const vepi16 *>(&output_weights[i]);
        for (int j = 0; j < NUM_REGI; ++j) {
            regs[j] = vepi16_clamp(regs[j], QZERO, QONE);
            const vepi16 weight_vec = vepi16_load(&weights_vec[j]);
            vepi32 product = vepi16_madd(vepi16_mult(regs[j], weight_vec), regs[j]);
            sum_vec = vepi32_add(sum_vec, product);
        }
    }

    int32_t sum = vepi32_reduce_add(sum_vec);

#else
    for (size_t i = 0; i < add_count; ++i) {
        for (size_t column = 0; column < HIDDEN_LAYER_SIZE; ++column) {
            cached_entry.accumulator.neurons[column] += network.hidden_weights[add[i] + column];
        }
    }

    for (size_t i = 0; i < sub_count; ++i) {
        for (size_t column = 0; column < HIDDEN_LAYER_SIZE; ++column) {
            cached_entry.accumulator.neurons[column] -= network.hidden_weights[sub[i] + column];
        }
    }

    int32_t sum = 0;
    for (int neuron_index = 0; neuron_index < HIDDEN_LAYER_SIZE; ++neuron_index) {
        sum += screlu(cached_entry.accumulator.neurons[neuron_index]) * output_weights[neuron_index];
    }
#endif // USE_SIMD

    return sum;
}

void FinnyTable::FinnyTableCache::reset() {
    for (Bitboard &occ : occupancies) {
        occ = 0;
    }
    accumulator.reset(network.hidden_bias);
}

FinnyTable::FinnyTableCache &FinnyTable::get_cache(const Color side, const bool flip, const size_t king_bucket) {
    return cache[side][flip][king_bucket];
}
