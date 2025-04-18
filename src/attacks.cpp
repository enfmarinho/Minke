/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "attacks.h"

#include "hash.h"
#include "types.h"
#include "utils.h"

Bitboard bishop_masks[64];
Bitboard rook_masks[64];

Bitboard bishop_shifts[64];
Bitboard rook_shifts[64];

Bitboard bishop_magic_numbers[64];
Bitboard rook_magic_numbers[64];

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

        Bitboard mask, *magic, *attacks;
        int n_shifts;
        if (piece_type == BISHOP) {
            mask = bishop_masks[sq] = generate_bishop_mask(sq);
            magic = &bishop_magic_numbers[sq];
            attacks = bishop_attacks[sq];
            n_shifts = bishop_shifts[sq] = 64 - count_bits(mask);
        } else {
            mask = rook_masks[sq] = generate_rook_mask(sq);
            magic = &rook_magic_numbers[sq];
            attacks = rook_attacks[sq];
            n_shifts = rook_shifts[sq] = 64 - count_bits(mask);
        }

        // Use Carry-Rippler trick to enumerate all subsets of mask, store them on
        // occupancy[] and store the corresponding sliding attack bitboard in reference[].
        int size = 0;
        Bitboard blockers = 0;
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
            for (*magic = 0; count_bits(((*magic) * mask) >> 56) < 6;)
                *magic = prng.sparse_rand<Bitboard>();

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
    Bitboard mask = 0ULL;
    Bitboard board = 0ULL;
    set_bit(board, sq);

    if (board & NOT_A_FILE & NOT_1_RANK) {
        for (Bitboard board_cp = shift(board, SOUTH_WEST); board_cp & NOT_A_FILE & NOT_1_RANK;
             board_cp = shift(board_cp, SOUTH_WEST)) {
            mask |= board_cp;
        }
    }
    if (board & NOT_H_FILE & NOT_1_RANK) {
        for (Bitboard board_cp = shift(board, SOUTH_EAST); board_cp & NOT_H_FILE & NOT_1_RANK;
             board_cp = shift(board_cp, SOUTH_EAST)) {
            mask |= board_cp;
        }
    }
    if (board & NOT_A_FILE & NOT_8_RANK) {
        for (Bitboard board_cp = shift(board, NORTH_WEST); board_cp & NOT_A_FILE & NOT_8_RANK;
             board_cp = shift(board_cp, NORTH_WEST)) {
            mask |= board_cp;
        }
    }
    if (board & NOT_H_FILE & NOT_8_RANK) {
        for (Bitboard board_cp = shift(board, NORTH_EAST); board_cp & NOT_H_FILE & NOT_8_RANK;
             board_cp = shift(board_cp, NORTH_EAST)) {
            mask |= board_cp;
        }
    }

    return mask;
}

Bitboard generate_rook_mask(Square sq) {
    Bitboard mask = 0ULL;
    Bitboard board = 0ULL;
    set_bit(board, sq);

    if (board & NOT_8_RANK) {
        for (Bitboard board_cp = shift(board, NORTH); board_cp & NOT_8_RANK; board_cp = shift(board_cp, NORTH)) {
            mask |= board_cp;
        }
    }
    if (board & NOT_1_RANK) {
        for (Bitboard board_cp = shift(board, SOUTH); board_cp & NOT_1_RANK; board_cp = shift(board_cp, SOUTH)) {
            mask |= board_cp;
        }
    }
    if (board & NOT_A_FILE) {
        for (Bitboard board_cp = shift(board, WEST); board_cp & NOT_A_FILE; board_cp = shift(board_cp, WEST)) {
            mask |= board_cp;
        }
    }
    if (board & NOT_H_FILE) {
        for (Bitboard board_cp = shift(board, EAST); board_cp & NOT_H_FILE; board_cp = shift(board_cp, EAST)) {
            mask |= board_cp;
        }
    }

    return mask;
}

Bitboard generate_pawn_attacks(Square sq, Color color) {
    Bitboard attacks = 0ULL;
    Bitboard board = 0ULL;
    set_bit(board, sq);

    int forward_offset = NORTH;
    Bitboard not_last_rank = ~RANK_MASKS[7];
    if (color == BLACK) {
        forward_offset = SOUTH;
        not_last_rank = ~RANK_MASKS[0];
    }

    if (board & NOT_A_FILE & not_last_rank)
        attacks |= shift(board, forward_offset + WEST);
    if (board & NOT_H_FILE & not_last_rank)
        attacks |= shift(board, forward_offset + EAST);

    return attacks;
}

