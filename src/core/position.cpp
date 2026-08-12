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

#include "core/position.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

#include "core/attacks.h"
#include "core/bitboard.h"
#include "core/move.h"
#include "core/movegen.h"
#include "core/types.h"
#include "core/zobrist.h"
#include "eval/nnue.h"
#include "utils/utils.h"

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

            add_piece({get_piece(pt, player), sq});

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
            m_curr_state.castle_rooks.set_sq((piece_bb(WHITE_ROOK) & Bitboard::RANK_1).msb());
        } else if (castling == 'Q') {
            set_bits(m_curr_state.castling_rights, static_cast<uint8_t>(WHITE_OOO));
            m_curr_state.castle_rooks.set_sq((piece_bb(WHITE_ROOK) & Bitboard::RANK_1).lsb());
        } else if (castling == 'k') {
            set_bits(m_curr_state.castling_rights, static_cast<uint8_t>(BLACK_OO));
            m_curr_state.castle_rooks.set_sq((piece_bb(BLACK_ROOK) & Bitboard::RANK_8).msb());
        } else if (castling == 'q') {
            set_bits(m_curr_state.castling_rights, static_cast<uint8_t>(BLACK_OOO));
            m_curr_state.castle_rooks.set_sq((piece_bb(BLACK_ROOK) & Bitboard::RANK_8).lsb());
        } else if ('A' <= castling && castling <= 'H') {
            Square sq = get_square(castling - 'A', 0);
            set_bits(m_curr_state.castling_rights, static_cast<uint8_t>(king_sq(WHITE) > sq ? WHITE_OOO : WHITE_OO));
            m_curr_state.castle_rooks.set_sq(sq);
        } else if ('a' <= castling && castling <= 'h') {
            Square sq = get_square(castling - 'a', 7);
            set_bits(m_curr_state.castling_rights, static_cast<uint8_t>(king_sq(BLACK) > sq ? BLACK_OOO : BLACK_OO));
            m_curr_state.castle_rooks.set_sq(sq);
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
    calculate_aux_bbs();

    return true;
}

