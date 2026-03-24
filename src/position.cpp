/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
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
#include "eval/accumulator.h"
#include "hash.h"
#include "move.h"
#include "movegen.h"
#include "types.h"
#include "utils.h"

Position::Position() { set_fen(START_FEN); }

bool Position::set_fen(const std::string &fen) {
    reset();

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
            if (piece_char == 'p')
                add_piece({sq, get_piece(PAWN, player)});
            else if (piece_char == 'n')
                add_piece({sq, get_piece(KNIGHT, player)});
            else if (piece_char == 'b')
                add_piece({sq, get_piece(BISHOP, player)});
            else if (piece_char == 'r')
                add_piece({sq, get_piece(ROOK, player)});
            else if (piece_char == 'q')
                add_piece({sq, get_piece(QUEEN, player)});
            else if (piece_char == 'k')
                add_piece({sq, get_piece(KING, player)});
            else
                assert(false);

            ++file;
        } else {
            file += c - '0';
        }
    }

    if (fen_arguments[1] == "w" || fen_arguments[1] == "W") {
        m_stm = WHITE;
    } else if (fen_arguments[1] == "b" || fen_arguments[1] == "B") {
        m_stm = BLACK;
        hash_side_key();
    } else {
        std::cerr << "INVALID FEN: invalid player, it should be 'w' or 'b'." << std::endl;
        return false;
    }

    for (char castling : fen_arguments[2]) {
        if (castling == 'K') {
            set_bits(m_curr_state.castling_rights, static_cast<uint8_t>(WHITE_OO));
            set_bit(m_curr_state.castle_rooks, msb(get_piece_bb(WHITE_ROOK) & RANK_MASKS[0]));
        } else if (castling == 'Q') {
            set_bits(m_curr_state.castling_rights, static_cast<uint8_t>(WHITE_OOO));
            set_bit(m_curr_state.castle_rooks, lsb(get_piece_bb(WHITE_ROOK) & RANK_MASKS[0]));
        } else if (castling == 'k') {
            set_bits(m_curr_state.castling_rights, static_cast<uint8_t>(BLACK_OO));
            set_bit(m_curr_state.castle_rooks, msb(get_piece_bb(BLACK_ROOK) & RANK_MASKS[7]));
        } else if (castling == 'q') {
            set_bits(m_curr_state.castling_rights, static_cast<uint8_t>(BLACK_OOO));
            set_bit(m_curr_state.castle_rooks, lsb(get_piece_bb(BLACK_ROOK) & RANK_MASKS[7]));
        } else if ('A' <= castling && castling <= 'H') {
            Square sq = get_square(castling - 'A', 0);
            set_bits(m_curr_state.castling_rights,
                     static_cast<uint8_t>(get_king_placement(WHITE) > sq ? WHITE_OOO : WHITE_OO));
            set_bit(m_curr_state.castle_rooks, sq);
        } else if ('a' <= castling && castling <= 'h') {
            Square sq = get_square(castling - 'a', 7);
            set_bits(m_curr_state.castling_rights,
                     static_cast<uint8_t>(get_king_placement(BLACK) > sq ? BLACK_OOO : BLACK_OO));
            set_bit(m_curr_state.castle_rooks, sq);
        }
    }
    hash_castle_key();

    if (fen_arguments[3] == "-") {
        m_curr_state.en_passant = NO_SQ;
    } else {
        m_curr_state.en_passant = get_square(fen_arguments[3][0] - 'a', fen_arguments[3][1] - '1');
        hash_ep_key();
    }

    try {
        m_curr_state.fifty_move_ply = std::stoi(fen_arguments[4]);
    } catch (const std::exception &) {
        std::cerr << "INVALID FEN: halfmove clock is not a number." << std::endl;
        return false;
    }
    try {
        m_game_clock_ply = (std::stoi(fen_arguments[5]) - 1) * 2 + m_stm;
    } catch (const std::exception &) {
        std::cerr << "INVALID FEN: game clock is not a number." << std::endl;
        return false;
    }
    update_pin_and_checkers_bb();

    return true;
}

