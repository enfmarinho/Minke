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

#include "movegen.h"

#include <cassert>

#include "core/attacks.h"
#include "core/bitboard.h"
#include "core/move.h"
#include "core/position.h"
#include "core/types.h"
#include "utils/utils.h"

namespace Movegen {

template <MoveType move_t>
static inline void push_regular_moves(ScoredMoveList& move_list, const Square from_sq, Bitboard to_sqs) {
    while (to_sqs) {
        Square to = to_sqs.poplsb();
        move_list.push({Move(from_sq, to, move_t), 0});
    }
}

template <MoveType move_t>
static inline void push_pawn_moves(ScoredMoveList& move_list, const Direction offset, Bitboard to_sqs) {
    while (to_sqs) {
        Square to = to_sqs.poplsb();
        Square from = static_cast<Square>(static_cast<int>(to) - offset);
        move_list.push({Move(from, to, move_t), 0});
    }
}

static inline void push_pawn_promotions(ScoredMoveList& move_list, const Direction offset, Bitboard to_sqs,
                                        const bool is_capture) {
    while (to_sqs) {
        MoveType capture = is_capture ? CAPTURE : REGULAR;
        Square to = to_sqs.poplsb();
        Square from = static_cast<Square>(static_cast<int>(to) - offset);
        move_list.push({Move(from, to, static_cast<MoveType>(PAWN_PROMOTION_QUEEN | capture)), 0});
        move_list.push({Move(from, to, static_cast<MoveType>(PAWN_PROMOTION_KNIGHT | capture)), 0});
        move_list.push({Move(from, to, static_cast<MoveType>(PAWN_PROMOTION_ROOK | capture)), 0});
        move_list.push({Move(from, to, static_cast<MoveType>(PAWN_PROMOTION_BISHOP | capture)), 0});
    }
}

static inline void gen_pawn_noisies(ScoredMoveList& move_list, const Position& pos, Bitboard dst_mask) {
    const Color stm = pos.stm();
    const Color nstm = pos.nstm();
    const Direction push = get_pawn_offset(stm);
    const Direction west_capture = static_cast<Direction>(WEST + push);
    const Direction east_capture = static_cast<Direction>(EAST + push);

    const Bitboard pawns_bb = pos.piece_bb(PAWN, pos.stm());
    const Bitboard theirs = pos.occ_bb(pos.nstm());
    const Bitboard pawn_promotion_rank = Bitboard::pawn_promotion_rank(stm);

    // pins bitboards
    const Square king_sq = pos.king_sq(stm);
    const Bitboard pins_bb = pos.pins_bb();
    const Bitboard west_capture_pin_mask = stm == WHITE ? antidiagonal_masks[king_sq] : diagonal_masks[king_sq];
    const Bitboard east_capture_pin_mask = stm == WHITE ? diagonal_masks[king_sq] : antidiagonal_masks[king_sq];

    // captures destination bitboards
    const Bitboard west_attackers_bb = (pawns_bb & ~pins_bb) | (pawns_bb & west_capture_pin_mask);
    const Bitboard west_captures_bb = west_attackers_bb.shift_up_west_pov(stm) & dst_mask & theirs;

    const Bitboard east_attackers_bb = (pawns_bb & ~pins_bb) | (pawns_bb & east_capture_pin_mask);
    const Bitboard east_captures_bb = east_attackers_bb.shift_up_east_pov(stm) & dst_mask & theirs;

    // Promotions captures
    const Bitboard west_captures_promos_bb = west_captures_bb & pawn_promotion_rank;
    const Bitboard east_captures_promos_bb = east_captures_bb & pawn_promotion_rank;
    push_pawn_promotions(move_list, west_capture, west_captures_promos_bb, true);
    push_pawn_promotions(move_list, east_capture, east_captures_promos_bb, true);

    // Promotions
    const Bitboard forward_pin_mask = Bitboard::file(king_sq);
    const Bitboard pawn_possible_promos_bb = (pawns_bb & ~pins_bb) | (pawns_bb & forward_pin_mask);
    const Bitboard pawn_promos_bb = pawn_possible_promos_bb.shift_up_pov(stm) & dst_mask & ~theirs;
    push_pawn_promotions(move_list, push, pawn_promos_bb, false);

    // Captures that aren't promotions
    const Bitboard west_captures_not_promos_bb = west_captures_bb & ~west_captures_promos_bb;
    const Bitboard east_captures_not_promos_bb = east_captures_bb & ~east_captures_promos_bb;
    push_pawn_moves<CAPTURE>(move_list, west_capture, west_captures_not_promos_bb);
    push_pawn_moves<CAPTURE>(move_list, east_capture, east_captures_not_promos_bb);

    // En-passant
    const Square ep_sq = pos.ep_sq();
    if (ep_sq != NO_SQ) {
        const Bitboard ep_sq_mask(ep_sq);

        Bitboard ep_west_captures_bb = west_attackers_bb.shift_up_west_pov(stm) & ep_sq_mask;
        Bitboard ep_east_captures_bb = east_attackers_bb.shift_up_east_pov(stm) & ep_sq_mask;
        const Square captured_pawn_sq = static_cast<Square>(ep_sq - static_cast<int>(push));

        if (pos.in_check()) {
            const bool captures_checker = pos.checkers_bb().is_set(captured_pawn_sq);
            const bool blocks_check = inbetween_masks[king_sq][pos.checkers_bb().lsb()].is_set(ep_sq);

            if (!captures_checker && !blocks_check) {
                ep_west_captures_bb = 0;
                ep_east_captures_bb = 0;
            }
        }

        auto ep_discovers_check = [&](Direction capture_offset, Bitboard ep_attacker_bb) -> bool {
            assert(ep_attacker_bb.popcount() <= 1); // at most one pawn can land on ep_sq from a given direction

            if (!ep_attacker_bb)
                return false;

            const Square from_sq = static_cast<Square>(ep_attacker_bb.lsb() - static_cast<int>(capture_offset));
            const Bitboard occ_no_moved_bb = pos.occ_bb() ^ Bitboard(from_sq) ^ Bitboard(captured_pawn_sq);
            const Bitboard horizontal_attackers = pos.piece_bb(ROOK, nstm) | pos.piece_bb(QUEEN, nstm);

            return static_cast<bool>(get_rook_attacks(king_sq, occ_no_moved_bb) & horizontal_attackers &
                                     Bitboard::rank(king_sq));
        };

        if (!ep_discovers_check(west_capture, ep_west_captures_bb))
            push_pawn_moves<EP>(move_list, west_capture, ep_west_captures_bb);

        if (!ep_discovers_check(east_capture, ep_east_captures_bb))
            push_pawn_moves<EP>(move_list, east_capture, ep_east_captures_bb);
    }
}

static inline void gen_pawn_quiets(ScoredMoveList& move_list, const Position& pos, Bitboard dst_mask) {
    const Color stm = pos.stm();
    const Bitboard occ = pos.occ_bb();
    const Direction push = get_pawn_offset(stm);
    const Direction double_push = static_cast<Direction>(2 * push);

    const Bitboard pov_third_rank_mask = (stm == WHITE ? Bitboard::RANK_3 : Bitboard::RANK_6);
    const Bitboard forward_pin_mask = Bitboard::file(pos.king_sq(stm));
    const Bitboard pins_bb = pos.pins_bb();

    const Bitboard pawns_bb = pos.piece_bb(PAWN, stm);
    const Bitboard movable_pawns_bb = (pawns_bb & ~pins_bb) | (pawns_bb & forward_pin_mask);

    const Bitboard single_push_bb = movable_pawns_bb.shift_up_pov(stm) & ~occ;
    push_pawn_moves<REGULAR>(move_list, push, single_push_bb & dst_mask);

    const Bitboard double_push_bb = (single_push_bb & pov_third_rank_mask).shift_up_pov(stm) & ~occ;
    push_pawn_moves<REGULAR>(move_list, double_push, double_push_bb & dst_mask);
}

template <MoveType move_t>
static inline void gen_knights(ScoredMoveList& move_list, const Position& pos, Bitboard dst_mask) {
    Bitboard not_pinned_knights = pos.piece_bb(KNIGHT, pos.stm()) & ~pos.pins_bb();
    while (not_pinned_knights) {
        const Square from = not_pinned_knights.poplsb();
        const Bitboard attacks = knight_attacks[from];
        push_regular_moves<move_t>(move_list, from, attacks & dst_mask);
    }
}

template <MoveType move_t>
static inline void gen_sliders(ScoredMoveList& move_list, const Position& pos, Bitboard dst_mask) {
    const Color stm = pos.stm();
    const Bitboard occ = pos.occ_bb();
    const Bitboard pins = pos.pins_bb();
    const Square king_sq = pos.king_sq(stm);

    Bitboard queen_bb = pos.piece_bb(QUEEN, stm);
    Bitboard rook_bb = pos.piece_bb(ROOK, stm) | queen_bb;
    Bitboard bishop_bb = pos.piece_bb(BISHOP, stm) | queen_bb;

    auto gen_not_pinned = [&](Bitboard bb, PieceType pt) {
        Bitboard not_pinned_bb = bb & ~pins;
        while (not_pinned_bb) {
            Square from = not_pinned_bb.poplsb();
            Bitboard attacks = get_piece_attacks(from, occ, pt);
            push_regular_moves<move_t>(move_list, from, attacks & dst_mask);
        }
    };
    auto gen_pinned = [&](Bitboard bb, PieceType pt) {
        Bitboard pinned_bb = bb & pins;
        while (pinned_bb) {
            Square from = pinned_bb.poplsb();
            Bitboard attacks = get_piece_attacks(from, occ, pt);
            const Bitboard passing_mask = passing_masks[king_sq][from];
            push_regular_moves<move_t>(move_list, from, attacks & dst_mask & passing_mask);
        }
    };
    gen_not_pinned(rook_bb, ROOK);
    gen_not_pinned(bishop_bb, BISHOP);
    gen_pinned(rook_bb, ROOK);
    gen_pinned(bishop_bb, BISHOP);
}

template <MoveType move_t>
static inline void gen_kings(ScoredMoveList& move_list, const Position& pos, Bitboard dst_mask) {
    const Square king_sq = pos.king_sq(pos.stm());
    const Bitboard attacks = king_attacks[king_sq];

    push_regular_moves<move_t>(move_list, king_sq, attacks & dst_mask & ~pos.threats_bb());
}

static inline void gen_castling(ScoredMoveList& move_list, const Position& pos) {
    const Color stm = pos.stm();
    const Square king_from = pos.king_sq(stm);

    Bitboard castle_rooks_stm = pos.castle_rooks_bb() & pos.occ_bb(stm);
    while (castle_rooks_stm) {
        Square rook_from = castle_rooks_stm.poplsb();
        Square king_to, rook_to;
        if (rook_from > king_from) {
            king_to = g1, rook_to = f1;
        } else {
            king_to = c1, rook_to = d1;
        }
        if (stm == BLACK) {
            // flip sq, to make it from other players pov
            king_to = static_cast<Square>(king_to ^ 56);
            rook_to = static_cast<Square>(rook_to ^ 56);
        }

        const Bitboard crossing_mask = (inbetween_masks[king_from][king_to] | inbetween_masks[rook_from][rook_to] |
                                        Bitboard(king_to) | Bitboard(rook_to)) &
                                       ~(Bitboard(king_from) | Bitboard(rook_from));

        const Bitboard king_crossing = inbetween_masks[king_from][king_to] | Bitboard(king_to);
        if (!(crossing_mask & pos.occ_bb())        // no blocker
            && !(king_crossing & pos.threats_bb()) // no passing square is attacked
        ) {
            // TODO, for datagen FRC compatibility its better to encode castling as king takes rook
            move_list.push({Move(king_from, king_to, CASTLING), 0});
        }
    }
}

void noisies(ScoredMoveList& move_list, const Position& pos) {
    const Color stm = pos.stm();
    const Bitboard king_dst_mask = pos.occ_bb(pos.nstm());
    Bitboard dst_mask = king_dst_mask;

    // Promotions are noisy no matter if its a capture or not
    Bitboard pawn_push_promotions = ~pos.occ_bb(stm) & Bitboard::pawn_promotion_rank(stm);
    Bitboard pawn_dst_mask = dst_mask | pawn_push_promotions;

    if (pos.in_check()) {
        if (pos.checkers_bb().popcount() > 1) {
            gen_kings<CAPTURE>(move_list, pos, king_dst_mask);
            return;
        }

        dst_mask = pos.checkers_bb();

        pawn_dst_mask = dst_mask;
        pawn_dst_mask |= pawn_push_promotions & inbetween_masks[pos.king_sq(stm)][pos.checkers_bb().lsb()];
    }

    gen_pawn_noisies(move_list, pos, pawn_dst_mask);
    gen_knights<CAPTURE>(move_list, pos, dst_mask);
    gen_sliders<CAPTURE>(move_list, pos, dst_mask);
    gen_kings<CAPTURE>(move_list, pos, king_dst_mask);
}

void quiets(ScoredMoveList& move_list, const Position& pos) {
    const Color stm = pos.stm();
    Bitboard king_dst_mask = ~pos.occ_bb();
    Bitboard dst_mask = king_dst_mask;

    if (pos.in_check()) {
        if (pos.checkers_bb().popcount() > 1) {
            gen_kings<REGULAR>(move_list, pos, king_dst_mask);
            return;
        }

        dst_mask = inbetween_masks[pos.king_sq(stm)][pos.checkers_bb().lsb()];
    } else {
        gen_castling(move_list, pos);
    }

    gen_pawn_quiets(move_list, pos, dst_mask & ~Bitboard::pawn_promotion_rank(stm));
    gen_knights<REGULAR>(move_list, pos, dst_mask);
    gen_sliders<REGULAR>(move_list, pos, dst_mask);
    gen_kings<REGULAR>(move_list, pos, king_dst_mask);
}

void all(ScoredMoveList& move_list, const Position& pos) {
    // Can be done more efficiently than this, but its only used for debugging
    noisies(move_list, pos);
    quiets(move_list, pos);
}

} // namespace Movegen
