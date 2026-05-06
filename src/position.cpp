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

#include "position.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

#include "attacks.h"
#include "eval/nnue.h"
#include "hash.h"
#include "move.h"
#include "movegen.h"
#include "types.h"
#include "utils.h"

Position::Position() { set_fen<true>(START_FEN); }

template <bool UPDATE_NNUE>
bool Position::set_fen(const std::string &fen) {
    reset<UPDATE_NNUE>();

    std::stringstream iss(fen);
    std::array<std::string, 6> fen_arguments;
    for (int index = 0; index < 6; ++index) {
        iss >> std::skipws >> fen_arguments[index];
        if (iss.fail()) {
            std::cerr << "INVALID FEN: wrong format." << std::endl;
            return false;
        }
    }

    int rank = 7, file = 0;
    for (char c : fen_arguments[0]) {
        if (c == '/') {
            --rank;
            file = 0;
            continue;
        }
        if (!std::isdigit(c)) {
            char piece_char = std::tolower(c);
            Color player = std::isupper(c) ? WHITE : BLACK;
            Square sq = get_square(file, rank);
            const PieceType pt = [](const char pchar) {
                switch (pchar) {
                    case 'p':
                        return PAWN;
                    case 'n':
                        return KNIGHT;
                    case 'b':
                        return BISHOP;
                    case 'r':
                        return ROOK;
                    case 'q':
                        return QUEEN;
                    case 'k':
                        return KING;
                    default:
                        __builtin_unreachable();
                }
            }(piece_char);

            add_piece<UPDATE_NNUE>({get_piece(pt, player), sq});

            ++file;
        } else {
            file += c - '0';
        }
    }

    if (fen_arguments[1] == "w" || fen_arguments[1] == "W") {
        stm() = WHITE;
    } else if (fen_arguments[1] == "b" || fen_arguments[1] == "B") {
        stm() = BLACK;
        hash_side_key();
    } else {
        std::cerr << "INVALID FEN: invalid player, it should be 'w' or 'b'." << std::endl;
        return false;
    }

    for (char castling : fen_arguments[2]) {
        if (castling == 'K') {
            set_bits(castling_rights(), static_cast<uint8_t>(WHITE_OO));
            set_bit(castle_rooks(), msb(piece_bb(WHITE_ROOK) & RANK_MASKS[0]));
        } else if (castling == 'Q') {
            set_bits(castling_rights(), static_cast<uint8_t>(WHITE_OOO));
            set_bit(castle_rooks(), lsb(piece_bb(WHITE_ROOK) & RANK_MASKS[0]));
        } else if (castling == 'k') {
            set_bits(castling_rights(), static_cast<uint8_t>(BLACK_OO));
            set_bit(castle_rooks(), msb(piece_bb(BLACK_ROOK) & RANK_MASKS[7]));
        } else if (castling == 'q') {
            set_bits(castling_rights(), static_cast<uint8_t>(BLACK_OOO));
            set_bit(castle_rooks(), lsb(piece_bb(BLACK_ROOK) & RANK_MASKS[7]));
        } else if ('A' <= castling && castling <= 'H') {
            Square sq = get_square(castling - 'A', 0);
            set_bits(castling_rights(), static_cast<uint8_t>(king_placement(WHITE) > sq ? WHITE_OOO : WHITE_OO));
            set_bit(castle_rooks(), sq);
        } else if ('a' <= castling && castling <= 'h') {
            Square sq = get_square(castling - 'a', 7);
            set_bits(castling_rights(), static_cast<uint8_t>(king_placement(BLACK) > sq ? BLACK_OOO : BLACK_OO));
            set_bit(castle_rooks(), sq);
        }
    }
    hash_castle_key();

    if (fen_arguments[3] == "-") {
        en_passant() = NO_SQ;
    } else {
        en_passant() = get_square(fen_arguments[3][0] - 'a', fen_arguments[3][1] - '1');
        hash_ep_key();
    }

    try {
        fifty_move_ply() = std::stoi(fen_arguments[4]);
    } catch (const std::exception &) {
        std::cerr << "INVALID FEN: halfmove clock is not a number." << std::endl;
        return false;
    }
    try {
        game_ply() = (std::stoi(fen_arguments[5]) - 1) * 2 + stm();
    } catch (const std::exception &) {
        std::cerr << "INVALID FEN: game clock is not a number." << std::endl;
        return false;
    }
    update_pin_and_checkers_bb();

    if constexpr (UPDATE_NNUE)
        reset_nnue();

    return true;
}

