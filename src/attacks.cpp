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

Bitboard BishopMasks[64];
Bitboard RookMasks[64];

Bitboard BishopShifts[64];
Bitboard RookShifts[64];

Bitboard BishopMagicNumbers[64];
Bitboard RookMagicNumbers[64];

Bitboard PawnAttacks[2][64];
Bitboard KnightAttacks[64];
Bitboard KingAttacks[64];
Bitboard BishopAttacks[64][512];
Bitboard RookAttacks[64][4096];

//=== Adapted from Stockfish.
// Initialize all attacks, masks, magics and shifts tables for piece_type.
// piece_type must by Bishop or Rook
void init_magic_table(PieceType piece_type) {
    assert(piece_type == Bishop || piece_type == Rook);

    // Maybe optimal PRNG seeds to pick the correct magics in the shortest time
    int seeds[8] = {728, 10316, 55013, 32803, 12281, 15100, 16645, 255};

    Bitboard occupancy[4096];
    Bitboard reference[4096];
    int epoch[4096] = {}, cnt = 0;

    for (int sqi = a1; sqi <= h8; ++sqi) {
        Square sq = static_cast<Square>(sqi);

        Bitboard mask, *magic, *attacks;
        int n_shifts;
        if (piece_type == Bishop) {
            mask = BishopMasks[sq] = generate_bishop_mask(sq);
            magic = &BishopMagicNumbers[sq];
            attacks = BishopAttacks[sq];
            n_shifts = BishopShifts[sq] = 64 - count_bits(mask);
        } else {
            mask = RookMasks[sq] = generate_rook_mask(sq);
            magic = &RookMagicNumbers[sq];
            attacks = RookAttacks[sq];
            n_shifts = RookShifts[sq] = 64 - count_bits(mask);
        }

        // Use Carry-Rippler trick to enumerate all subsets of mask, store them on
        // occupancy[] and store the corresponding sliding attack bitboard in reference[].
        int size = 0;
        Bitboard blockers = 0;
        do {
            occupancy[size] = blockers;
            reference[size] =
                (piece_type == Bishop) ? generate_bishop_attacks(sq, blockers) : generate_rook_attacks(sq, blockers);

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

Bitboard generate_pawn_attacks(Square sq, Color color) {
    Bitboard attacks = 0ULL;
    Bitboard board = 0ULL;
    set_bit(board, sq);

    int forward_offset = North;
    Bitboard not_last_rank = ~RankMasks[7];
    if (color == Black) {
        forward_offset = South;
        not_last_rank = ~RankMasks[0];
    }

    if (board & not_a_file & not_last_rank)
        attacks |= shift(board, forward_offset + West);
    if (board & not_h_file & not_last_rank)
        attacks |= shift(board, forward_offset + East);

    return attacks;
}

Bitboard generate_knight_attacks(Square sq) {
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

Bitboard generate_bishop_attacks(Square sq, const Bitboard& blockers) {
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

Bitboard generate_rook_attacks(Square sq, const Bitboard& blockers) {
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

Bitboard generate_king_attacks(Square sq) {
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
