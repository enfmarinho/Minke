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