template bool Position::set_fen<true>(const std::string &fen);
template bool Position::set_fen<false>(const std::string &fen);

std::string Position::fen() const {
    std::string fen;
    for (int rank = 7; rank >= 0; --rank) {
        int counter = 0;
        for (int file = 0; file < 8; ++file) {
            const Piece &piece = consult(get_square(file, rank));
            const Color color = get_color(piece);
            const PieceType piece_type = get_piece_type(piece, color);
            if (piece == EMPTY) {
                ++counter;
                continue;
            } else if (counter > 0) {
                fen += ('0' + counter);
                counter = 0;
            }

            char piece_char;
            if (piece_type == PAWN)
                piece_char = 'p';
            else if (piece_type == KNIGHT)
                piece_char = 'n';
            else if (piece_type == BISHOP)
                piece_char = 'b';
            else if (piece_type == ROOK)
                piece_char = 'r';
            else if (piece_type == QUEEN)
                piece_char = 'q';
            else if (piece_type == KING)
                piece_char = 'k';
            else
                __builtin_unreachable();

            if (color == WHITE)
                fen += toupper(piece_char);
            else
                fen += piece_char;
        }
        if (counter > 0)
            fen += ('0' + counter);
        if (rank != 0)
            fen += '/';
    }
    fen += (stm() == WHITE ? " w " : " b ");
    bool none = true;
    if (castling_rights() & WHITE_OO) {
        none = false;
        fen += "K";
    }
    if (castling_rights() & WHITE_OOO) {
        none = false;
        fen += "Q";
    }
    if (castling_rights() & BLACK_OO) {
        none = false;
        fen += "k";
    }
    if (castling_rights() & BLACK_OOO) {
        none = false;
        fen += "q";
    }
    if (none)
        fen += "-";

    fen += ' ';
    if (en_passant() == NO_SQ) {
        fen += "-";
    } else {
        fen += (get_file(en_passant()) + 'a');
        fen += (stm() == WHITE ? '6' : '3');
    }
    fen += ' ';

    fen += std::to_string(fifty_move_ply());
    fen += ' ';
    fen += std::to_string(1 + (game_ply() - stm()) / 2);

    return fen;
}

void Position::reset_nnue() { m_nnue.refresh(*this); }

void Board::reset() {
    for (int sqi = a1; sqi <= h8; ++sqi)
        mailbox[sqi] = EMPTY;
    std::memset(piece_bbs, 0ULL, sizeof(piece_bbs));
    std::memset(occupancy_bbs, 0ULL, sizeof(occupancy_bbs));
    checkers_bb = 0ULL;
    pins_bb = 0ULL;
    castle_rooks_bb = 0ULL;

    ply_from_null = 0;
    fifty_move_ply = 0;
    history_ply = 0;
    game_clock_ply = 0;

    hash_key = 0ULL;

    castling_rights = 0;
    en_passant = NO_SQ;
    stm = WHITE;
}

template <bool UPDATE_NNUE>
void Position::reset() {
    m_history_top = 0;
    m_stack_top = 0;
    board().reset();
}

template <bool UPDATE_NNUE>
void Position::add_piece(const PieceSquare &ps) {
    assert(ps.piece >= WHITE_PAWN && ps.piece <= BLACK_KING);
    assert(ps.sq >= a1 && ps.sq <= h8);

    Color color = get_color(ps.piece);
    set_bit(occupancy(color), ps.sq);
    set_bit(piece_bb(ps.piece), ps.sq);
    consult(ps.sq) = ps.piece;

    hash_piece_key(ps);
}

template <bool UPDATE_NNUE>
void Position::remove_piece(const PieceSquare &ps) {
    assert(ps.piece >= WHITE_PAWN && ps.piece <= BLACK_KING);
    assert(ps.sq >= a1 && ps.sq <= h8);

    Color color = get_color(ps.piece);
    unset_bit(occupancy(color), ps.sq);
    unset_bit(piece_bb(ps.piece), ps.sq);
    consult(ps.sq) = EMPTY;

    hash_piece_key(ps);
}

