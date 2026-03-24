/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "accumulator.h"

#include <array>
#include <cstdint>
#include <cstring>

#include "../position.h"
#include "../utils.h"
#include "arch.h"
#include "simd.h"

size_t feature_idx(const Piece piece, const Square sq, const Color stm, const size_t king_bucket, const bool flip) {
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

void PovAccumulator::self_add(const size_t feature_idx) { add(neurons.data(), feature_idx); }

void PovAccumulator::self_sub(const size_t feature_idx) { sub(neurons.data(), feature_idx); }

void PovAccumulator::self_add_sub(const size_t add0, const size_t sub0) { add_sub(neurons.data(), add0, sub0); }

void PovAccumulator::add(const int16_t *input, const size_t feature_idx) {
#ifdef USE_SIMD
    for (int i = 0; i < HIDDEN_LAYER_SIZE; i += REGISTER_SIZE) {
        vepi16 weights_vec = vepi16_load(&network.hidden_weights[feature_idx + i]);
        vepi16 input_vec = vepi16_load(&input[i]);
        vepi16_store(&neurons[i], vepi16_add(input_vec, weights_vec));
    }
#else
    for (int i = 0; i < HIDDEN_LAYER_SIZE; ++i) {
        neurons[i] = input[i] + network.hidden_weights[feature_idx + i];
    }
#endif
}

void PovAccumulator::sub(const int16_t *input, const size_t feature_idx) {
#ifdef USE_SIMD
    for (int i = 0; i < HIDDEN_LAYER_SIZE; i += REGISTER_SIZE) {
        vepi16 weights_vec = vepi16_load(&network.hidden_weights[feature_idx + i]);
        vepi16 input_vec = vepi16_load(&input[i]);
        vepi16_store(&neurons[i], vepi16_sub(input_vec, weights_vec));
    }
#else
    for (int i = 0; i < HIDDEN_LAYER_SIZE; ++i) {
        neurons[i] = input[i] + network.hidden_weights[feature_idx + i];
    }
#endif
}

// TODO implement fused updates
void PovAccumulator::add_sub(const int16_t *input, const size_t add0, const size_t sub0) {
    add(input, add0);
    self_sub(sub0);
}

// TODO implement fused updates
void PovAccumulator::add_sub2(const int16_t *input, const size_t add0, const size_t sub0, const size_t sub1) {
    add(input, add0);
    self_sub(sub0);
    self_sub(sub1);
}

// TODO implement fused updates
void PovAccumulator::add2_sub2(const int16_t *input, const size_t add0, const size_t add1, const size_t sub0,
                               const size_t sub1) {
    add(input, add0);
    self_add(add1);
    self_sub(sub0);
    self_sub(sub1);
}

Accumulator::Accumulator(const DirtyPiece dp, const Square white_king_sq, const Square black_king_sq) {
    m_updated[WHITE] = m_updated[BLACK] = false;
    m_king_sqs[WHITE] = white_king_sq;
    m_king_sqs[BLACK] = black_king_sq;
    m_dirty_piece = dp;
}

void Accumulator::update(const Color side, const Position &pos, const Accumulator &prev_acc) {
    const DirtyPiece &dp = m_dirty_piece;

    const Square king_sq = pos.get_king_placement(side);
    const Square king_pov_sq = static_cast<Square>(side == WHITE ? king_sq : king_sq ^ 56);
    const size_t king_bucket = KING_BUCKETS_LAYOUT[king_pov_sq];
    const bool flip = should_flip(king_pov_sq);

    switch (dp.move_type) {
        case REGULAR:
            m_pov_accumulators[side].add_sub(prev_acc.m_pov_accumulators[side].neurons.data(),
                                             feature_idx(dp.add0.piece, dp.add0.sq, side, king_bucket, flip),
                                             feature_idx(dp.sub0.piece, dp.sub0.sq, side, king_bucket, flip));
            break;
        case CAPTURE:
            m_pov_accumulators[side].add_sub2(prev_acc.m_pov_accumulators[side].neurons.data(),
                                              feature_idx(dp.add0.piece, dp.add0.sq, side, king_bucket, flip),
                                              feature_idx(dp.sub0.piece, dp.sub0.sq, side, king_bucket, flip),
                                              feature_idx(dp.sub1.piece, dp.sub1.sq, side, king_bucket, flip));
            break;
        case CASTLING:
            m_pov_accumulators[side].add2_sub2(prev_acc.m_pov_accumulators[side].neurons.data(),
                                               feature_idx(dp.add0.piece, dp.add0.sq, side, king_bucket, flip),
                                               feature_idx(dp.add1.piece, dp.add1.sq, side, king_bucket, flip),
                                               feature_idx(dp.sub0.piece, dp.sub0.sq, side, king_bucket, flip),
                                               feature_idx(dp.sub1.piece, dp.sub1.sq, side, king_bucket, flip));
            break;
        default:
            __builtin_unreachable();
    }

    m_updated[side] = true;
}

void Accumulator::set_dirty_piece(const DirtyPiece dp) {
    m_updated[WHITE] = m_updated[BLACK] = false;
    m_dirty_piece = dp;
}

void Accumulator::reset() {
    std::memcpy(m_pov_accumulators[0].neurons.data(), network.hidden_bias.data(), sizeof(network.hidden_bias));
    std::memcpy(m_pov_accumulators[1].neurons.data(), network.hidden_bias.data(), sizeof(network.hidden_bias));
}

void Accumulator::set_king_sqs(const Square white_king_sq, const Square black_king_sq) {
    m_king_sqs[WHITE] = white_king_sq;
    m_king_sqs[BLACK] = black_king_sq;
}

void Accumulator::refresh(const Color side, const PovAccumulator &finny_table_neurons) {
    std::memcpy(m_pov_accumulators[side].neurons.data(), finny_table_neurons.neurons.data(),
                sizeof(finny_table_neurons.neurons));
    m_updated[side] = true;
}

bool Accumulator::needs_refresh(const Color side, const Square new_king_sq) {
    return (new_king_sq & 0b100) != (m_king_sqs[side] & 0b100) ||                     // King crossed half of the board
           KING_BUCKETS_LAYOUT[new_king_sq] != KING_BUCKETS_LAYOUT[m_king_sqs[side]]; // King bucket change
}