std::string Position::get_fen() const {
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
    fen += (m_stm == WHITE ? " w " : " b ");
    bool none = true;
    if (m_curr_state.castling_rights & WHITE_OO) {
        none = false;
        fen += "K";
    }
    if (m_curr_state.castling_rights & WHITE_OOO) {
        none = false;
        fen += "Q";
    }
    if (m_curr_state.castling_rights & BLACK_OO) {
        none = false;
        fen += "k";
    }
    if (m_curr_state.castling_rights & BLACK_OOO) {
        none = false;
        fen += "q";
    }
    if (none)
        fen += "-";

    fen += ' ';
    if (get_en_passant() == NO_SQ) {
        fen += "-";
    } else {
        fen += (get_file(get_en_passant()) + 'a');
        fen += (m_stm == WHITE ? '6' : '3');
    }
    fen += ' ';

    fen += std::to_string(get_fifty_move_ply());
    fen += ' ';
    fen += std::to_string(1 + (m_game_clock_ply - m_stm) / 2);

    return fen;
}

void Position::reset() {
    for (int sqi = a1; sqi <= h8; ++sqi)
        m_board[sqi] = EMPTY;
    std::memset(m_occupancies, 0ULL, sizeof(m_occupancies));
    std::memset(m_pieces, 0ULL, sizeof(m_pieces));

    m_hash_key = 0ULL;
    m_history_ply = 0;
    m_curr_state.reset();
}

void Position::add_piece(SquarePiece sp) {
    assert(sp.piece >= WHITE_PAWN && sp.piece <= BLACK_KING);
    assert(sp.sq >= a1 && sp.sq <= h8);

    Color color = get_color(sp.piece);
    set_bit(m_occupancies[color], sp.sq);
    set_bit(m_pieces[sp.piece], sp.sq);
    m_board[sp.sq] = sp.piece;

    hash_piece_key(sp.piece, sp.sq);
}

void Position::remove_piece(SquarePiece sp) {
    assert(sp.piece >= WHITE_PAWN && sp.piece <= BLACK_KING);
    assert(sp.sq >= a1 && sp.sq <= h8);

    Color color = get_color(sp.piece);
    unset_bit(m_occupancies[color], sp.sq);
    unset_bit(m_pieces[sp.piece], sp.sq);
    m_board[sp.sq] = EMPTY;

    hash_piece_key(sp.piece, sp.sq);
}

void Position::move_piece(SquarePiece add, SquarePiece sub) {
    remove_piece(sub);
    add_piece(add);
}

void Position::move_piece(const Piece &piece, const Square &from, const Square &to) {
    remove_piece({from, piece});
    add_piece({to, piece});
}

bool Position::make_move(const Move &move, DirtyPiece &dp) {
    m_played_positions[m_history_ply] = m_hash_key;
    m_history_stack[m_history_ply] = m_curr_state;
    ++m_history_ply;
    ++m_game_clock_ply;
    ++m_curr_state.fifty_move_ply;
    ++m_curr_state.ply_from_null;

    if (m_curr_state.en_passant != NO_SQ) {
        hash_ep_key();
        m_curr_state.en_passant = NO_SQ;
    }

    m_curr_state.captured = consult(move.to());

    if (move.is_regular())
        make_regular(move, dp);
    else if (move.is_capture() && !move.is_ep())
        make_capture(move, dp);
    else if (move.is_castle())
        make_castle(move, dp);
    else if (move.is_promotion())
        make_promotion(move, dp);
    else if (move.is_ep())
        make_en_passant(move, dp);

    hash_castle_key();
    update_castling_rights(move);
    hash_castle_key();
    hash_side_key();

    bool legal = !is_attacked(get_king_placement(m_stm));

    change_side();
    update_pin_and_checkers_bb();
    return legal;
}