template <bool UPDATE_NNUE>
bool Position::make_move(const Move &move) {
    push_board();

    ++game_ply();
    ++fifty_move_ply();
    ++ply_from_null();

    if (en_passant() != NO_SQ) {
        hash_ep_key();
        en_passant() = NO_SQ;
    }

    const DirtyPiece dp = [&]() {
        if (move.is_regular())
            return make_regular<UPDATE_NNUE>(move);
        else if (move.is_capture() && !move.is_ep())
            return make_capture<UPDATE_NNUE>(move);
        else if (move.is_castle())
            return make_castle<UPDATE_NNUE>(move);
        else if (move.is_promotion())
            return make_promotion<UPDATE_NNUE>(move);
        else if (move.is_ep())
            return make_en_passant<UPDATE_NNUE>(move);
        else
            __builtin_unreachable();
    }();

    if constexpr (UPDATE_NNUE)
        m_nnue.push(dp);

    hash_castle_key();
    update_castling_rights(move);
    hash_castle_key();
    hash_side_key();

    bool legal = !is_attacked(king_placement(stm()));

    change_side();
    update_pin_and_checkers_bb();
    return legal;
}

template bool Position::make_move<true>(const Move &move);
template bool Position::make_move<false>(const Move &move);

template <bool UPDATE_NNUE>
DirtyPiece Position::make_regular(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(from);

    DirtyPiece dp;
    dp.move_type = REGULAR;
    dp.sub0 = {piece, from};
    dp.add0 = {piece, to};

    remove_piece<UPDATE_NNUE>(dp.sub0);
    add_piece<UPDATE_NNUE>(dp.add0);

    if (get_piece_type(piece, stm()) == PAWN) {
        fifty_move_ply() = 0;
        int pawn_offset = get_pawn_offset(stm());
        if (to - from == 2 * pawn_offset &&
            (pawn_attacks[stm()][to - pawn_offset] &
             piece_bb(PAWN, ntm()))) { // Double push and there is a enemy pawn to en passant
            en_passant() = static_cast<Square>(to - pawn_offset);
            hash_ep_key();
        }
    }

    return dp;
}

template <bool UPDATE_NNUE>
DirtyPiece Position::make_capture(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(from);

    fifty_move_ply() = 0;
    Piece captured = consult(to);
    assert(captured != EMPTY && get_piece_type(captured) != KING);

    DirtyPiece dp;
    dp.move_type = CAPTURE;
    dp.sub0 = {piece, from};
    dp.sub1 = {captured, to};
    dp.add0 = {piece, to};

    if (move.is_promotion())
        dp.add0.piece = get_piece(move.promotee(), stm());

    remove_piece<UPDATE_NNUE>(dp.sub0);
    remove_piece<UPDATE_NNUE>(dp.sub1);
    add_piece<UPDATE_NNUE>(dp.add0);

    return dp;
}

template <bool UPDATE_NNUE>
DirtyPiece Position::make_castle(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    Piece king = consult(from);
    Piece rook = get_piece(ROOK, stm());

    Bitboard stm_castling_rooks = castle_rooks() & piece_bb(rook) & RANK_MASKS[stm() == WHITE ? 0 : 7];

    Square rook_from = [&]() {
        if (to == c1 || to == c8)
            return lsb(stm_castling_rooks);
        else
            return msb(stm_castling_rooks);
    }();
    Square rook_to = [&]() {
        switch (to) { // TODO: incompatible with FRC
            case g1:  // White castle short
                return f1;
            case c1: // White castle long
                return d1;
            case g8: // Black castle short
                return f8;
            case c8: // Black castle long
                return d8;
            default:
                __builtin_unreachable();
        }
    }();

    DirtyPiece dp;
    dp.move_type = CASTLING;
    dp.sub0 = {king, from};
    dp.sub1 = {rook, rook_from};
    dp.add0 = {king, to};
    dp.add1 = {rook, rook_to};

    remove_piece<UPDATE_NNUE>(dp.sub0);
    remove_piece<UPDATE_NNUE>(dp.sub1);
    add_piece<UPDATE_NNUE>(dp.add0);
    add_piece<UPDATE_NNUE>(dp.add1);

    return dp;
}