Bitboard generate_knight_attacks(Square sq) {
    Bitboard attacks = 0ULL;
    Bitboard board = 0ULL;
    set_bit(board, sq);

    if (board & NOT_A_FILE & NOT_1_2_RANK)
        attacks |= shift(board, 2 * SOUTH + WEST);
    if (board & NOT_H_FILE & NOT_1_2_RANK)
        attacks |= shift(board, 2 * SOUTH + EAST);
    if (board & NOT_A_FILE & NOT_7_8_RANK)
        attacks |= shift(board, 2 * NORTH + WEST);
    if (board & NOT_H_FILE & NOT_7_8_RANK)
        attacks |= shift(board, 2 * NORTH + EAST);
    if (board & NOT_AB_FILE & NOT_1_RANK)
        attacks |= shift(board, 2 * WEST + SOUTH);
    if (board & NOT_AB_FILE & NOT_8_RANK)
        attacks |= shift(board, 2 * WEST + NORTH);
    if (board & NOT_HG_FILE & NOT_1_RANK)
        attacks |= shift(board, 2 * EAST + SOUTH);
    if (board & NOT_HG_FILE & NOT_8_RANK)
        attacks |= shift(board, 2 * EAST + NORTH);

    return attacks;
}

Bitboard generate_bishop_attacks(Square sq, const Bitboard& blockers) {
    Bitboard attacks = 0ULL;
    Bitboard board = 0ULL;
    set_bit(board, sq);

    for (Bitboard board_cp = board; board_cp & NOT_A_FILE & NOT_1_RANK;) {
        board_cp = shift(board_cp, SOUTH_WEST);
        attacks |= board_cp;
        if (board_cp & blockers)
            break;
    }
    for (Bitboard board_cp = board; board_cp & NOT_H_FILE & NOT_1_RANK;) {
        board_cp = shift(board_cp, SOUTH_EAST);
        attacks |= board_cp;
        if (board_cp & blockers)
            break;
    }
    for (Bitboard board_cp = board; board_cp & NOT_A_FILE & NOT_8_RANK;) {
        board_cp = shift(board_cp, NORTH_WEST);
        attacks |= board_cp;
        if (board_cp & blockers)
            break;
    }
    for (Bitboard board_cp = board; board_cp & NOT_H_FILE & NOT_8_RANK;) {
        board_cp = shift(board_cp, NORTH_EAST);
        attacks |= board_cp;
        if (board_cp & blockers)
            break;
    }

    return attacks;
}

Bitboard generate_rook_attacks(Square sq, const Bitboard& blockers) {
    Bitboard attacks = 0ULL;
    Bitboard board = 0ULL;
    set_bit(board, sq);

    for (Bitboard board_cp = board; board_cp & NOT_8_RANK;) {
        board_cp = shift(board_cp, NORTH);
        attacks |= board_cp;
        if (board_cp & blockers)
            break;
    }
    for (Bitboard board_cp = board; board_cp & NOT_1_RANK;) {
        board_cp = shift(board_cp, SOUTH);
        attacks |= board_cp;
        if (board_cp & blockers)
            break;
    }
    for (Bitboard board_cp = board; board_cp & NOT_A_FILE;) {
        board_cp = shift(board_cp, WEST);
        attacks |= board_cp;
        if (board_cp & blockers)
            break;
    }
    for (Bitboard board_cp = board; board_cp & NOT_H_FILE;) {
        board_cp = shift(board_cp, EAST);
        attacks |= board_cp;
        if (board_cp & blockers)
            break;
    }

    return attacks;
}

Bitboard generate_king_attacks(Square sq) {
    Bitboard attacks = 0ULL;
    Bitboard board = 0ULL;
    set_bit(board, sq);

    if (board & NOT_A_FILE)
        attacks |= shift(board, WEST);
    if (board & NOT_H_FILE)
        attacks |= shift(board, EAST);
    if (board & NOT_1_RANK)
        attacks |= shift(board, SOUTH);
    if (board & NOT_8_RANK)
        attacks |= shift(board, NORTH);
    if (board & NOT_A_FILE & NOT_1_RANK)
        attacks |= shift(board, SOUTH_WEST);
    if (board & NOT_A_FILE & NOT_8_RANK)
        attacks |= shift(board, NORTH_WEST);
    if (board & NOT_H_FILE & NOT_1_RANK)
        attacks |= shift(board, SOUTH_EAST);
    if (board & NOT_H_FILE & NOT_8_RANK)
        attacks |= shift(board, NORTH_EAST);

    return attacks;
}