void Position::make_regular(const Move &move, DirtyPiece &dp) {
    dp.move_type = REGULAR;
    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(from);

    dp.add0 = SquarePiece(to, piece);
    dp.sub0 = SquarePiece(from, piece);

    move_piece(dp.add0, dp.sub0);

    if (get_piece_type(piece, m_stm) == PAWN) {
        m_curr_state.fifty_move_ply = 0;
        int pawn_offset = get_pawn_offset(m_stm);
        if (to - from == 2 * pawn_offset &&
            (pawn_attacks[m_stm][to - pawn_offset] &
             get_piece_bb(PAWN, get_adversary()))) { // Double push and there is a enemy pawn to en passant
            m_curr_state.en_passant = static_cast<Square>(to - pawn_offset);
            hash_ep_key();
        }
    }
}

void Position::make_capture(const Move &move, DirtyPiece &dp) {
    dp.move_type = CAPTURE;
    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(from);

    m_curr_state.fifty_move_ply = 0;
    m_curr_state.captured = consult(to);
    assert(m_curr_state.captured != EMPTY && get_piece_type(m_curr_state.captured) != KING);

    dp.sub0 = SquarePiece(from, piece);
    dp.sub1 = SquarePiece(to, m_curr_state.captured);

    remove_piece(dp.sub0);
    remove_piece(dp.sub1);

    if (move.is_promotion())
        piece = get_piece(move.promotee(), m_stm);

    dp.add0 = SquarePiece(to, piece);
    add_piece(dp.add0);
}

void Position::make_castle(const Move &move, DirtyPiece &dp) {
    dp.move_type = CASTLING;
    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(from);
    Piece rook = get_piece(ROOK, get_stm());

    Bitboard stm_castling_rooks =
        m_curr_state.castle_rooks & get_piece_bb(rook) & RANK_MASKS[get_stm() == WHITE ? 0 : 7];
    Square rook_from = msb(stm_castling_rooks);
    if (to == c1 || to == c8) {
        rook_from = lsb(stm_castling_rooks);
    }

    Square rook_to;
    switch (to) {
        case g1: // White castle short
            rook_to = f1;
            break;
        case c1: // White castle long
            rook_to = d1;
            break;
        case g8: // Black castle short
            rook_to = f8;
            break;
        case c8: // Black castle long
            rook_to = d8;
            break;
        default:
            __builtin_unreachable();
    }

    dp.add0 = SquarePiece(to, piece);
    dp.sub0 = SquarePiece(from, piece);
    dp.add1 = SquarePiece(rook_to, rook);
    dp.sub1 = SquarePiece(rook_from, rook);

    move_piece(dp.add0, dp.sub0);
    move_piece(dp.add1, dp.sub1);
}

void Position::make_promotion(const Move &move, DirtyPiece &dp) {
    dp.move_type = REGULAR;
    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(from);

    dp.sub0 = SquarePiece(from, piece);
    dp.add0 = SquarePiece(to, get_piece(move.promotee(), m_stm));
    move_piece(dp.add0, dp.sub0);
}

void Position::make_en_passant(const Move &move, DirtyPiece &dp) {
    dp.move_type = CAPTURE;
    m_curr_state.fifty_move_ply = 0;
    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(from);
    Square captured_square = static_cast<Square>(to - static_cast<int>(get_pawn_offset(m_stm)));
    Piece captured = consult(captured_square);
    m_curr_state.captured = captured;

    dp.add0 = SquarePiece(to, piece);
    dp.sub0 = SquarePiece(from, piece);
    dp.sub1 = SquarePiece(captured_square, captured);
    remove_piece(dp.sub1);
    move_piece(dp.add0, dp.sub0);
}