template <bool UPDATE_NNUE>
DirtyPiece Position::make_promotion(const Move &move) {
    const Square from = move.from();
    const Square to = move.to();

    DirtyPiece dp;
    dp.move_type = REGULAR;
    dp.sub0 = {consult(from), from};
    dp.add0 = {get_piece(move.promotee(), stm()), to};

    remove_piece<UPDATE_NNUE>(dp.sub0);
    add_piece<UPDATE_NNUE>(dp.add0);

    return dp;
}

template <bool UPDATE_NNUE>
DirtyPiece Position::make_en_passant(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(from);
    Square captured_square = static_cast<Square>(to - static_cast<int>(get_pawn_offset(stm())));
    Piece captured = consult(captured_square);

    fifty_move_ply() = 0;

    DirtyPiece dp;
    dp.move_type = CAPTURE;
    dp.sub0 = {piece, from};
    dp.sub1 = {captured, captured_square};
    dp.add0 = {piece, to};

    remove_piece<UPDATE_NNUE>(dp.sub0);
    remove_piece<UPDATE_NNUE>(dp.sub1);
    add_piece<UPDATE_NNUE>(dp.add0);

    return dp;
}

void Position::update_castling_rights(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    PieceType moved_piece_type = get_piece_type(consult(to), stm()); // Piece has already been moved

    if (moved_piece_type == KING) { // Moved king
        switch (stm()) {
            case WHITE:
                unset_mask(castling_rights(), static_cast<uint8_t>(WHITE_CASTLING));
                unset_mask(castle_rooks(), RANK_MASKS[0]);
                break;
            case BLACK:
                unset_mask(castling_rights(), static_cast<uint8_t>(BLACK_CASTLING));
                unset_mask(castle_rooks(), RANK_MASKS[7]);
                break;
            default:
                __builtin_unreachable();
        }
    } else if (moved_piece_type == ROOK) { // Moved rook
        Bitboard from_bb = 1ULL << from;
        if (from_bb & castle_rooks()) {
            unset_mask(castle_rooks(), from_bb);

            if (from > king_placement(stm())) {
                unset_mask(castling_rights(), static_cast<uint8_t>(stm() == WHITE ? WHITE_OO : BLACK_OO));
            } else {
                unset_mask(castling_rights(), static_cast<uint8_t>(stm() == WHITE ? WHITE_OOO : BLACK_OOO));
            }
        }
    }
    Bitboard to_bb = 1ULL << to;
    if (to_bb & castle_rooks()) {
        unset_mask(castle_rooks(), to_bb);
        if (to > king_placement(ntm())) {
            unset_mask(castling_rights(), static_cast<uint8_t>(ntm() == WHITE ? WHITE_OO : BLACK_OO));
        } else {
            unset_mask(castling_rights(), static_cast<uint8_t>(ntm() == WHITE ? WHITE_OOO : BLACK_OOO));
        }
    }
}

template <bool UPDATE_NNUE>
void Position::unmake_move() {
    pop_board();
    if constexpr (UPDATE_NNUE)
        m_nnue.pop();
}

template void Position::unmake_move<true>();
template void Position::unmake_move<false>();

void Position::make_null_move() {
    push_board();

    ply_from_null() = 0;
    ++fifty_move_ply();
    ++game_ply();
    if (en_passant() != NO_SQ) {
        hash_ep_key();
        en_passant() = NO_SQ;
    }
    hash_side_key();
    change_side();
    update_pin_and_checkers_bb();
}

void Position::unmake_null_move() { pop_board(); }

void Position::update_pin_and_checkers_bb() {
    Color adversary = ntm();
    Square king_sq = king_placement(stm());
    pins() = 0;
    checkers() = (pawn_attacks[stm()][king_sq] & piece_bb(PAWN, adversary)) // Pawns
                 | (knight_attacks[king_sq] & piece_bb(KNIGHT, adversary)); // Knights;

    Bitboard slider_checkers =
        ((piece_bb(QUEEN, adversary) | piece_bb(BISHOP, adversary)) & get_bishop_attacks(king_sq, 0)) |
        ((piece_bb(QUEEN, adversary) | piece_bb(ROOK, adversary)) & get_rook_attacks(king_sq, 0));
    while (slider_checkers) {
        Square sq = poplsb(slider_checkers);

        Bitboard blockers = between_squares[king_sq][sq] & occupancy();
        if (!blockers) {
            set_bit(checkers(), sq);
        } else if (count_bits(blockers) == 1) {
            set_bits(pins(), blockers & occupancy(stm()));
        }
    }
}

