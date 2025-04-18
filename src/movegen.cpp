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

static inline ScoredMove* gen_pawn_captures(ScoredMove* moves, const Position& position, const int& capture_offset) {
    assert(capture_offset == NORTH_WEST || capture_offset == NORTH_EAST || capture_offset == SOUTH_WEST ||
           capture_offset == SOUTH_EAST);

    Bitboard pawns = position.get_piece_bb(PAWN, position.get_stm()) &
                     (capture_offset - get_pawn_offset(position.get_stm()) == WEST ? NOT_A_FILE : NOT_H_FILE);
    if (!pawns) // No pawns can capture
        return moves;

    Bitboard enemy_targets = position.get_occupancy(position.get_adversary());
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

static inline ScoredMove* gen_pawn_moves(ScoredMove* moves, const Position& position, const MoveGenType gen_type) {
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

        while (single_push_no_promotion) {
            Square to = poplsb(single_push_no_promotion);
            *moves++ = {Move(static_cast<Square>(to - pawn_offset), to, REGULAR), 0};
        }
        while (double_push) {
            Square to = poplsb(double_push);
            *moves++ = {Move(static_cast<Square>(to - 2 * pawn_offset), to, REGULAR), 0};
        }
    }
    if (gen_type & NOISY) {
        while (promotion) {
            Square to = poplsb(promotion);
            *moves++ = {Move(static_cast<Square>(to - pawn_offset), to, PAWN_PROMOTION_QUEEN), 0};
            *moves++ = {Move(static_cast<Square>(to - pawn_offset), to, PAWN_PROMOTION_KNIGHT), 0};
            *moves++ = {Move(static_cast<Square>(to - pawn_offset), to, PAWN_PROMOTION_ROOK), 0};
            *moves++ = {Move(static_cast<Square>(to - pawn_offset), to, PAWN_PROMOTION_BISHOP), 0};
        }

        moves = gen_pawn_captures(moves, position, pawn_offset + WEST);
        moves = gen_pawn_captures(moves, position, pawn_offset + EAST);

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
                                          const MoveGenType gen_type) {
    assert(piece_type >= KNIGHT && piece_type <= KING);

    Bitboard empty_targets = 0ULL;
    Bitboard enemy_targets = 0ULL;
    if (gen_type & QUIET)
        empty_targets = ~position.get_occupancy();
    if (gen_type & NOISY)
        enemy_targets = position.get_occupancy(position.get_adversary());

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
    uint8_t short_right = WHITE_OO;
    uint8_t long_right = WHITE_OOO;
    Bitboard short_castling_crossing_mask = WHITE_OO_CROSSING_MASK;
    Bitboard long_castling_crossing_mask = WHITE_OOO_CROSSING_MASK;
    int first_rank = 0;
    if (position.get_stm() == BLACK) {
        short_right = BLACK_OO;
        long_right = BLACK_OOO;
        short_castling_crossing_mask = BLACK_OO_CROSSING_MASK;
        long_castling_crossing_mask = BLACK_OOO_CROSSING_MASK;
        first_rank = 7;
    }

    Bitboard occupancy = position.get_occupancy();
    uint8_t castling_rights = position.get_castling_rights();
    if (castling_rights & short_right && !(occupancy & short_castling_crossing_mask))
        *moves++ = {Move(get_square(4, first_rank), get_square(6, first_rank), CASTLING), 0};
    if (castling_rights & long_right && !(occupancy & long_castling_crossing_mask))
        *moves++ = {Move(get_square(4, first_rank), get_square(2, first_rank), CASTLING), 0};

    return moves;
}

ScoredMove* gen_moves(ScoredMove* moves, const Position& position, const MoveGenType gen_type) {
    moves = gen_pawn_moves(moves, position, gen_type);
    moves = gen_piece_moves(moves, position, KNIGHT, gen_type);
    moves = gen_piece_moves(moves, position, BISHOP, gen_type);
    moves = gen_piece_moves(moves, position, ROOK, gen_type);
    moves = gen_piece_moves(moves, position, QUEEN, gen_type);
    moves = gen_piece_moves(moves, position, KING, gen_type);
    if (gen_type & QUIET)
        moves = gen_castling_moves(moves, position);

    return moves;
}