void Position::update_castling_rights(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    PieceType moved_piece_type = get_piece_type(consult(to), m_stm); // Piece has already been moved

    if (moved_piece_type == KING) { // Moved king
        switch (m_stm) {
            case WHITE:
                unset_mask(m_curr_state.castling_rights, static_cast<uint8_t>(WHITE_CASTLING));
                unset_mask(m_curr_state.castle_rooks, RANK_MASKS[0]);
                break;
            case BLACK:
                unset_mask(m_curr_state.castling_rights, static_cast<uint8_t>(BLACK_CASTLING));
                unset_mask(m_curr_state.castle_rooks, RANK_MASKS[7]);
                break;
            default:
                __builtin_unreachable();
        }
    } else if (moved_piece_type == ROOK) { // Moved rook
        Bitboard from_bb = 1ULL << from;
        if (from_bb & m_curr_state.castle_rooks) {
            unset_mask(m_curr_state.castle_rooks, from_bb);

            if (from > get_king_placement(get_stm())) {
                unset_mask(m_curr_state.castling_rights,
                           static_cast<uint8_t>(get_stm() == WHITE ? WHITE_OO : BLACK_OO));
            } else {
                unset_mask(m_curr_state.castling_rights,
                           static_cast<uint8_t>(get_stm() == WHITE ? WHITE_OOO : BLACK_OOO));
            }
        }
    }
    if (get_piece_type(m_curr_state.captured) == ROOK) { // Captured rook
        Bitboard to_bb = 1ULL << to;
        if (to_bb & m_curr_state.castle_rooks) {
            unset_mask(m_curr_state.castle_rooks, to_bb);
            if (to > get_king_placement(get_adversary())) {
                unset_mask(m_curr_state.castling_rights,
                           static_cast<uint8_t>(get_adversary() == WHITE ? WHITE_OO : BLACK_OO));
            } else {
                unset_mask(m_curr_state.castling_rights,
                           static_cast<uint8_t>(get_adversary() == WHITE ? WHITE_OOO : BLACK_OOO));
            }
        }
    }
}

void Position::unmake_move(const Move &move) {
    assert(m_history_ply > 0); // check if there is a move to unmake

    --m_game_clock_ply;

    change_side();

    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(to);

    if (move.is_regular()) {
        move_piece(piece, to, from);
    } else if (move.is_capture() && !move.is_ep()) {
        remove_piece({to, piece});
        add_piece({to, m_curr_state.captured});
        if (move.is_promotion()) {
            piece = get_piece(PAWN, m_stm);
        }
        add_piece({from, piece});
    } else if (move.is_castle()) {
        remove_piece({to, piece});
        Bitboard rook_castle_bb =
            m_history_stack[m_history_ply - 1].castle_rooks & RANK_MASKS[get_stm() == WHITE ? 0 : 7];
        switch (to) {
            case g1: // White castle short
                remove_piece({f1, get_piece(ROOK, get_stm())});
                add_piece({msb(rook_castle_bb), WHITE_ROOK});
                break;
            case c1: // White castle long
                remove_piece({d1, get_piece(ROOK, get_stm())});
                add_piece({lsb(rook_castle_bb), WHITE_ROOK});
                break;
            case g8: // Black castle short
                remove_piece({f8, get_piece(ROOK, get_stm())});
                add_piece({msb(rook_castle_bb), BLACK_ROOK});
                break;
            case c8: // Black castle long
                remove_piece({d8, get_piece(ROOK, get_stm())});
                add_piece({lsb(rook_castle_bb), BLACK_ROOK});
                break;
            default:
                __builtin_unreachable();
        }
        add_piece({from, piece});
    } else if (move.is_promotion()) {
        remove_piece({to, piece});
        piece = get_piece(PAWN, m_stm);
        add_piece({from, piece});
    } else if (move.is_ep()) {
        move_piece(piece, to, from);
        Square captured_square = static_cast<Square>(to - static_cast<int>(get_pawn_offset(m_stm)));
        add_piece({captured_square, m_curr_state.captured});
    }

    if (m_curr_state.en_passant != NO_SQ)
        hash_ep_key();
    hash_castle_key();

    m_curr_state = m_history_stack[--m_history_ply];

    if (m_curr_state.en_passant != NO_SQ)
        hash_ep_key();
    hash_castle_key();
    hash_side_key();
}

void Position::make_null_move() {
    m_history_stack[m_history_ply] = m_curr_state;
    m_played_positions[m_history_ply] = m_hash_key;
    ++m_history_ply;

    m_curr_state.ply_from_null = 0;
    m_curr_state.captured = EMPTY;
    ++m_curr_state.fifty_move_ply;
    ++m_game_clock_ply;
    if (m_curr_state.en_passant != NO_SQ) {
        hash_ep_key();
        m_curr_state.en_passant = NO_SQ;
    }
    hash_side_key();
    change_side();
    update_pin_and_checkers_bb();
}