bool Position::is_attacked(const Square &sq) const {
    Color opponent = ntm();
    Bitboard occ = occupancy();
    unset_bit(occ, sq); // square to be checked has to be unset on occupancy bitboard

    // Check if sq is attacked by opponent pawns. Note: pawn attack mask has to be "stm" because the logic is reversed
    if (piece_bb(PAWN, opponent) & pawn_attacks[stm()][sq])
        return true;

    // Check if sq is attacked by opponent knights
    if (piece_bb(KNIGHT, opponent) & knight_attacks[sq])
        return true;

    // Check if sq is attacked by opponent bishops or queens
    if ((piece_bb(BISHOP, opponent) | piece_bb(QUEEN, opponent)) & get_bishop_attacks(sq, occ))
        return true;

    // Check if sq is attacked by opponent rooks or queens
    if ((piece_bb(ROOK, opponent) | piece_bb(QUEEN, opponent)) & get_rook_attacks(sq, occ))
        return true;

    // Check if sq is attacked by opponent king. Unnecessary when checking for checks
    if (piece_bb(KING, opponent) & king_attacks[sq])
        return true;

    return false;
}

Bitboard Position::attackers(const Square &sq) const {
    Bitboard attackers = 0ULL;
    Bitboard occ = occupancy();

    attackers |= pawn_attacks[WHITE][sq] & piece_bb(PAWN, BLACK);
    attackers |= pawn_attacks[BLACK][sq] & piece_bb(PAWN, WHITE);
    attackers |= get_piece_attacks(sq, occ, KNIGHT) & piece_bb(KNIGHT);
    attackers |= get_piece_attacks(sq, occ, BISHOP) & (piece_bb(BISHOP) | piece_bb(QUEEN));
    attackers |= get_piece_attacks(sq, occ, ROOK) & (piece_bb(ROOK) | piece_bb(QUEEN));
    attackers |= get_piece_attacks(sq, occ, KING) & piece_bb(KING);

    return attackers;
}

int Position::legal_move_amount() {
    ScoredMove moves[MAX_MOVES_PER_POS];
    ScoredMove *end = gen_moves(moves, *this, GEN_ALL);
    int legal_amount = 0;
    for (ScoredMove *begin = moves; begin != end; ++begin) {
        if (make_move<false>(begin->move))
            ++legal_amount;
        unmake_move<false>();
    }
    return legal_amount;
}

bool Position::no_legal_moves() {
    ScoredMove moves[MAX_MOVES_PER_POS];
    ScoredMove *end = gen_moves(moves, *this, GEN_ALL);
    for (ScoredMove *curr = moves; curr != end; ++curr) {
        bool legal = make_move<false>(curr->move);
        unmake_move<false>();

        if (legal)
            return false;
    }
    return true;
}

bool Position::is_legal(const Move &move) {
    Square king_sq = king_placement(stm());
    Square from = move.from();
    Square to = move.to();
    PieceType moved_pt = get_piece_type(consult(from));

    if (move.is_castle()) {
        Piece rook = get_piece(ROOK, stm());
        Bitboard stm_castling_rooks = castle_rooks() & piece_bb(rook) & RANK_MASKS[stm() == WHITE ? 0 : 7];
        Square rook_from = msb(stm_castling_rooks);
        if (to == c1 || to == c8) {
            rook_from = lsb(stm_castling_rooks);
        }
        return !is_attacked(to) && !(pins() & (1ULL << rook_from)); // Other clauses were checked by movegen
    }
    if (move.is_ep()) {
        int pawn_offset = (stm() == WHITE ? NORTH : SOUTH);
        Piece stm_pawn = get_piece(PAWN, stm());
        Piece ntm_pawn = get_piece(PAWN, ntm());
        remove_piece<false>({stm_pawn, from});
        remove_piece<false>({ntm_pawn, static_cast<Square>(to - pawn_offset)});
        add_piece<false>({stm_pawn, to});
        bool is_king_attacked = is_attacked(king_sq);
        add_piece<false>({stm_pawn, from});
        add_piece<false>({ntm_pawn, static_cast<Square>(to - pawn_offset)});
        remove_piece<false>({stm_pawn, to});
        return !is_king_attacked;
    }
    if (moved_pt == KING) {
        remove_piece<false>({get_piece(KING, stm()), king_sq});
        bool is_king_attacked = is_attacked(to);
        add_piece<false>({get_piece(KING, stm()), king_sq});
        return !is_king_attacked;
    }

    if (count_bits(checkers()) > 1) // Double check can only be evaded by king movements
        return false;

    if (pins() & (1ULL << from)) // if piece is pinned, it must keep blocking the check
        return !checkers() &&
               (((1ULL << from) & between_squares[king_sq][to]) || ((1ULL << to) & between_squares[king_sq][from]));

    if (checkers()) // If in check and not moving the king, it must either block the check or take the attacker
        return (1ULL << to) & (checkers() | between_squares[lsb(checkers())][king_sq]);

    return true;
}

