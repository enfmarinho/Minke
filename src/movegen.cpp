/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "movegen.h"

#include <cassert>
#include <cstdint>

#include "attacks.h"
#include "move.h"
#include "position.h"
#include "types.h"
#include "utils.h"

static inline ScoredMove* gen_pawn_captures(ScoredMove* moves, const Position& position, const Bitboard& destination,
                                            const int& capture_offset) {
    assert(capture_offset == NORTH_WEST || capture_offset == NORTH_EAST || capture_offset == SOUTH_WEST ||
           capture_offset == SOUTH_EAST);

    Bitboard pawns = position.get_piece_bb(PAWN, position.get_stm()) &
                     (capture_offset - get_pawn_offset(position.get_stm()) == WEST ? NOT_A_FILE : NOT_H_FILE);
    if (!pawns) // No pawns can capture
        return moves;

    Bitboard enemy_targets = position.get_occupancy(position.get_adversary()) & destination;
    Bitboard captures = shift(pawns, capture_offset) & enemy_targets;
    Bitboard capture_promotions = captures & RANK_MASKS[get_pawn_promotion_rank(position.get_stm())];
    Bitboard capture_no_promotions = captures & ~capture_promotions;
    while (capture_promotions) {
        Square to = poplsb(capture_promotions);
        *moves++ = {Move(static_cast<Square>(to - capture_offset), to, PAWN_PROMOTION_QUEEN_CAPTURE), 0};
        *moves++ = {Move(static_cast<Square>(to - capture_offset), to, PAWN_PROMOTION_KNIGHT_CAPTURE), 0};
        *moves++ = {Move(static_cast<Square>(to - capture_offset), to, PAWN_PROMOTION_ROOK_CAPTURE), 0};
        *moves++ = {Move(static_cast<Square>(to - capture_offset), to, PAWN_PROMOTION_BISHOP_CAPTURE), 0};
    }
    while (capture_no_promotions) {
        Square to = poplsb(capture_no_promotions);
        *moves++ = {Move(static_cast<Square>(to - capture_offset), to, CAPTURE), 0};
    }
    return moves;
}

static inline ScoredMove* gen_pawn_moves(ScoredMove* moves, const Position& position, const Bitboard& destination,
                                         const MoveGenType gen_type) {
    Color stm = position.get_stm();
    Color adversary = position.get_adversary();
    int pawn_offset = get_pawn_offset(stm);

    Bitboard pawns = position.get_piece_bb(PAWN, stm);
    if (!pawns) // No pawns
        return moves;

    Bitboard empty_targets = ~position.get_occupancy();
    Bitboard single_push = shift(pawns, pawn_offset) & empty_targets;
    Bitboard promotion = single_push & RANK_MASKS[get_pawn_promotion_rank(stm)];

    if (gen_type & QUIET) {
        Bitboard single_push_no_promotion = single_push & ~promotion;
        Bitboard double_push =
            shift(single_push_no_promotion, pawn_offset) & empty_targets & RANK_MASKS[(stm == WHITE ? 3 : 4)];

        single_push_no_promotion &= destination;
        while (single_push_no_promotion) {
            Square to = poplsb(single_push_no_promotion);
            *moves++ = {Move(static_cast<Square>(to - pawn_offset), to, REGULAR), 0};
        }

        double_push &= destination;
        while (double_push) {
            Square to = poplsb(double_push);
            *moves++ = {Move(static_cast<Square>(to - 2 * pawn_offset), to, REGULAR), 0};
        }
    }
    if (gen_type & NOISY) {
        promotion &= destination;
        while (promotion) {
            Square to = poplsb(promotion);
            *moves++ = {Move(static_cast<Square>(to - pawn_offset), to, PAWN_PROMOTION_QUEEN), 0};
            *moves++ = {Move(static_cast<Square>(to - pawn_offset), to, PAWN_PROMOTION_KNIGHT), 0};
            *moves++ = {Move(static_cast<Square>(to - pawn_offset), to, PAWN_PROMOTION_ROOK), 0};
            *moves++ = {Move(static_cast<Square>(to - pawn_offset), to, PAWN_PROMOTION_BISHOP), 0};
        }

        moves = gen_pawn_captures(moves, position, destination, pawn_offset + WEST);
        moves = gen_pawn_captures(moves, position, destination, pawn_offset + EAST);

        Square en_passant = position.get_en_passant();
        if (en_passant != NO_SQ) {
            Bitboard attackers = pawns & pawn_attacks[adversary][en_passant];
            while (attackers) {
                Square from = poplsb(attackers);
                *moves++ = {Move(from, en_passant, EP), 0};
            }
        }
    }

    return moves;
}