void Position::unmake_null_move() {
    --m_history_ply;
    m_curr_state = m_history_stack[m_history_ply];
    m_hash_key = m_played_positions[m_history_ply];
    --m_game_clock_ply;
    change_side();
}

void Position::update_pin_and_checkers_bb() {
    Color adversary = get_adversary();
    Square king_sq = get_king_placement(m_stm);
    m_curr_state.pins = 0;
    m_curr_state.checkers = (pawn_attacks[m_stm][king_sq] & get_piece_bb(PAWN, adversary)) // Pawns
                            | (knight_attacks[king_sq] & get_piece_bb(KNIGHT, adversary)); // Knights;

    Bitboard slider_checkers =
        ((get_piece_bb(QUEEN, adversary) | get_piece_bb(BISHOP, adversary)) & get_bishop_attacks(king_sq, 0)) |
        ((get_piece_bb(QUEEN, adversary) | get_piece_bb(ROOK, adversary)) & get_rook_attacks(king_sq, 0));
    while (slider_checkers) {
        Square sq = poplsb(slider_checkers);

        Bitboard blockers = between_squares[king_sq][sq] & get_occupancy();
        if (!blockers) {
            set_bit(m_curr_state.checkers, sq);
        } else if (count_bits(blockers) == 1) {
            set_bits(m_curr_state.pins, blockers & get_occupancy(m_stm));
        }
    }
}

bool Position::is_attacked(const Square &sq) const {
    Color opponent = get_adversary();
    Bitboard occupancy = get_occupancy();
    unset_bit(occupancy, sq); // square to be checked has to be unset on occupancy bitboard

    // Check if sq is attacked by opponent pawns. Note: pawn attack mask has to be "stm" because the logic is reversed
    if (get_piece_bb(PAWN, opponent) & pawn_attacks[m_stm][sq])
        return true;

    // Check if sq is attacked by opponent knights
    if (get_piece_bb(KNIGHT, opponent) & knight_attacks[sq])
        return true;

    // Check if sq is attacked by opponent bishops or queens
    if ((get_piece_bb(BISHOP, opponent) | get_piece_bb(QUEEN, opponent)) & get_bishop_attacks(sq, occupancy))
        return true;

    // Check if sq is attacked by opponent rooks or queens
    if ((get_piece_bb(ROOK, opponent) | get_piece_bb(QUEEN, opponent)) & get_rook_attacks(sq, occupancy))
        return true;

    // Check if sq is attacked by opponent king. Unnecessary when checking for checks
    if (get_piece_bb(KING, opponent) & king_attacks[sq])
        return true;

    return false;
}

Bitboard Position::attackers(const Square &sq) const {
    Bitboard attackers = 0ULL;
    Bitboard occupancy = get_occupancy();

    attackers |= pawn_attacks[WHITE][sq] & get_piece_bb(PAWN, BLACK);
    attackers |= pawn_attacks[BLACK][sq] & get_piece_bb(PAWN, WHITE);
    attackers |= get_piece_attacks(sq, occupancy, KNIGHT) & get_piece_bb(KNIGHT);
    attackers |= get_piece_attacks(sq, occupancy, BISHOP) & (get_piece_bb(BISHOP) | get_piece_bb(QUEEN));
    attackers |= get_piece_attacks(sq, occupancy, ROOK) & (get_piece_bb(ROOK) | get_piece_bb(QUEEN));
    attackers |= get_piece_attacks(sq, occupancy, KING) & get_piece_bb(KING);

    return attackers;
}

int Position::legal_move_amount() {
    ScoredMove moves[MAX_MOVES_PER_POS];
    ScoredMove *end = gen_moves(moves, *this, GEN_ALL);
    int legal_amount = 0;
    DirtyPiece dp;
    for (ScoredMove *begin = moves; begin != end; ++begin) {
        if (make_move(begin->move, dp))
            ++legal_amount;
        unmake_move(begin->move);
    }
    return legal_amount;
}