// TODO if the moved piece and/or the capture piece is present in the move itself this could be way faster
bool Position::is_pseudo_legal(const Move &move) const {
    if (move == MOVE_NONE)
        return false;

    Square from = move.from();
    Square to = move.to();
    Piece moved_piece = consult(from);
    PieceType moved_piece_type = get_piece_type(moved_piece, stm());

    // No piece in "from" square or piece is not stm
    if (moved_piece == EMPTY || get_color(moved_piece) != stm())
        return false;
    if (get_color(consult(to)) == stm()) // stm piece on "to" square
        return false;
    if (move.is_capture() && !move.is_ep() && consult(to) == EMPTY)
        return false;
    if ((!move.is_capture() || move.is_ep()) && consult(to) != EMPTY)
        return false;
    if (moved_piece_type != PAWN && (move.is_ep() || move.is_promotion()))
        return false;

    // get_piece_attacks can't be called when piece_type = PAWN, so this has to cause an early return clause
    if (moved_piece_type == PAWN) {
        return pawn_pseudo_legal(from, to, move);
    }

    // Castling moves has to cause an early return because castling is a border case for the king attacks array
    if (move.is_castle()) {
        return castling_pseudo_legal(from, to, moved_piece_type);
    }

    Bitboard moved_piece_attacks = get_piece_attacks(from, occupancy(), moved_piece_type);
    return moved_piece_attacks & (1ULL << to);
}

bool Position::pawn_pseudo_legal(const Square &from, const Square &to, const Move &move) const {
    int pawn_offset = get_pawn_offset(stm());
    if (move.is_ep()) {
        if (en_passant() != to || !(piece_bb(PAWN, ntm()) & (1ULL << (to - pawn_offset))))
            return false;
    } else if (move.is_capture()) {
        if (!(pawn_attacks[stm()][from] & (1ULL << to)))
            return false;
    } else if (from + 2 * pawn_offset == to) {
        if (get_rank(from) != get_pawn_start_rank(stm()) || consult(static_cast<Square>(from + pawn_offset)) != EMPTY)
            return false;
    } else if (from + pawn_offset != to) {
        return false;
    } else if (move.is_promotion()) {
        int from_rank = get_rank(from);
        int to_rank = get_rank(to);

        if (stm() == WHITE && (from_rank != 6 || to_rank != 7))
            return false;
        if (stm() == BLACK && (from_rank != 1 || to_rank != 0))
            return false;
    } else if ((stm() == WHITE && get_rank(to) == 7) ||
               (stm() == BLACK && get_rank(to) == 0)) { // No promotion flag in promotion rank
        return false;
    }

    return true;
}

bool Position::castling_pseudo_legal(const Square &from, const Square &to, const PieceType &moved_piece_type) const {
    if (moved_piece_type != KING)
        return false;

    bool castling_short = (from == e1 && to == g1) || (from == e8 && to == g8);
    bool castling_long = (from == e1 && to == c1) || (from == e8 && to == c8);

    if (!castling_short && !castling_long)
        return false;

    uint8_t short_right = WHITE_OO;
    uint8_t long_right = WHITE_OOO;
    Bitboard short_castling_crossing_mask = WHITE_OO_CROSSING_MASK;
    Bitboard long_castling_crossing_mask = WHITE_OOO_CROSSING_MASK;
    if (stm() == BLACK) {
        short_right = BLACK_OO;
        long_right = BLACK_OOO;
        short_castling_crossing_mask = BLACK_OO_CROSSING_MASK;
        long_castling_crossing_mask = BLACK_OOO_CROSSING_MASK;
    }

    if (castling_short && (!(castling_rights() & short_right) || (occupancy() & short_castling_crossing_mask))) {
        return false;
    }
    if (castling_long && (!(castling_rights() & long_right) || (occupancy() & long_castling_crossing_mask))) {
        return false;
    }

    return true;
}

