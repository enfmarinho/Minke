/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "attacks.h"

#include "types.h"
#include "utils.h"

Bitboard generate_bishop_mask(Square sq) {
    Bitboard mask = 0ULL;
    Bitboard board = 0ULL;
    set_bit(board, sq);

    if (board & not_a_file & not_1_rank) {
        for (Bitboard board_cp = shift(board, SouthWest); board_cp & not_a_file & not_1_rank;
             board_cp = shift(board_cp, SouthWest)) {
            mask |= board_cp;
        }
    }
    if (board & not_h_file & not_1_rank) {
        for (Bitboard board_cp = shift(board, SouthEast); board_cp & not_h_file & not_1_rank;
             board_cp = shift(board_cp, SouthEast)) {
            mask |= board_cp;
        }
    }
    if (board & not_a_file & not_8_rank) {
        for (Bitboard board_cp = shift(board, NorthWest); board_cp & not_a_file & not_8_rank;
             board_cp = shift(board_cp, NorthWest)) {
            mask |= board_cp;
        }
    }
    if (board & not_h_file & not_8_rank) {
        for (Bitboard board_cp = shift(board, NorthEast); board_cp & not_h_file & not_8_rank;
             board_cp = shift(board_cp, NorthEast)) {
            mask |= board_cp;
        }
    }

    return mask;
}

Bitboard generate_rook_mask(Square sq) {
    Bitboard mask = 0ULL;
    Bitboard board = 0ULL;
    set_bit(board, sq);

    if (board & not_8_rank) {
        for (Bitboard board_cp = shift(board, North); board_cp & not_8_rank; board_cp = shift(board_cp, North)) {
            mask |= board_cp;
        }
    }
    if (board & not_1_rank) {
        for (Bitboard board_cp = shift(board, South); board_cp & not_1_rank; board_cp = shift(board_cp, South)) {
            mask |= board_cp;
        }
    }
    if (board & not_a_file) {
        for (Bitboard board_cp = shift(board, West); board_cp & not_a_file; board_cp = shift(board_cp, West)) {
            mask |= board_cp;
        }
    }
    if (board & not_h_file) {
        for (Bitboard board_cp = shift(board, East); board_cp & not_h_file; board_cp = shift(board_cp, East)) {
            mask |= board_cp;
        }
    }

    return mask;
}

Bitboard generate_pawn_attack(Square sq, Color color) {
    Bitboard attacks = 0ULL;
    Bitboard board = 0ULL;
    set_bit(board, sq);

    int forward_offset;
    if (color == White)
        forward_offset = North;
    else
        forward_offset = South;

    if (board & not_a_file)
        attacks |= shift(board, forward_offset + West);
    if (board & not_h_file)
        attacks |= shift(board, forward_offset + East);

    return attacks;
}

Bitboard generate_knight_attack(Square sq) {
    Bitboard attacks = 0ULL;
    Bitboard board = 0ULL;
    set_bit(board, sq);

    if (board & not_a_file & not_1_2_rank)
        attacks |= shift(board, 2 * South + West);
    if (board & not_h_file & not_1_2_rank)
        attacks |= shift(board, 2 * South + East);
    if (board & not_a_file & not_7_8_rank)
        attacks |= shift(board, 2 * North + West);
    if (board & not_h_file & not_7_8_rank)
        attacks |= shift(board, 2 * North + East);
    if (board & not_ab_file & not_1_rank)
        attacks |= shift(board, 2 * West + South);
    if (board & not_ab_file & not_8_rank)
        attacks |= shift(board, 2 * West + North);
    if (board & not_hg_file & not_1_rank)
        attacks |= shift(board, 2 * East + South);
    if (board & not_hg_file & not_8_rank)
        attacks |= shift(board, 2 * East + North);

    return attacks;
}

Bitboard generate_bishop_attack(Square sq, const Bitboard& blockers) {
    Bitboard attacks = 0ULL;
    Bitboard board = 0ULL;
    set_bit(board, sq);

    for (Bitboard board_cp = board; board_cp & not_a_file & not_1_rank;) {
        board_cp = shift(board_cp, SouthWest);
        attacks |= board_cp;
        if (board_cp & blockers)
            break;
    }
    for (Bitboard board_cp = board; board_cp & not_h_file & not_1_rank;) {
        board_cp = shift(board_cp, SouthEast);
        attacks |= board_cp;
        if (board_cp & blockers)
            break;
    }
    for (Bitboard board_cp = board; board_cp & not_a_file & not_8_rank;) {
        board_cp = shift(board_cp, NorthWest);
        attacks |= board_cp;
        if (board_cp & blockers)
            break;
    }
    for (Bitboard board_cp = board; board_cp & not_h_file & not_8_rank;) {
        board_cp = shift(board_cp, NorthEast);
        attacks |= board_cp;
        if (board_cp & blockers)
            break;
    }

    return attacks;
}

Bitboard generate_rook_attack(Square sq, const Bitboard& blockers) {
    Bitboard attacks = 0ULL;
    Bitboard board = 0ULL;
    set_bit(board, sq);

    for (Bitboard board_cp = board; board_cp & not_8_rank;) {
        board_cp = shift(board_cp, North);
        attacks |= board_cp;
        if (board_cp & blockers)
            break;
    }
    for (Bitboard board_cp = board; board_cp & not_1_rank;) {
        board_cp = shift(board_cp, South);
        attacks |= board_cp;
        if (board_cp & blockers)
            break;
    }
    for (Bitboard board_cp = board; board_cp & not_a_file;) {
        board_cp = shift(board_cp, West);
        attacks |= board_cp;
        if (board_cp & blockers)
            break;
    }
    for (Bitboard board_cp = board; board_cp & not_h_file;) {
        board_cp = shift(board_cp, East);
        attacks |= board_cp;
        if (board_cp & blockers)
            break;
    }

    return attacks;
}

Bitboard generate_king_attack(Square sq) {
    Bitboard attacks = 0ULL;
    Bitboard board = 0ULL;
    set_bit(board, sq);

    if (board & not_a_file)
        attacks |= shift(board, West);
    if (board & not_h_file)
        attacks |= shift(board, East);
    if (board & not_1_rank)
        attacks |= shift(board, South);
    if (board & not_8_rank)
        attacks |= shift(board, North);
    if (board & not_a_file & not_1_rank)
        attacks |= shift(board, SouthWest);
    if (board & not_a_file & not_8_rank)
        attacks |= shift(board, NorthWest);
    if (board & not_h_file & not_1_rank)
        attacks |= shift(board, SouthEast);
    if (board & not_h_file & not_8_rank)
        attacks |= shift(board, NorthEast);

    return attacks;
}
