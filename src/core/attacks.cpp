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

#include "attacks.h"

#include <bit>
#include <cassert>
#include <cstdint>

#include "core/bitboard.h"
#include "core/types.h"
#include "utils/random.h"
#include "utils/utils.h"

namespace Attacks {

Bitboard bishop_masks[64];
Bitboard rook_masks[64];

int bishop_shifts[64];
int rook_shifts[64];

uint64_t bishop_magic_numbers[64];
uint64_t rook_magic_numbers[64];

Bitboard pawn_attacks[2][64];
Bitboard knight_attacks[64];
Bitboard king_attacks[64];
Bitboard bishop_attacks[64][512];
Bitboard rook_attacks[64][4096];

Bitboard inbetween_masks[64][64];
Bitboard passing_masks[64][64];

Bitboard diagonal_masks[64];
Bitboard antidiagonal_masks[64];

namespace {

void ray_mask(Bitboard& mask, Square sq, auto shift_fn, Bitboard edge_mask) {
    for (Bitboard b = shift_fn(Bitboard(sq)); b & ~edge_mask; b = shift_fn(b)) {
        mask |= b;
    }
}

Bitboard generate_bishop_mask(Square sq) {
    Bitboard mask;

    ray_mask(mask, sq, [](Bitboard b) { return b.shift_north_east(); }, Bitboard::RANK_8 | Bitboard::FILE_H);
    ray_mask(mask, sq, [](Bitboard b) { return b.shift_north_west(); }, Bitboard::RANK_8 | Bitboard::FILE_A);
    ray_mask(mask, sq, [](Bitboard b) { return b.shift_south_east(); }, Bitboard::RANK_1 | Bitboard::FILE_H);
    ray_mask(mask, sq, [](Bitboard b) { return b.shift_south_west(); }, Bitboard::RANK_1 | Bitboard::FILE_A);

    return mask;
}

Bitboard generate_rook_mask(Square sq) {
    Bitboard mask;

    ray_mask(mask, sq, [](Bitboard b) { return b.shift_north(); }, Bitboard::RANK_8);
    ray_mask(mask, sq, [](Bitboard b) { return b.shift_south(); }, Bitboard::RANK_1);
    ray_mask(mask, sq, [](Bitboard b) { return b.shift_west(); }, Bitboard::FILE_A);
    ray_mask(mask, sq, [](Bitboard b) { return b.shift_east(); }, Bitboard::FILE_H);

    return mask;
}

Bitboard generate_pawn_attacks(Square sq, Color color) {
    Bitboard attacks;
    Bitboard board(sq);

    if (color == WHITE) {
        attacks |= board.shift_north_west();
        attacks |= board.shift_north_east();
    } else {
        assert(color == BLACK);
        attacks |= board.shift_south_west();
        attacks |= board.shift_south_east();
    }

    return attacks;
}

Bitboard generate_knight_attacks(Square sq) {
    Bitboard attacks;
    Bitboard board(sq);

    attacks |= board.shift_double_north_west();
    attacks |= board.shift_double_north_east();
    attacks |= board.shift_double_south_west();
    attacks |= board.shift_double_south_east();
    attacks |= board.shift_double_west_south();
    attacks |= board.shift_double_west_north();
    attacks |= board.shift_double_east_south();
    attacks |= board.shift_double_east_north();

    return attacks;
}

void ray_attack_mask(Bitboard& attack_mask, Square sq, const Bitboard& blockers, auto shift_fn) {
    for (Bitboard b = shift_fn(Bitboard(sq)); b; b = shift_fn(b)) {
        attack_mask |= b;
        if (b & blockers)
            break;
    }
}

Bitboard generate_bishop_attacks(Square sq, const Bitboard& blockers) {
    Bitboard attack_mask;
    Bitboard board(sq);

    ray_attack_mask(attack_mask, sq, blockers, [](Bitboard b) { return b.shift_north_west(); });
    ray_attack_mask(attack_mask, sq, blockers, [](Bitboard b) { return b.shift_north_east(); });
    ray_attack_mask(attack_mask, sq, blockers, [](Bitboard b) { return b.shift_south_west(); });
    ray_attack_mask(attack_mask, sq, blockers, [](Bitboard b) { return b.shift_south_east(); });

    return attack_mask;
}

Bitboard generate_rook_attacks(Square sq, const Bitboard& blockers) {
    Bitboard attack_mask;
    Bitboard board(sq);

    ray_attack_mask(attack_mask, sq, blockers, [](Bitboard b) { return b.shift_north(); });
    ray_attack_mask(attack_mask, sq, blockers, [](Bitboard b) { return b.shift_south(); });
    ray_attack_mask(attack_mask, sq, blockers, [](Bitboard b) { return b.shift_west(); });
    ray_attack_mask(attack_mask, sq, blockers, [](Bitboard b) { return b.shift_east(); });

    return attack_mask;
}

Bitboard generate_king_attacks(Square sq) {
    Bitboard attacks;
    Bitboard board(sq);

    attacks |= board.shift_north();
    attacks |= board.shift_south();
    attacks |= board.shift_west();
    attacks |= board.shift_east();
    attacks |= board.shift_north_west();
    attacks |= board.shift_north_east();
    attacks |= board.shift_south_west();
    attacks |= board.shift_south_east();

    return attacks;
}

void init_inbetween_masks() {
    for (int sqi1 = a1; sqi1 <= h8; ++sqi1) {
        for (int sqi2 = a1; sqi2 <= h8; ++sqi2) {
            Square sq1 = static_cast<Square>(sqi1);
            Square sq2 = static_cast<Square>(sqi2);
            Bitboard occ1(sq1);
            Bitboard occ2(sq2);
            if (get_bishop_attacks(sq1, 0) & occ2) {
                inbetween_masks[sq1][sq2] = get_bishop_attacks(sq1, occ2) & get_bishop_attacks(sq2, occ1);
            } else if (get_rook_attacks(sq1, 0) & occ2) {
                inbetween_masks[sq1][sq2] = get_rook_attacks(sq1, occ2) & get_rook_attacks(sq2, occ1);
            }
        }
    }
}

void init_passing_masks() {
    for (int src = a1; src <= h8; ++src) {
        Square src_sq = static_cast<Square>(src);
        Bitboard src_mask(src_sq);

        Bitboard rook_attack = get_rook_attacks(src_sq, 0);
        Bitboard bishop_attack = get_bishop_attacks(src_sq, 0);
        for (int to = a1; to <= h8; ++to) {
            if (src == to)
                continue;

            Square to_sq = static_cast<Square>(to);
            Bitboard to_mask(to_sq);

            if (rook_attack & to_mask) {
                passing_masks[src][to] = rook_attack & (get_rook_attacks(to_sq, src_mask) | to_mask);
            } else if (bishop_attack & to_mask) {
                passing_masks[src][to] = bishop_attack & (get_bishop_attacks(to_sq, src_mask) | to_mask);
            }
        }
    }
}

void init_diagonal_antidiagonal_masks() {
    for (int sqi = a1; sqi <= h8; ++sqi) {
        Square sq = static_cast<Square>(sqi);

        Bitboard diag(sq); // south_west to north_east
        for (Bitboard mask = diag.shift_north_east(); mask; mask = mask.shift_north_east())
            diag |= mask;
        for (Bitboard mask = diag.shift_south_west(); mask; mask = mask.shift_south_west())
            diag |= mask;
        diagonal_masks[sq] = diag;

        Bitboard antidiag(sq); // north_west to south_east
        for (Bitboard mask = antidiag.shift_north_west(); mask; mask = mask.shift_north_west())
            antidiag |= mask;
        for (Bitboard mask = antidiag.shift_south_east(); mask; mask = mask.shift_south_east())
            antidiag |= mask;
        antidiagonal_masks[sq] = antidiag;
    }
}

//=== Adapted from Stockfish.
// Initialize all attacks, masks, magics and shifts tables for piece_type.
// piece_type must by Bishop or Rook
void init_magic_table(PieceType piece_type) {
    assert(piece_type == BISHOP || piece_type == ROOK);

    // Maybe optimal PRNG seeds to pick the correct magics in the shortest time
    int seeds[8] = {728, 10316, 55013, 32803, 12281, 15100, 16645, 255};

    Bitboard occupancy[4096];
    Bitboard reference[4096];
    int epoch[4096] = {}, cnt = 0;

    for (int sqi = a1; sqi <= h8; ++sqi) {
        Square sq = static_cast<Square>(sqi);

        Bitboard mask, *attacks;
        uint64_t* magic;
        int n_shifts;
        if (piece_type == BISHOP) {
            mask = bishop_masks[sq] = generate_bishop_mask(sq);
            magic = &bishop_magic_numbers[sq];
            attacks = bishop_attacks[sq];
            n_shifts = bishop_shifts[sq] = 64 - mask.popcount();
        } else {
            mask = rook_masks[sq] = generate_rook_mask(sq);
            magic = &rook_magic_numbers[sq];
            attacks = rook_attacks[sq];
            n_shifts = rook_shifts[sq] = 64 - mask.popcount();
        }

        // Use Carry-Rippler trick to enumerate all subsets of mask, store them on
        // occupancy[] and store the corresponding sliding attack bitboard in reference[].
        int size = 0;
        Bitboard blockers;
        do {
            occupancy[size] = blockers;
            reference[size] =
                (piece_type == BISHOP) ? generate_bishop_attacks(sq, blockers) : generate_rook_attacks(sq, blockers);

            size++;
            blockers = (blockers - mask) & mask;
        } while (blockers);

        PRNG prng(seeds[get_rank(sq)]);

        // Find a magic for square picking up an (almost) random number
        // until we find the one that passes the verification test.
        for (int i = 0; i < size;) {
            for (*magic = 0; std::popcount(((*magic) * mask.raw()) >> 56) < 6;)
                *magic = prng.sparse_rand<Bitboard::UnderlyingT>();

            for (++cnt, i = 0; i < size; ++i) {
                unsigned idx = get_attack_index(occupancy[i], *magic, n_shifts);

                if (epoch[idx] < cnt) {
                    epoch[idx] = cnt;
                    attacks[idx] = reference[i];
                } else if (attacks[idx] != reference[i]) {
                    break;
                }
            }
        }
    }
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

} // namespace

void init() {
    init_magic_attack_tables();
    init_inbetween_masks();
    init_passing_masks();
    init_diagonal_antidiagonal_masks();
}

} // namespace Attacks
