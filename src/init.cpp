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

#include "init.h"

#include <cmath>
#include <cstdint>

#include "attacks.h"
#include "eval/nnue/arch.h"
#include "hash.h"
#include "incbin.h"
#include "search.h"
#include "tune.h"
#include "types.h"

#ifndef EVALFILE
#define EVALFILE "../src/minke.bin"
#endif // !EVALFILE

INCBIN(NetParameters, EVALFILE);

int LMR_TABLE[64][64];
int LMP_TABLE[2][LMP_DEPTH];
HashKeys hash_keys;
Network network;
Bitboard between_squares[64][64];
Bitboard passing_rays[64][64];

void init_all() {
    init_search_params();
    init_network_params();
    init_hash_keys();
    init_magic_attack_tables();
    init_between_squares();
    init_passing_rays();
}

void init_search_params() {
    for (int depth = 1; depth < 64; ++depth) {
        for (int move_counter = 1; move_counter < 64; ++move_counter) {
            LMR_TABLE[depth][move_counter] = lmr_base() + lmr_scale() * std::log(depth) * std::log(move_counter);
        }
    }
    LMR_TABLE[0][0] = 0;

    for (int depth = 0; depth < LMP_DEPTH; ++depth) {
        LMP_TABLE[0][depth] = (lmp_base() / 100.0) + (lmp_scale() / 100.0) * depth * depth;
        LMP_TABLE[1][depth] = (lmp_improving_base() / 100.0) + (lmp_improving_scale() / 100.0) * depth * depth;
    }
}

void init_network_params() {
    const uint8_t* raw_bytes = reinterpret_cast<const uint8_t*>(gNetParametersData);

    // Copy FT weights
    size_t offset = 0;
    std::memcpy(network.ft_weights, raw_bytes + offset, sizeof(network.ft_weights));
    offset += sizeof(network.ft_weights);

    // Copy FT biases
    std::memcpy(network.ft_biases, raw_bytes + offset, sizeof(network.ft_biases));
    offset += sizeof(network.ft_biases);

    // Transform raw l1_weights, bullet output (transposed: (output_buckets * l2_size) x l1_size) into VNNI layout
    const int8_t* raw_l1 = reinterpret_cast<const int8_t*>(raw_bytes + offset);
    for (int l1_idx = 0; l1_idx < L1_SIZE; ++l1_idx) {
        for (int out_bucket_idx = 0; out_bucket_idx < OUTPUT_BUCKET_COUNT; ++out_bucket_idx) {
            for (int l2_idx = 0; l2_idx < L2_SIZE; ++l2_idx) {
                network.l1_weights[out_bucket_idx][l1_idx / 4][l2_idx][l1_idx % 4] =
                    raw_l1[(out_bucket_idx * L2_SIZE + l2_idx) * L1_SIZE + l1_idx];
            }
        }
    }
    offset += sizeof(network.l1_weights);

    // Copy L1 biases
    const int32_t* raw_l1b = reinterpret_cast<const int32_t*>(raw_bytes + offset);
    for (int out_bucket_idx = 0; out_bucket_idx < OUTPUT_BUCKET_COUNT; ++out_bucket_idx) {
        for (int l2_idx = 0; l2_idx < L2_SIZE; ++l2_idx) {
            network.l1_biases[out_bucket_idx][l2_idx] = raw_l1b[out_bucket_idx * L2_SIZE + l2_idx];
        }
    }
    offset += sizeof(network.l1_biases);

    // Transform raw l2_weights, bullet output (transposed: (output_buckets * l3_size) x l2_size)
    const int32_t* raw_l2 = reinterpret_cast<const int32_t*>(raw_bytes + offset);
    for (int l2_idx = 0; l2_idx < L2_SIZE; ++l2_idx) {
        for (int out_bucket_idx = 0; out_bucket_idx < OUTPUT_BUCKET_COUNT; ++out_bucket_idx) {
            for (int l3_idx = 0; l3_idx < L3_SIZE; ++l3_idx) {
                network.l2_weights[out_bucket_idx][l2_idx][l3_idx] =
                    raw_l2[(out_bucket_idx * L3_SIZE + l3_idx) * L2_SIZE + l2_idx];
            }
        }
    }
    offset += sizeof(network.l2_weights);

    // L2 biases
    std::memcpy(network.l2_biases, raw_bytes + offset, sizeof(network.l2_biases));
    offset += sizeof(network.l2_biases);

    // Transform raw l3_weights, bullet output (transposed: output_buckets x l3_size)
    const int32_t* raw_l3 = reinterpret_cast<const int32_t*>(raw_bytes + offset);
    for (int l3_idx = 0; l3_idx < L3_SIZE; ++l3_idx) {
        for (int out_bucket_idx = 0; out_bucket_idx < OUTPUT_BUCKET_COUNT; ++out_bucket_idx) {
            network.l3_weights[out_bucket_idx][l3_idx] = raw_l3[out_bucket_idx * L3_SIZE + l3_idx];
        }
    }
    offset += sizeof(network.l3_weights);

    // L3 biases
    std::memcpy(network.l3_biases, raw_bytes + offset, sizeof(network.l3_biases));
}