std::string Position::get_fen() const {
    std::string fen;
    for (int rank = 7; rank >= 0; --rank) {
        int counter = 0;
        for (int file = 0; file < 8; ++file) {
            const Piece &piece = piece_at(get_square(file, rank));
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
    if (ep_sq() == NO_SQ) {
        fen += "-";
    } else {
        fen += (get_file(ep_sq()) + 'a');
        fen += (m_stm == WHITE ? '6' : '3');
    }
    fen += ' ';

    fen += std::to_string(halfmove_clock());
    fen += ' ';
    fen += std::to_string(1 + (m_game_clock_ply - m_stm) / 2);

    return fen;
}

void Position::reset() {
    for (int sqi = a1; sqi <= h8; ++sqi)
        m_board[sqi] = EMPTY;
    for (size_t i = 0; i < 12; ++i) {
        m_pieces[i] = Bitboard::EMPTY;
    }
    for (size_t i = 0; i < 2; ++i) {
        m_occupancies[i] = Bitboard::EMPTY;
    }

    m_position_hash = 0ULL;
    m_pawn_hash = 0ULL;
    m_white_non_pawn_hash = 0ULL;
    m_black_non_pawn_hash = 0ULL;
    m_history_ply = 0;
    m_curr_state.reset();
}

void Position::add_piece(const PieceSquare &ps) {
    assert(ps.piece >= WHITE_PAWN && ps.piece <= BLACK_KING);
    assert(ps.sq >= a1 && ps.sq <= h8);

    Color color = get_color(ps.piece);
    m_occupancies[color].set_sq(ps.sq);
    m_pieces[ps.piece].set_sq(ps.sq);
    m_board[ps.sq] = ps.piece;

    hash_piece_key(ps);
}

void Position::remove_piece(const PieceSquare &ps) {
    assert(ps.piece >= WHITE_PAWN && ps.piece <= BLACK_KING);
    assert(ps.sq >= a1 && ps.sq <= h8);

    Color color = get_color(ps.piece);
    m_occupancies[color].unset_sq(ps.sq);
    m_pieces[ps.piece].unset_sq(ps.sq);
    m_board[ps.sq] = EMPTY;

    hash_piece_key(ps);
}

DirtyPiece Position::make_move(const Move &move) {
    m_played_positions[m_history_ply] = m_position_hash;
    m_history_stack[m_history_ply] = m_curr_state;
    ++m_history_ply;
    ++m_game_clock_ply;
    ++m_curr_state.fifty_move_ply;
    ++m_curr_state.ply_from_null;

    if (m_curr_state.en_passant != NO_SQ) {
        hash_ep_key();
        m_curr_state.en_passant = NO_SQ;
    }

    m_curr_state.captured = piece_at(move.to());

    const DirtyPiece dp = [&]() {
        if (move.is_regular())
            return make_regular(move);
        else if (move.is_capture() && !move.is_ep())
            return make_capture(move);
        else if (move.is_castle())
            return make_castle(move);
        else if (move.is_promotion())
            return make_promotion(move);
        else if (move.is_ep())
            return make_en_passant(move);
        else
            __builtin_unreachable();
    }();

    hash_castle_key();
    update_castling_rights(move);
    hash_castle_key();
    hash_side_key();

    change_side();
    calculate_aux_bbs();

    return dp;
}

DirtyPiece Position::make_regular(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    Piece piece = piece_at(from);

    DirtyPiece dp;
    dp.move_type = REGULAR;
    dp.sub0 = {piece, from};
    dp.add0 = {piece, to};

    remove_piece(dp.sub0);
    add_piece(dp.add0);

    if (get_piece_type(piece, m_stm) == PAWN) {
        m_curr_state.fifty_move_ply = 0;
        int pawn_offset = get_pawn_offset(m_stm);
        if (to - from == 2 * pawn_offset &&
            (pawn_attacks[m_stm][to - pawn_offset] &
             piece_bb(PAWN, nstm()))) { // Double push and there is a enemy pawn to en passant
            m_curr_state.en_passant = static_cast<Square>(to - pawn_offset);
            hash_ep_key();
        }
    }

    return dp;
}

DirtyPiece Position::make_capture(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    Piece piece = piece_at(from);

    m_curr_state.fifty_move_ply = 0;
    m_curr_state.captured = piece_at(to);
    assert(m_curr_state.captured != EMPTY && get_piece_type(m_curr_state.captured) != KING);

    DirtyPiece dp;
    dp.move_type = CAPTURE;
    dp.sub0 = {piece, from};
    dp.sub1 = {m_curr_state.captured, to};
    dp.add0 = {piece, to};

    if (move.is_promotion())
        dp.add0.piece = get_piece(move.promotee(), m_stm);

    remove_piece(dp.sub0);
    remove_piece(dp.sub1);
    add_piece(dp.add0);

    return dp;
}

DirtyPiece Position::make_castle(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    Piece king = piece_at(from);
    Piece rook = get_piece(ROOK, stm());

    Bitboard stm_castling_rooks = m_curr_state.castle_rooks & (stm() == WHITE ? Bitboard::RANK_1 : Bitboard::RANK_8);

    Square rook_from = [&]() {
        if (to == c1 || to == c8)
            return stm_castling_rooks.lsb();
        else
            return stm_castling_rooks.msb();
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

    remove_piece(dp.sub0);
    remove_piece(dp.sub1);
    add_piece(dp.add0);
    add_piece(dp.add1);

    return dp;
}

DirtyPiece Position::make_promotion(const Move &move) {
    const Square from = move.from();
    const Square to = move.to();

    DirtyPiece dp;
    dp.move_type = REGULAR;
    dp.sub0 = {piece_at(from), from};
    dp.add0 = {get_piece(move.promotee(), m_stm), to};

    remove_piece(dp.sub0);
    add_piece(dp.add0);

    return dp;
}

DirtyPiece Position::make_en_passant(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    Piece piece = piece_at(from);
    Square captured_square = static_cast<Square>(to - static_cast<int>(get_pawn_offset(m_stm)));
    Piece captured = piece_at(captured_square);

    m_curr_state.fifty_move_ply = 0;
    m_curr_state.captured = captured;

    DirtyPiece dp;
    dp.move_type = CAPTURE;
    dp.sub0 = {piece, from};
    dp.sub1 = {captured, captured_square};
    dp.add0 = {piece, to};

    remove_piece(dp.sub0);
    remove_piece(dp.sub1);
    add_piece(dp.add0);

    return dp;
}

void Position::update_castling_rights(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    PieceType moved_piece_type = get_piece_type(piece_at(to), m_stm); // Piece has already been moved

    if (moved_piece_type == KING) { // Moved king
        switch (m_stm) {
            case WHITE:
                unset_mask(m_curr_state.castling_rights, static_cast<uint8_t>(WHITE_CASTLING));
                m_curr_state.castle_rooks.unset_mask(Bitboard::RANK_1);
                break;
            case BLACK:
                unset_mask(m_curr_state.castling_rights, static_cast<uint8_t>(BLACK_CASTLING));
                m_curr_state.castle_rooks.unset_mask(Bitboard::RANK_8);
                break;
            default:
                __builtin_unreachable();
        }
    } else if (moved_piece_type == ROOK) { // Moved rook
        if (m_curr_state.castle_rooks.is_set(from)) {
            m_curr_state.castle_rooks.unset_sq(from);

            if (from > king_sq(stm())) {
                unset_mask(m_curr_state.castling_rights, static_cast<uint8_t>(stm() == WHITE ? WHITE_OO : BLACK_OO));
            } else {
                unset_mask(m_curr_state.castling_rights, static_cast<uint8_t>(stm() == WHITE ? WHITE_OOO : BLACK_OOO));
            }
        }
    }
    if (get_piece_type(m_curr_state.captured) == ROOK) { // Captured rook
        if (m_curr_state.castle_rooks.is_set(to)) {
            m_curr_state.castle_rooks.unset_sq(to);
            if (to > king_sq(nstm())) {
                unset_mask(m_curr_state.castling_rights, static_cast<uint8_t>(nstm() == WHITE ? WHITE_OO : BLACK_OO));
            } else {
                unset_mask(m_curr_state.castling_rights, static_cast<uint8_t>(nstm() == WHITE ? WHITE_OOO : BLACK_OOO));
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
    Piece piece = piece_at(to);

    if (move.is_regular()) {
        remove_piece({piece, to});
        add_piece({piece, from});
    } else if (move.is_capture() && !move.is_ep()) {
        remove_piece({piece, to});
        add_piece({m_curr_state.captured, to});
        if (move.is_promotion()) {
            piece = get_piece(PAWN, m_stm);
        }
        add_piece({piece, from});
    } else if (move.is_castle()) {
        remove_piece({piece, to});
        Bitboard rook_castle_bb =
            m_history_stack[m_history_ply - 1].castle_rooks & (stm() == WHITE ? Bitboard::RANK_1 : Bitboard::RANK_8);
        switch (to) {
            case g1: // White castle short
                remove_piece({get_piece(ROOK, stm()), f1});
                add_piece({WHITE_ROOK, rook_castle_bb.msb()});
                break;
            case c1: // White castle long
                remove_piece({get_piece(ROOK, stm()), d1});
                add_piece({WHITE_ROOK, rook_castle_bb.lsb()});
                break;
            case g8: // Black castle short
                remove_piece({get_piece(ROOK, stm()), f8});
                add_piece({BLACK_ROOK, rook_castle_bb.msb()});
                break;
            case c8: // Black castle long
                remove_piece({get_piece(ROOK, stm()), d8});
                add_piece({BLACK_ROOK, rook_castle_bb.lsb()});
                break;
            default:
                __builtin_unreachable();
        }
        add_piece({piece, from});
    } else if (move.is_promotion()) {
        remove_piece({piece, to});
        piece = get_piece(PAWN, m_stm);
        add_piece({piece, from});
    } else if (move.is_ep()) {
        remove_piece({piece, to});
        add_piece({piece, from});

        Square captured_square = static_cast<Square>(to - static_cast<int>(get_pawn_offset(m_stm)));
        add_piece({m_curr_state.captured, captured_square});
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
    m_played_positions[m_history_ply] = m_position_hash;
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
    calculate_aux_bbs();
}

void Position::unmake_null_move() {
    --m_history_ply;
    m_curr_state = m_history_stack[m_history_ply];
    m_position_hash = m_played_positions[m_history_ply];
    --m_game_clock_ply;
    change_side();
}

void Position::calculate_aux_bbs() {
    Color adversary = nstm();
    Square ksq = king_sq(m_stm);
    m_curr_state.pins = 0;
    m_curr_state.checkers = (pawn_attacks[m_stm][ksq] & piece_bb(PAWN, adversary)) // Pawns
                            | (knight_attacks[ksq] & piece_bb(KNIGHT, adversary)); // Knights;

    Bitboard slider_checkers =
        ((piece_bb(QUEEN, adversary) | piece_bb(BISHOP, adversary)) & get_bishop_attacks(ksq, 0)) |
        ((piece_bb(QUEEN, adversary) | piece_bb(ROOK, adversary)) & get_rook_attacks(ksq, 0));
    while (slider_checkers) {
        Square sq = slider_checkers.poplsb();

        Bitboard blockers = inbetween_masks[ksq][sq] & occ_bb();
        if (!blockers) {
            m_curr_state.checkers.set_sq(sq);
        } else if (blockers.popcount() == 1) {
            m_curr_state.pins.set_mask(blockers & occ_bb(m_stm));
        }
    }

    calculate_threats_bb();
}

void Position::calculate_threats_bb() {
    Bitboard &threats = m_curr_state.threats;
    threats = 0;

    const Color opp = nstm();
    const Bitboard occupancy_bb = occ_bb() ^ piece_bb(KING, stm());

    const Bitboard pawn_bb = piece_bb(PAWN, opp);
    threats |= pawn_bb.shift_up_east_pov(opp);
    threats |= pawn_bb.shift_up_west_pov(opp);

    Bitboard knights_bb = piece_bb(KNIGHT, opp);
    while (knights_bb) {
        const Square sq = knights_bb.poplsb();
        threats |= knight_attacks[sq];
    }

    Bitboard bishop_bb = piece_bb(BISHOP, opp) | piece_bb(QUEEN, opp);
    while (bishop_bb) {
        const Square sq = bishop_bb.poplsb();
        threats |= get_piece_attacks(sq, occupancy_bb, BISHOP);
    }

    Bitboard rook_bb = piece_bb(ROOK, opp) | piece_bb(QUEEN, opp);
    while (rook_bb) {
        const Square sq = rook_bb.poplsb();
        threats |= get_piece_attacks(sq, occupancy_bb, ROOK);
    }

    threats |= king_attacks[king_sq(opp)];
}

bool Position::is_attacked(const Square &sq) const {
    Color opponent = nstm();
    Bitboard occupancy = occ_bb();
    occupancy.unset_sq(sq); // square to be checked has to be unset on occupancy bitboard

    // Check if sq is attacked by opponent pawns. Note: pawn attack mask has to be "stm" because the logic is reversed
    if (piece_bb(PAWN, opponent) & pawn_attacks[m_stm][sq])
        return true;

    // Check if sq is attacked by opponent knights
    if (piece_bb(KNIGHT, opponent) & knight_attacks[sq])
        return true;

    // Check if sq is attacked by opponent bishops or queens
    if ((piece_bb(BISHOP, opponent) | piece_bb(QUEEN, opponent)) & get_bishop_attacks(sq, occupancy))
        return true;

    // Check if sq is attacked by opponent rooks or queens
    if ((piece_bb(ROOK, opponent) | piece_bb(QUEEN, opponent)) & get_rook_attacks(sq, occupancy))
        return true;

    // Check if sq is attacked by opponent king. Unnecessary when checking for checks
    if (piece_bb(KING, opponent) & king_attacks[sq])
        return true;

    return false;
}

Bitboard Position::attackers(const Square &sq) const {
    Bitboard attackers;
    Bitboard occupancy = occ_bb();

    attackers |= pawn_attacks[WHITE][sq] & piece_bb(PAWN, BLACK);
    attackers |= pawn_attacks[BLACK][sq] & piece_bb(PAWN, WHITE);
    attackers |= get_piece_attacks(sq, occupancy, KNIGHT) & piece_bb(KNIGHT);
    attackers |= get_piece_attacks(sq, occupancy, BISHOP) & (piece_bb(BISHOP) | piece_bb(QUEEN));
    attackers |= get_piece_attacks(sq, occupancy, ROOK) & (piece_bb(ROOK) | piece_bb(QUEEN));
    attackers |= get_piece_attacks(sq, occupancy, KING) & piece_bb(KING);

    return attackers;
}

bool Position::is_legal(const Move &move) {
    Square ksq = king_sq(m_stm);
    Square from = move.from();
    Square to = move.to();
    PieceType moved_pt = get_piece_type(piece_at(from));

    if (move.is_castle()) {
        Piece rook = get_piece(ROOK, stm());
        // TODO Unnecessary & get_piece_bb(rook)
        Bitboard stm_castling_rooks =
            m_curr_state.castle_rooks & piece_bb(rook) & Bitboard(stm() == WHITE ? Bitboard::RANK_1 : Bitboard::RANK_8);
        Square rook_from = stm_castling_rooks.msb();
        if (to == c1 || to == c8) {
            rook_from = stm_castling_rooks.lsb();
        }
        return !is_attacked(to) && !pins_bb().is_set(rook_from); // Other clauses were checked by movegen
    }
    if (move.is_ep()) {
        int pawn_offset = (m_stm == WHITE ? NORTH : SOUTH);
        Piece stm_pawn = get_piece(PAWN, m_stm);
        Piece ntm_pawn = get_piece(PAWN, nstm());
        remove_piece({stm_pawn, from});
        remove_piece({ntm_pawn, static_cast<Square>(to - pawn_offset)});
        add_piece({stm_pawn, to});
        bool is_king_attacked = is_attacked(ksq);
        add_piece({stm_pawn, from});
        add_piece({ntm_pawn, static_cast<Square>(to - pawn_offset)});
        remove_piece({stm_pawn, to});
        return !is_king_attacked;
    }
    if (moved_pt == KING) {
        remove_piece({get_piece(KING, m_stm), ksq});
        bool is_king_attacked = is_attacked(to);
        add_piece({get_piece(KING, m_stm), ksq});
        return !is_king_attacked;
    }

    if (checkers_bb().popcount() > 1) // Double check can only be evaded by king movements
        return false;

    if (pins_bb().is_set(from)) // if piece is pinned, it must keep blocking the check
        return !checkers_bb() && (inbetween_masks[ksq][to].is_set(from) || inbetween_masks[ksq][from].is_set(to));

    if (checkers_bb()) // If in check and not moving the king, it must either block the check or take the attacker
        return (checkers_bb() | inbetween_masks[checkers_bb().lsb()][ksq]).is_set(to);

    return true;
}

// TODO if the moved piece and/or the capture piece is present in the move itself this could be way faster
bool Position::is_pseudo_legal(const Move &move) const {
    if (!move)
        return false;

    Square from = move.from();
    Square to = move.to();
    Piece moved_piece = piece_at(from);
    PieceType moved_piece_type = get_piece_type(moved_piece, m_stm);

    // No piece in "from" square or piece is not stm
    if (moved_piece == EMPTY || get_color(moved_piece) != m_stm)
        return false;
    if (get_color(piece_at(to)) == m_stm) // stm piece on "to" square
        return false;
    if (move.is_capture() && !move.is_ep() && piece_at(to) == EMPTY)
        return false;
    if ((!move.is_capture() || move.is_ep()) && piece_at(to) != EMPTY)
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

    Bitboard moved_piece_attacks = get_piece_attacks(from, occ_bb(), moved_piece_type);
    return moved_piece_attacks.is_set(to);
}

bool Position::pawn_pseudo_legal(const Square &from, const Square &to, const Move &move) const {
    int pawn_offset = get_pawn_offset(m_stm);
    if (move.is_ep()) {
        if (m_curr_state.en_passant != to || !piece_bb(PAWN, nstm()).is_set(static_cast<Square>(to - pawn_offset)))
            return false;
    } else if (move.is_capture()) {
        if (!pawn_attacks[m_stm][from].is_set(to))
            return false;
    } else if (from + 2 * pawn_offset == to) {
        if (get_rank(from) != get_pawn_start_rank(m_stm) || piece_at(static_cast<Square>(from + pawn_offset)) != EMPTY)
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
    Bitboard short_castling_crossing_mask = Bitboard::WHITE_OO_CROSSING_MASK;
    Bitboard long_castling_crossing_mask = Bitboard::WHITE_OOO_CROSSING_MASK;
    if (m_stm == BLACK) {
        short_right = BLACK_OO;
        long_right = BLACK_OOO;
        short_castling_crossing_mask = Bitboard::BLACK_OO_CROSSING_MASK;
        long_castling_crossing_mask = Bitboard::BLACK_OOO_CROSSING_MASK;
    }

    if (castling_short && (!(castling_rights() & short_right) || (occ_bb() & short_castling_crossing_mask))) {
        return false;
    }
    if (castling_long && (!(castling_rights() & long_right) || (occ_bb() & long_castling_crossing_mask))) {
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
            if (m_curr_state.checkers.is_set(sq)) {
                color = "\033[31m";
            } else if (m_curr_state.pins.is_set(sq)) {
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
    std::cout << "\nHash: " << m_position_hash << "\n";
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
    int distance = std::min(m_curr_state.fifty_move_ply, m_curr_state.ply_from_null);
    int starting_index = m_history_ply;

    for (int index = 4; index <= distance; index += 2)
        if (m_played_positions[starting_index - index] == m_position_hash) {
            if (index < m_history_ply) // 2-fold repetition within the search tree, this avoids cycles
                return true;

            counter++;

            if (counter >= 2) // 3-fold repetition
                return true;
        }
    return false;
}

bool Position::is_fifty_move_draw() {
    if (m_curr_state.fifty_move_ply >= 100) {
        Movegen::ScoredMoveList move_list;
        Movegen::all(move_list, *this);
        return !move_list.empty(); // if there is at least one legal move, its not checkmate
    }

    return false;
}

void Position::hash_piece_key(const PieceSquare &ps) {
    assert(ps.piece >= WHITE_PAWN && ps.piece <= BLACK_KING);
    assert(ps.sq >= a1 && ps.sq <= h8);

    const HashType psq_key = Zobrist::piece_square_key(ps);
    m_position_hash ^= psq_key;
    if (ps.piece == WHITE_PAWN || ps.piece == BLACK_PAWN) {
        m_pawn_hash ^= psq_key;
    } else if (get_color(ps.piece) == WHITE) {
        m_white_non_pawn_hash ^= psq_key;
    } else {
        assert(get_color(ps.piece) == BLACK);
        m_black_non_pawn_hash ^= psq_key;
    }
}

void Position::hash_castle_key() {
    assert(m_curr_state.castling_rights >= 0 && m_curr_state.castling_rights <= ANY_CASTLING);

    m_position_hash ^= Zobrist::castle_key(m_curr_state.castling_rights);
}

void Position::hash_ep_key() {
    assert(get_file(m_curr_state.en_passant) >= 0 && get_file(m_curr_state.en_passant) < 8);

    const HashType ep_key = Zobrist::ep_key(get_file(m_curr_state.en_passant));
    m_position_hash ^= ep_key;
    m_pawn_hash ^= ep_key;
}

void Position::hash_side_key() { m_position_hash ^= Zobrist::color_key(); }
