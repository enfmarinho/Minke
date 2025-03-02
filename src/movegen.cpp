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
    assert(capture_offset == NorthWest || capture_offset == NorthEast || capture_offset == SouthWest ||
           capture_offset == SouthEast);

    Bitboard pawns = position.get_piece_bb(Pawn, position.get_stm()) &
                     (capture_offset - get_pawn_offset(position.get_stm()) == West ? not_a_file : not_h_file);
    if (!pawns) // No pawns can capture
        return moves;

    Bitboard enemy_targets = position.get_occupancy(position.get_adversary());
    Bitboard captures = shift(pawns, capture_offset) & enemy_targets;
    Bitboard capture_promotions = captures & RankMasks[get_pawn_promotion_rank(position.get_stm())];
    Bitboard capture_no_promotions = captures & ~capture_promotions;
    while (capture_promotions) {
        Square to = poplsb(capture_promotions);
        *moves++ = {Move(static_cast<Square>(to - capture_offset), to, PawnPromotionQueenCapture), 0};
        *moves++ = {Move(static_cast<Square>(to - capture_offset), to, PawnPromotionKnightCapture), 0};
        *moves++ = {Move(static_cast<Square>(to - capture_offset), to, PawnPromotionRookCapture), 0};
        *moves++ = {Move(static_cast<Square>(to - capture_offset), to, PawnPromotionBishopCapture), 0};
    }
    while (capture_no_promotions) {
        Square to = poplsb(capture_no_promotions);
        *moves++ = {Move(static_cast<Square>(to - capture_offset), to, Capture), 0};
    }
    return moves;
}

static inline ScoredMove* gen_pawn_moves(ScoredMove* moves, const Position& position, const MoveGenType gen_type) {
    Color stm = position.get_stm();
    Color adversary = position.get_adversary();
    int pawn_offset = get_pawn_offset(stm);

    Bitboard pawns = position.get_piece_bb(Pawn, stm);
    if (!pawns) // No pawns
        return moves;

    Bitboard empty_targets = ~position.get_occupancy();
    Bitboard single_push = shift(pawns, pawn_offset) & empty_targets;
    Bitboard promotion = single_push & RankMasks[get_pawn_promotion_rank(stm)];

    if (gen_type & Quiet) {
        Bitboard single_push_no_promotion = single_push & ~promotion;
        Bitboard double_push =
            shift(single_push_no_promotion, pawn_offset) & empty_targets & RankMasks[(stm == White ? 3 : 4)];

        while (single_push_no_promotion) {
            Square to = poplsb(single_push_no_promotion);
            *moves++ = {Move(static_cast<Square>(to - pawn_offset), to, Regular), 0};
        }
        while (double_push) {
            Square to = poplsb(double_push);
            *moves++ = {Move(static_cast<Square>(to - 2 * pawn_offset), to, Regular), 0};
        }
    }
    if (gen_type & Noisy) {
        while (promotion) {
            Square to = poplsb(promotion);
            *moves++ = {Move(static_cast<Square>(to - pawn_offset), to, PawnPromotionQueen), 0};
            *moves++ = {Move(static_cast<Square>(to - pawn_offset), to, PawnPromotionKnight), 0};
            *moves++ = {Move(static_cast<Square>(to - pawn_offset), to, PawnPromotionRook), 0};
            *moves++ = {Move(static_cast<Square>(to - pawn_offset), to, PawnPromotionBishop), 0};
        }

        moves = gen_pawn_captures(moves, position, pawn_offset + West);
        moves = gen_pawn_captures(moves, position, pawn_offset + East);

        Square en_passant = position.get_en_passant();
        if (en_passant != NoSquare) {
            Bitboard attackers = pawns & PawnAttacks[adversary][en_passant];
            while (attackers) {
                Square from = poplsb(attackers);
                *moves++ = {Move(from, en_passant, EnPassant), 0};
            }
        }
    }

    return moves;
}

static inline ScoredMove* gen_piece_moves(ScoredMove* moves, const Position& position, const PieceType piece_type,
                                          const MoveGenType gen_type) {
    assert(piece_type >= Knight && piece_type <= King);

    Bitboard empty_targets = 0ULL;
    Bitboard enemy_targets = 0ULL;
    if (gen_type & Quiet)
        empty_targets = ~position.get_occupancy();
    if (gen_type & Noisy)
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
            *moves++ = {Move(from, to, Regular), 0};
        }

        // Noisy moves
        to_bb = attacks & enemy_targets;
        while (to_bb) {
            Square to = poplsb(to_bb);
            *moves++ = {Move(from, to, Capture), 0};
        }
    }
    return moves;
}

ScoredMove* gen_castling_moves(ScoredMove* moves, const Position& position) {
    uint8_t short_right = WhiteShortCastling;
    uint8_t long_right = WhiteLongCastling;
    Bitboard short_castling_crossing_mask = WhiteShortCastlingCrossingMask;
    Bitboard long_castling_crossing_mask = WhiteLongCastlingCrossingMask;
    int first_rank = 0;
    if (position.get_stm() == Black) {
        short_right = BlackShortCastling;
        long_right = BlackLongCastling;
        short_castling_crossing_mask = BlackShortCastlingCrossingMask;
        long_castling_crossing_mask = BlackLongCastlingCrossingMask;
        first_rank = 7;
    }

    Bitboard occupancy = position.get_occupancy();
    uint8_t castling_rights = position.get_castling_rights();
    if (castling_rights & short_right && !(occupancy & short_castling_crossing_mask))
        *moves++ = {Move(get_square(4, first_rank), get_square(6, first_rank), Castling), 0};
    if (castling_rights & long_right && !(occupancy & long_castling_crossing_mask))
        *moves++ = {Move(get_square(4, first_rank), get_square(2, first_rank), Castling), 0};

    return moves;
}

ScoredMove* gen_moves(ScoredMove* moves, const Position& position, const MoveGenType gen_type) {
    moves = gen_pawn_moves(moves, position, gen_type);
    moves = gen_piece_moves(moves, position, Knight, gen_type);
    moves = gen_piece_moves(moves, position, Bishop, gen_type);
    moves = gen_piece_moves(moves, position, Rook, gen_type);
    moves = gen_piece_moves(moves, position, Queen, gen_type);
    moves = gen_piece_moves(moves, position, King, gen_type);
    if (gen_type & Quiet)
        moves = gen_castling_moves(moves, position);

    return moves;
}