static inline ScoredMove* gen_piece_moves(ScoredMove* moves, const Position& position, const PieceType piece_type,
                                          const Bitboard& destination, const MoveGenType gen_type) {
    assert(piece_type >= KNIGHT && piece_type <= KING);

    Bitboard empty_targets = 0ULL;
    Bitboard enemy_targets = 0ULL;
    if (gen_type & QUIET)
        empty_targets = ~position.get_occupancy() & destination;
    if (gen_type & NOISY)
        enemy_targets = position.get_occupancy(position.get_adversary()) & destination;

    Bitboard pieces = position.get_piece_bb(piece_type, position.get_stm());
    Bitboard occupancy = position.get_occupancy();
    while (pieces) {
        Square from = poplsb(pieces);
        Bitboard attacks = get_piece_attacks(from, occupancy, piece_type);

        // Quiet moves
        Bitboard to_bb = attacks & empty_targets;
        while (to_bb) {
            Square to = poplsb(to_bb);
            *moves++ = {Move(from, to, REGULAR), 0};
        }

        // Noisy moves
        to_bb = attacks & enemy_targets;
        while (to_bb) {
            Square to = poplsb(to_bb);
            *moves++ = {Move(from, to, CAPTURE), 0};
        }
    }
    return moves;
}

ScoredMove* gen_castling_moves(ScoredMove* moves, const Position& position) {
    Bitboard castle_rooks_stm = position.get_castle_rooks() & position.get_occupancy(position.get_stm());
    Square king_from = position.get_king_placement(position.get_stm());
    while (castle_rooks_stm) {
        Square rook_from = poplsb(castle_rooks_stm);
        Square king_to, rook_to;
        if (rook_from > king_from) {
            king_to = g1, rook_to = f1;
        } else {
            king_to = c1, rook_to = d1;
        }
        if (position.get_stm() == BLACK) {
            king_to = static_cast<Square>(king_to ^ 56);
            rook_to = static_cast<Square>(rook_to ^ 56);
        }

        Bitboard crossing_mask = between_squares[king_from][king_to] | between_squares[rook_from][rook_to] |
                                 (1ULL << king_to) | (1ULL << rook_to);
        crossing_mask &= ~((1ULL << king_from) | (1ULL << rook_from));

        if (crossing_mask & position.get_occupancy()) // There is a blocker
            continue;

        *moves++ = {Move(king_from, king_to, CASTLING), 0};
    }

    return moves;
}

ScoredMove* gen_moves(ScoredMove* moves, const Position& position, const MoveGenType gen_type) {
    Bitboard destination =
        !position.in_check()
            ? ALL_BITS
            : between_squares[position.get_king_placement(position.get_stm())][lsb(position.get_checkers())] |
                  position.get_checkers();

    if (count_bits(position.get_checkers()) < 2) {
        moves = gen_pawn_moves(moves, position, destination, gen_type);
        moves = gen_piece_moves(moves, position, KNIGHT, destination, gen_type);
        moves = gen_piece_moves(moves, position, BISHOP, destination, gen_type);
        moves = gen_piece_moves(moves, position, ROOK, destination, gen_type);
        moves = gen_piece_moves(moves, position, QUEEN, destination, gen_type);
    }
    moves = gen_piece_moves(moves, position, KING, ALL_BITS, gen_type);

    if (!position.in_check() && (gen_type & QUIET))
        moves = gen_castling_moves(moves, position);

    return moves;
}