void init_hash_keys() {
    PRNG prng(1070372);
    for (int piece = WHITE_PAWN; piece <= BLACK_KING; ++piece) {
        for (int sqi = a1; sqi <= h8; ++sqi) {
            hash_keys.pieces[piece][sqi] = prng.rand<HashType>();
        }
    }

    for (int castle = 0; castle < 16; ++castle) {
        hash_keys.castle[castle] = prng.rand<HashType>();
    }

    for (int rank = 0; rank < 8; ++rank) {
        hash_keys.en_passant[rank] = prng.rand<HashType>();
    }

    hash_keys.side = prng.rand<HashType>();
}

void init_magic_attack_tables() {
    // This initializes all attacks, masks, magics and shifts for Bishop and Rook as a side effect
    init_magic_table(BISHOP);
    init_magic_table(ROOK);

    // Initialize non-slider attack tables
    for (int sqi = a1; sqi <= h8; ++sqi) {
        Square sq = static_cast<Square>(sqi);

        pawn_attacks[WHITE][sq] = generate_pawn_attacks(sq, WHITE);
        pawn_attacks[BLACK][sq] = generate_pawn_attacks(sq, BLACK);
        knight_attacks[sq] = generate_knight_attacks(sq);
        king_attacks[sq] = generate_king_attacks(sq);
    }
}

void init_between_squares() {
    for (int sqi1 = a1; sqi1 <= h8; ++sqi1) {
        for (int sqi2 = a1; sqi2 <= h8; ++sqi2) {
            Square sq1 = static_cast<Square>(sqi1);
            Square sq2 = static_cast<Square>(sqi2);
            Bitboard occ1 = (1ULL << sq1);
            Bitboard occ2 = (1ULL << sq2);
            if (get_bishop_attacks(sq1, 0) & occ2) {
                between_squares[sq1][sq2] = get_bishop_attacks(sq1, occ2) & get_bishop_attacks(sq2, occ1);
            } else if (get_rook_attacks(sq1, 0) & occ2) {
                between_squares[sq1][sq2] = get_rook_attacks(sq1, occ2) & get_rook_attacks(sq2, occ1);
            }
        }
    }
}

void init_passing_rays() {
    for (int src = a1; src <= h8; ++src) {
        Square src_sq = static_cast<Square>(src);
        Bitboard src_mask = (1ull << src_sq);

        Bitboard rook_attack = get_rook_attacks(src_sq, 0);
        Bitboard bishop_attack = get_bishop_attacks(src_sq, 0);
        for (int to = a1; to <= h8; ++to) {
            if (src == to)
                continue;

            Square to_sq = static_cast<Square>(to);
            Bitboard to_mask = (1ull << to_sq);

            if (rook_attack & to_mask) {
                passing_rays[src][to] = rook_attack & (get_rook_attacks(to_sq, src_mask) | to_mask);
            } else if (bishop_attack & to_mask) {
                passing_rays[src][to] = bishop_attack & (get_bishop_attacks(to_sq, src_mask) | to_mask);
            }
        }
    }
}