void Position::print() {
    auto print_line = []() -> void {
        for (IndexType i = 0; i < 8; ++i) {
            std::cout << "+";
            for (IndexType j = 0; j < 3; ++j)
                std::cout << "-";
        }
        std::cout << "+\n";
    };

    for (int rank = 7; rank >= 0; --rank) {
        print_line();
        for (int file = 0; file < 8; ++file) {
            Square sq = get_square(file, rank);
            Piece piece = consult(sq);
            PieceType piece_type = get_piece_type(piece);
            char piece_char = '-';
            if (piece_type == PAWN)
                piece_char = 'p';
            else if (piece_type == KNIGHT)
                piece_char = 'n';
            else if (piece_type == BISHOP)
                piece_char = 'b';
            else if (piece_type == ROOK)
                piece_char = 'r';
            else if (piece_type == QUEEN)
                piece_char = 'q';
            else if (piece_type == KING)
                piece_char = 'k';

            if (piece <= WHITE_KING) // Piece is white
                piece_char = toupper(piece_char);

            std::string color = "";
            if (checkers() & (1ULL << sq)) {
                color = "\033[31m";
            } else if (pins() & (1ULL << sq)) {
                color = "\033[34m";
            }

            std::cout << "| " << color << piece_char << (color != "" ? "\033[0m" : "") << " ";
        }
        std::cout << "| " << rank + 1 << "\n";
    }

    print_line();
    for (char rank_simbol = 'a'; rank_simbol <= 'h'; ++rank_simbol)
        std::cout << "  " << rank_simbol << " ";

    std::cout << "\n\nFEN: " << fen();
    std::cout << "\nHash: " << hash();
    std::cout << "\nEval: " << eval() << "\n";
}

bool Position::insufficient_material() const {
    int piece_amount = material_count();
    if (piece_amount == 2) {
        return true;
    } else if (piece_amount == 3 && (material_count(KNIGHT) == 1 || material_count(BISHOP) == 1)) {
        return true;
    } else if (piece_amount == 4 && (material_count(KNIGHT) == 2 ||
                                     (material_count(WHITE_BISHOP) == 1 && material_count(BLACK_BISHOP) == 1))) {
        return true;
    }

    return false;
}

bool Position::repetition() const {
    int counter = 0;
    int starting_index = m_history_top;
    size_t distance = std::min(fifty_move_ply(), ply_from_null());

    for (size_t index = 4; index <= distance; index += 2)
        if (m_history[starting_index - index] == hash()) {
            if (index < m_history_top) // 2-fold repetition within the search tree, this avoids cycles
                return true;

            counter++;

            if (counter >= 2) // 3-fold repetition
                return true;
        }
    return false;
}

bool Position::fifty_move_draw() {
    if (fifty_move_ply() >= 100) {
        ScoredMove moves[MAX_MOVES_PER_POS];
        ScoredMove *end = gen_moves(moves, *this, GEN_ALL);
        for (ScoredMove *begin = moves; begin != end; ++begin) {
            bool legal = is_legal(begin->move);
            if (legal)
                return true;
        }
    }

    return false;
}

void Position::hash_piece_key(const PieceSquare &ps) {
    assert(ps.piece >= WHITE_PAWN && ps.piece <= BLACK_KING);
    assert(ps.sq >= a1 && ps.sq <= h8);
    hash() ^= hash_keys.pieces[ps.piece][ps.sq];
}

void Position::hash_castle_key() {
    assert(castling_rights() >= 0 && castling_rights() <= ANY_CASTLING);
    hash() ^= hash_keys.castle[castling_rights()];
}

void Position::hash_ep_key() {
    assert(get_file(en_passant()) >= 0 && get_file(en_passant()) < 8);
    hash() ^= hash_keys.en_passant[get_file(en_passant())];
}

void Position::hash_side_key() { hash() ^= hash_keys.side; }
