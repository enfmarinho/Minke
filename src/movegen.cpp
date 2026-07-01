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

#include "attacks.h"
#include "move.h"
#include "position.h"
#include "types.h"
#include "utils.h"

namespace Movegen {

template <MoveType move_t>
static inline void push_regular_moves(ScoredMoveList& move_list, const Square from_sq, Bitboard to_sqs) {
    while (to_sqs) {
        Square to = poplsb(to_sqs);
        move_list.push({Move(from_sq, to, move_t), 0});
    }
}

template <MoveType move_t>
static inline void push_pawn_moves(ScoredMoveList& move_list, const Direction offset, Bitboard to_sqs) {
    while (to_sqs) {
        Square to = poplsb(to_sqs);
        Square from = static_cast<Square>(static_cast<int>(to) - offset);
        move_list.push({Move(from, to, move_t), 0});
    }
}

static inline void push_pawn_promotions(ScoredMoveList& move_list, const Direction offset, Bitboard to_sqs,
                                        const bool is_capture) {
    while (to_sqs) {
        MoveType capture = is_capture ? CAPTURE : REGULAR;
        Square to = poplsb(to_sqs);
        Square from = static_cast<Square>(static_cast<int>(to) - offset);
        move_list.push({Move(from, to, static_cast<MoveType>(PAWN_PROMOTION_QUEEN | capture)), 0});
        move_list.push({Move(from, to, static_cast<MoveType>(PAWN_PROMOTION_KNIGHT | capture)), 0});
        move_list.push({Move(from, to, static_cast<MoveType>(PAWN_PROMOTION_ROOK | capture)), 0});
        move_list.push({Move(from, to, static_cast<MoveType>(PAWN_PROMOTION_BISHOP | capture)), 0});
    }
}

static inline void gen_pawn_noisies(ScoredMoveList& move_list, const Position& pos, Bitboard dst_mask) {
    const Direction push = get_pawn_offset(pos.get_stm());
    const Direction west_push = static_cast<Direction>(WEST + push);
    const Direction east_push = static_cast<Direction>(EAST + push);

    const Bitboard pawn_promotion_rank = get_pawn_promotion_rank_mask(pos.get_stm());
    const Bitboard theirs = pos.get_occupancy(pos.get_adversary());

    Bitboard pawns_bb = pos.get_piece_bb(PAWN, pos.get_stm());

    Bitboard west_captures_bb = shift(pawns_bb & NOT_A_FILE, west_push) & dst_mask;
    Bitboard east_captures_bb = shift(pawns_bb & NOT_H_FILE, east_push) & dst_mask;

    // Promotions captures
    Bitboard west_captures_promos_bb = west_captures_bb & pawn_promotion_rank & theirs;
    Bitboard east_captures_promos_bb = east_captures_bb & pawn_promotion_rank & theirs;
    push_pawn_promotions(move_list, west_push, west_captures_promos_bb, true);
    push_pawn_promotions(move_list, east_push, east_captures_promos_bb, true);

    // Promotions
    Bitboard pawn_promos_bb = shift(pawns_bb, push) & dst_mask & ~theirs;
    push_pawn_promotions(move_list, push, pawn_promos_bb, false);

    // Captures that aren't promotions
    Bitboard west_captures_not_promos_bb = west_captures_bb & ~west_captures_promos_bb & theirs;
    Bitboard east_captures_not_promos_bb = east_captures_bb & ~east_captures_promos_bb & theirs;
    push_pawn_moves<CAPTURE>(move_list, west_push, west_captures_not_promos_bb);
    push_pawn_moves<CAPTURE>(move_list, east_push, east_captures_not_promos_bb);

    // En-passant
    Square ep_sq = pos.get_en_passant();
    if (ep_sq != NO_SQ) {
        Bitboard ep_sq_mask = 1ull << ep_sq;

        Bitboard west_attacker_bb = shift(pawns_bb & NOT_A_FILE, west_push) & ep_sq_mask;
        Bitboard east_attacker_bb = shift(pawns_bb & NOT_H_FILE, east_push) & ep_sq_mask;

        push_pawn_moves<EP>(move_list, west_push, west_attacker_bb);
        push_pawn_moves<EP>(move_list, east_push, east_attacker_bb);
    }
}

static inline void gen_pawn_quiets(ScoredMoveList& move_list, const Position& pos, Bitboard dst_mask) {
    const Bitboard occ = pos.get_occupancy();
    const Direction push = get_pawn_offset(pos.get_stm());
    const Direction double_push = static_cast<Direction>(2 * push);
    const Bitboard third_pov_rank = (pos.get_stm() == WHITE ? RANK_MASKS[2] : RANK_MASKS[5]);

    Bitboard pawn_bb = pos.get_piece_bb(PAWN, pos.get_stm());

    Bitboard single_push_bb = shift(pawn_bb, push) & ~occ;
    push_pawn_moves<REGULAR>(move_list, push, single_push_bb & dst_mask);

    Bitboard double_push_bb = shift(single_push_bb & third_pov_rank, push);
    push_pawn_moves<REGULAR>(move_list, double_push, double_push_bb & dst_mask);
}

template <MoveType move_t>
static inline void gen_knights(ScoredMoveList& move_list, const Position& pos, Bitboard dst_mask) {
    Bitboard not_pinned_knights = pos.get_piece_bb(KNIGHT, pos.get_stm()) & ~pos.get_pins();
    while (not_pinned_knights) {
        const Square from = poplsb(not_pinned_knights);
        const Bitboard attacks = knight_attacks[from];
        push_regular_moves<move_t>(move_list, from, attacks & dst_mask);
    }
}

template <MoveType move_t>
static inline void gen_sliders(ScoredMoveList& move_list, const Position& pos, Bitboard dst_mask) {
    const Bitboard occ = pos.get_occupancy();
    // const Bitboard pins = pos.get_pins();
    // const Square king_sq = pos.get_king_placement(pos.get_stm());

    Bitboard queen_bb = pos.get_piece_bb(QUEEN, pos.get_stm());
    Bitboard rook_bb = pos.get_piece_bb(ROOK, pos.get_stm()) | queen_bb;
    Bitboard bishop_bb = pos.get_piece_bb(BISHOP, pos.get_stm()) | queen_bb;

    // auto gen_not_pinned = [&](Bitboard bb, PieceType pt) {
    //     Bitboard not_pinned_bb = bb & ~pins;
    //     while (not_pinned_bb) {
    //         Square from = poplsb(not_pinned_bb);
    //         Bitboard attacks = get_piece_attacks(from, occ, pt);
    //         push_regular_moves<move_t>(move_list, from, attacks & dst_mask);
    //     }
    // };
    // auto gen_pinned = [&](Bitboard bb, PieceType pt) {
    //     Bitboard pinned_bb = bb & pins;
    //     while (pinned_bb) {
    //         Square from = poplsb(pinned_bb);
    //         Bitboard attacks = get_piece_attacks(from, occ, pt);
    //         const Bitboard ray_mask = between_squares[from][king_sq];
    //         push_regular_moves<move_t>(move_list, from, attacks & dst_mask & ray_mask);
    //     }
    // };
    // gen_not_pinned(rook_bb, ROOK);
    // gen_not_pinned(bishop_bb, BISHOP);
    // gen_pinned(rook_bb, ROOK);
    // gen_pinned(bishop_bb, BISHOP);

    auto gen_not_pinned = [&](Bitboard bb, PieceType pt) {
        while (bb) {
            Square from = poplsb(bb);
            Bitboard attacks = get_piece_attacks(from, occ, pt);
            push_regular_moves<move_t>(move_list, from, attacks & dst_mask);
        }
    };
    gen_not_pinned(rook_bb, ROOK);
    gen_not_pinned(bishop_bb, BISHOP);
}

template <MoveType move_t>
static inline void gen_kings(ScoredMoveList& move_list, const Position& pos, Bitboard dst_mask) {
    const Square king_sq = pos.get_king_placement(pos.get_stm());
    const Bitboard attacks = king_attacks[king_sq];

    // NOTE: illegal moves may be generated, the king may go to an attacked sq
    push_regular_moves<move_t>(move_list, king_sq, attacks & dst_mask);
}

static inline void gen_castling(ScoredMoveList& move_list, const Position& pos) {
    Bitboard castle_rooks_stm = pos.get_castle_rooks() & pos.get_occupancy(pos.get_stm());
    Square king_from = pos.get_king_placement(pos.get_stm());
    while (castle_rooks_stm) {
        Square rook_from = poplsb(castle_rooks_stm);
        Square king_to, rook_to;
        if (rook_from > king_from) {
            king_to = g1, rook_to = f1;
        } else {
            king_to = c1, rook_to = d1;
        }
        if (pos.get_stm() == BLACK) {
            king_to = static_cast<Square>(king_to ^ 56);
            rook_to = static_cast<Square>(rook_to ^ 56);
        }

        Bitboard crossing_mask = between_squares[king_from][king_to] | between_squares[rook_from][rook_to] |
                                 (1ULL << king_to) | (1ULL << rook_to);
        crossing_mask &= ~((1ULL << king_from) | (1ULL << rook_from));

        if (crossing_mask & pos.get_occupancy()) // There is a blocker
            continue;

        Bitboard king_crossing = between_squares[king_from][king_to];
        bool illegal = false;
        while (king_crossing) {
            Square sq = poplsb(king_crossing);
            if (pos.is_attacked(sq)) {
                illegal = true;
                break;
            }
        }

        if (illegal) {
            continue;
        }

        move_list.push({Move(king_from, king_to, CASTLING), 0});
    }
}

void noisies(ScoredMoveList& move_list, const Position& pos) {
    Bitboard king_dst_mask = pos.get_occupancy(pos.get_adversary());
    Bitboard dst_mask = king_dst_mask;

    // Promotions are noisy no matter if its a capture or not
    Bitboard pawn_push_promotions = ~pos.get_occupancy(pos.get_stm()) & get_pawn_promotion_rank_mask(pos.get_stm());
    Bitboard pawn_dst_mask = dst_mask | pawn_push_promotions;

    if (pos.in_check()) {
        if (count_bits(pos.get_checkers()) > 1) {
            gen_kings<CAPTURE>(move_list, pos, king_dst_mask);
            return;
        }

        dst_mask = pos.get_checkers();

        pawn_dst_mask = dst_mask;
        pawn_dst_mask |=
            pawn_push_promotions & between_squares[pos.get_king_placement(pos.get_stm())][lsb(pos.get_checkers())];
    }

    gen_pawn_noisies(move_list, pos, pawn_dst_mask);
    gen_knights<CAPTURE>(move_list, pos, dst_mask);
    gen_sliders<CAPTURE>(move_list, pos, dst_mask);
    gen_kings<CAPTURE>(move_list, pos, king_dst_mask);
}

void quiets(ScoredMoveList& move_list, const Position& pos) {
    Bitboard king_dst_mask = ~pos.get_occupancy();
    Bitboard dst_mask = king_dst_mask;

    if (pos.in_check()) {
        if (count_bits(pos.get_checkers()) > 1) {
            gen_kings<REGULAR>(move_list, pos, king_dst_mask);
            return;
        }

        dst_mask = between_squares[pos.get_king_placement(pos.get_stm())][lsb(pos.get_checkers())];
    } else {
        gen_castling(move_list, pos);
    }

    gen_pawn_quiets(move_list, pos, dst_mask & ~get_pawn_promotion_rank_mask(pos.get_stm()));
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
