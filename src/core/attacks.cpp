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

#include "core/types.h"
#include "utils/hash.h"
#include "utils/utils.h"

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

Bitboard generate_bishop_mask(Square sq) {
    Bitboard mask;

    // TODO improve readability
    for (Bitboard b(sq); (b = b.shift_north_east()) & ~Bitboard::RANK_8 & ~Bitboard::FILE_H;)
        mask |= b;
    for (Bitboard b(sq); (b = b.shift_north_west()) & ~Bitboard::RANK_8 & ~Bitboard::FILE_A;)
        mask |= b;
    for (Bitboard b(sq); (b = b.shift_south_east()) & ~Bitboard::RANK_1 & ~Bitboard::FILE_H;)
        mask |= b;
    for (Bitboard b(sq); (b = b.shift_south_west()) & ~Bitboard::RANK_1 & ~Bitboard::FILE_A;)
        mask |= b;

    return mask;
}

Bitboard generate_rook_mask(Square sq) {
    Bitboard mask;

    // TODO improve readability
    for (Bitboard b(sq); (b = b.shift_north()) & ~Bitboard::RANK_8;)
        mask |= b;
    for (Bitboard b(sq); (b = b.shift_south()) & ~Bitboard::RANK_1;)
        mask |= b;
    for (Bitboard b(sq); (b = b.shift_west()) & ~Bitboard::FILE_A;)
        mask |= b;
    for (Bitboard b(sq); (b = b.shift_east()) & ~Bitboard::FILE_H;)
        mask |= b;

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

Bitboard generate_bishop_attacks(Square sq, const Bitboard& blockers) {
    Bitboard attacks;
    Bitboard board(sq);

    for (Bitboard bb = board.shift_south_west(); bb; bb = bb.shift_south_west()) {
        attacks |= bb;
        if (bb & blockers)
            break;
    }
    for (Bitboard bb = board.shift_south_east(); bb; bb = bb.shift_south_east()) {
        attacks |= bb;
        if (bb & blockers)
            break;
    }
    for (Bitboard bb = board.shift_north_west(); bb; bb = bb.shift_north_west()) {
        attacks |= bb;
        if (bb & blockers)
            break;
    }
    for (Bitboard bb = board.shift_north_east(); bb; bb = bb.shift_north_east()) {
        attacks |= bb;
        if (bb & blockers)
            break;
    }

    return attacks;
}

Bitboard generate_rook_attacks(Square sq, const Bitboard& blockers) {
    Bitboard attacks;
    Bitboard board(sq);

    for (Bitboard bb = board.shift_north(); bb; bb = bb.shift_north()) {
        attacks |= bb;
        if (bb & blockers)
            break;
    }
    for (Bitboard bb = board.shift_south(); bb; bb = bb.shift_south()) {
        attacks |= bb;
        if (bb & blockers)
            break;
    }
    for (Bitboard bb = board.shift_west(); bb; bb = bb.shift_west()) {
        attacks |= bb;
        if (bb & blockers)
            break;
    }
    for (Bitboard bb = board.shift_east(); bb; bb = bb.shift_east()) {
        attacks |= bb;
        if (bb & blockers)
            break;
    }

    return attacks;
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