bool Position::no_legal_moves() {
    ScoredMove moves[MAX_MOVES_PER_POS];
    ScoredMove *end = gen_moves(moves, *this, GEN_ALL);
    DirtyPiece dp;
    for (ScoredMove *curr = moves; curr != end; ++curr) {
        bool legal = make_move(curr->move, dp);
        unmake_move(curr->move);

        if (legal)
            return false;
    }
    return true;
}

bool Position::is_legal(const Move &move) {
    Square king_sq = get_king_placement(m_stm);
    Square from = move.from();
    Square to = move.to();
    PieceType moved_pt = get_piece_type(consult(from));

    if (move.is_castle()) {
        Piece rook = get_piece(ROOK, get_stm());
        Bitboard stm_castling_rooks =
            m_curr_state.castle_rooks & get_piece_bb(rook) & RANK_MASKS[get_stm() == WHITE ? 0 : 7];
        Square rook_from = msb(stm_castling_rooks);
        if (to == c1 || to == c8) {
            rook_from = lsb(stm_castling_rooks);
        }
        return !is_attacked(to) && !(get_pins() & (1ULL << rook_from)); // Other clauses were checked by movegen
    }
    if (move.is_ep()) {
        int pawn_offset = (m_stm == WHITE ? NORTH : SOUTH);
        Piece stm_pawn = get_piece(PAWN, m_stm);
        Piece ntm_pawn = get_piece(PAWN, get_adversary());
        remove_piece({from, stm_pawn});
        remove_piece({static_cast<Square>(to - pawn_offset), ntm_pawn});
        add_piece({to, stm_pawn});
        bool is_king_attacked = is_attacked(king_sq);
        add_piece({from, stm_pawn});
        add_piece({static_cast<Square>(to - pawn_offset), ntm_pawn});
        remove_piece({to, stm_pawn});
        return !is_king_attacked;
    }
    if (moved_pt == KING) {
        remove_piece({king_sq, get_piece(KING, m_stm)});
        bool is_king_attacked = is_attacked(to);
        add_piece({king_sq, get_piece(KING, m_stm)});
        return !is_king_attacked;
    }

    if (count_bits(get_checkers()) > 1) // Double check can only be evaded by king movements
        return false;

    if (get_pins() & (1ULL << from)) // if piece is pinned, it must keep blocking the check
        return !get_checkers() &&
               (((1ULL << from) & between_squares[king_sq][to]) || ((1ULL << to) & between_squares[king_sq][from]));

    if (get_checkers()) // If in check and not moving the king, it must either block the check or take the attacker
        return (1ULL << to) & (get_checkers() | between_squares[lsb(get_checkers())][king_sq]);

    return true;
}

// TODO if the moved piece and/or the capture piece is present in the move itself this could be way faster
bool Position::is_pseudo_legal(const Move &move) const {
    if (move == MOVE_NONE)
        return false;

    Square from = move.from();
    Square to = move.to();
    Piece moved_piece = consult(from);
    PieceType moved_piece_type = get_piece_type(moved_piece, m_stm);

    // No piece in "from" square or piece is not stm
    if (moved_piece == EMPTY || get_color(moved_piece) != m_stm)
        return false;
    if (get_color(consult(to)) == m_stm) // stm piece on "to" square
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

    Bitboard moved_piece_attacks = get_piece_attacks(from, get_occupancy(), moved_piece_type);
    return moved_piece_attacks & (1ULL << to);
}

bool Position::pawn_pseudo_legal(const Square &from, const Square &to, const Move &move) const {
    int pawn_offset = get_pawn_offset(m_stm);
    if (move.is_ep()) {
        if (m_curr_state.en_passant != to || !(get_piece_bb(PAWN, get_adversary()) & (1ULL << (to - pawn_offset))))
            return false;
    } else if (move.is_capture()) {
        if (!(pawn_attacks[m_stm][from] & (1ULL << to)))
            return false;
    } else if (from + 2 * pawn_offset == to) {
        if (get_rank(from) != get_pawn_start_rank(m_stm) || consult(static_cast<Square>(from + pawn_offset)) != EMPTY)
            return false;
    } else if (from + pawn_offset != to) {
        return false;
    } else if (move.is_promotion()) {
        int from_rank = get_rank(from);
        int to_rank = get_rank(to);

        if (m_stm == WHITE && (from_rank != 6 || to_rank != 7))
            return false;
        if (m_stm == BLACK && (from_rank != 1 || to_rank != 0))
            return false;
    } else if ((m_stm == WHITE && get_rank(to) == 7) ||
               (m_stm == BLACK && get_rank(to) == 0)) { // No promotion flag in promotion rank
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
    if (m_stm == BLACK) {
        short_right = BLACK_OO;
        long_right = BLACK_OOO;
        short_castling_crossing_mask = BLACK_OO_CROSSING_MASK;
        long_castling_crossing_mask = BLACK_OOO_CROSSING_MASK;
    }

    if (castling_short &&
        (!(get_castling_rights() & short_right) || (get_occupancy() & short_castling_crossing_mask))) {
        return false;
    }
    if (castling_long && (!(get_castling_rights() & long_right) || (get_occupancy() & long_castling_crossing_mask))) {
        return false;
    }

    return true;
}

void Position::print() const {
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
            Piece piece = m_board[sq];
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
            if (m_curr_state.checkers & (1ULL << sq)) {
                color = "\033[31m";
            } else if (m_curr_state.pins & (1ULL << sq)) {
                color = "\033[34m";
            }

            std::cout << "| " << color << piece_char << (color != "" ? "\033[0m" : "") << " ";
        }
        std::cout << "| " << rank + 1 << "\n";
    }

    print_line();
    for (char rank_simbol = 'a'; rank_simbol <= 'h'; ++rank_simbol)
        std::cout << "  " << rank_simbol << " ";

    std::cout << "\n\nFEN: " << get_fen();
    std::cout << "\nHash: " << m_hash_key;
}

bool Position::insufficient_material() const {
    int piece_amount = get_material_count();
    if (piece_amount == 2) {
        return true;
    } else if (piece_amount == 3 && (get_material_count(KNIGHT) == 1 || get_material_count(BISHOP) == 1)) {
        return true;
    } else if (piece_amount == 4 && (get_material_count(KNIGHT) == 2 || (get_material_count(WHITE_BISHOP) == 1 &&
                                                                         get_material_count(BLACK_BISHOP) == 1))) {
        return true;
    }

    return false;
}

bool Position::repetition() const {
    int counter = 0;
    int distance = std::min(m_curr_state.fifty_move_ply, m_curr_state.ply_from_null);
    int starting_index = m_history_ply;

    for (int index = 4; index <= distance; index += 2)
        if (m_played_positions[starting_index - index] == m_hash_key) {
            if (index < m_history_ply) // 2-fold repetition within the search tree, this avoids cycles
                return true;

            counter++;

            if (counter >= 2) // 3-fold repetition
                return true;
        }
    return false;
}

bool Position::fifty_move_draw() {
    if (m_curr_state.fifty_move_ply >= 100) {
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

void Position::hash_piece_key(const Piece &piece, const Square &sq) {
    assert(piece >= WHITE_PAWN && piece <= BLACK_KING);
    assert(sq >= a1 && sq <= h8);
    m_hash_key ^= hash_keys.pieces[piece][sq];
}

void Position::hash_castle_key() {
    assert(m_curr_state.castling_rights >= 0 && m_curr_state.castling_rights <= ANY_CASTLING);
    m_hash_key ^= hash_keys.castle[m_curr_state.castling_rights];
}

void Position::hash_ep_key() {
    assert(get_file(m_curr_state.en_passant) >= 0 && get_file(m_curr_state.en_passant) < 8);
    m_hash_key ^= hash_keys.en_passant[get_file(m_curr_state.en_passant)];
}

void Position::hash_side_key() { m_hash_key ^= hash_keys.side; }
