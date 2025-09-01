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
#include <iostream>

#include "attacks.h"
#include "hash.h"
#include "move.h"
#include "movegen.h"
#include "types.h"
#include "utils.h"

Position::Position() { set_fen<true>(START_FEN); }

template <bool UPDATE>
bool Position::set_fen(const std::string &fen) {
    reset<UPDATE>();

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
                add_piece<UPDATE>(get_piece(PAWN, player), sq);
            else if (piece_char == 'n')
                add_piece<UPDATE>(get_piece(KNIGHT, player), sq);
            else if (piece_char == 'b')
                add_piece<UPDATE>(get_piece(BISHOP, player), sq);
            else if (piece_char == 'r')
                add_piece<UPDATE>(get_piece(ROOK, player), sq);
            else if (piece_char == 'q')
                add_piece<UPDATE>(get_piece(QUEEN, player), sq);
            else if (piece_char == 'k')
                add_piece<UPDATE>(get_piece(KING, player), sq);
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
        if (castling == 'K')
            set_bits(m_curr_state.castling_rights, static_cast<uint8_t>(WHITE_OO));
        else if (castling == 'Q')
            set_bits(m_curr_state.castling_rights, static_cast<uint8_t>(WHITE_OOO));
        else if (castling == 'k')
            set_bits(m_curr_state.castling_rights, static_cast<uint8_t>(BLACK_OO));
        else if (castling == 'q')
            set_bits(m_curr_state.castling_rights, static_cast<uint8_t>(BLACK_OOO));
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

    return true;
}

template bool Position::set_fen<true>(const std::string &fen);
template bool Position::set_fen<false>(const std::string &fen);

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

void Position::reset_nnue() { m_nnue.reset(*this); }

template <bool UPDATE>
void Position::reset() {
    for (int sqi = a1; sqi <= h8; ++sqi)
        m_board[sqi] = EMPTY;
    std::memset(m_occupancies, 0ULL, sizeof(m_occupancies));
    std::memset(m_pieces, 0ULL, sizeof(m_pieces));

    m_hash_key = 0ULL;
    m_history_ply = 0;
    m_curr_state.reset();

    if constexpr (UPDATE)
        reset_nnue();
}

template <bool UPDATE>
void Position::add_piece(const Piece &piece, const Square &sq) {
    assert(piece >= WHITE_PAWN && piece <= BLACK_KING);
    assert(sq >= a1 && sq <= h8);

    Color color = get_color(piece);
    set_bit(m_occupancies[color], sq);
    set_bit(m_pieces[piece], sq);
    m_board[sq] = piece;

    hash_piece_key(piece, sq);

    if constexpr (UPDATE)
        m_nnue.add_feature(piece, sq);
}

template <bool UPDATE>
void Position::remove_piece(const Piece &piece, const Square &sq) {
    assert(piece >= WHITE_PAWN && piece <= BLACK_KING);
    assert(sq >= a1 && sq <= h8);

    Color color = get_color(piece);
    unset_bit(m_occupancies[color], sq);
    unset_bit(m_pieces[piece], sq);
    m_board[sq] = EMPTY;

    hash_piece_key(piece, sq);

    if constexpr (UPDATE)
        m_nnue.remove_feature(piece, sq);
}

template <bool UPDATE>
void Position::move_piece(const Piece &piece, const Square &from, const Square &to) {
    remove_piece<UPDATE>(piece, from);
    add_piece<UPDATE>(piece, to);
}

template <bool UPDATE>
bool Position::make_move(const Move &move) {
    if constexpr (UPDATE)
        m_nnue.push();

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

    bool legal = true;
    if (move.is_regular())
        make_regular<UPDATE>(move);
    else if (move.is_capture() && !move.is_ep())
        make_capture<UPDATE>(move);
    else if (move.is_castle())
        legal = make_castle<UPDATE>(move);
    else if (move.is_promotion())
        make_promotion<UPDATE>(move);
    else if (move.is_ep())
        make_en_passant<UPDATE>(move);

    hash_castle_key();
    update_castling_rights(move);
    hash_castle_key();
    hash_side_key();

    if (!move.is_castle()) // If move is a castle, the legality has already been checked by make_castle()
        legal = !in_check();

    change_side();
    return legal;
}

template bool Position::make_move<true>(const Move &move);
template bool Position::make_move<false>(const Move &move);

template <bool UPDATE>
void Position::make_regular(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(from);

    move_piece<UPDATE>(piece, from, to);
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

template <bool UPDATE>
void Position::make_capture(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(from);

    m_curr_state.fifty_move_ply = 0;
    m_curr_state.captured = consult(to);
    assert(m_curr_state.captured != EMPTY);

    remove_piece<UPDATE>(m_curr_state.captured, to);
    remove_piece<UPDATE>(piece, from);

    if (move.is_promotion())
        piece = get_piece(move.promotee(), m_stm);

    add_piece<UPDATE>(piece, to);
}

template <bool UPDATE>
bool Position::make_castle(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(from);
    move_piece<UPDATE>(piece, from, to);
    switch (to) {
        case g1: // White castle short
            move_piece<UPDATE>(WHITE_ROOK, h1, f1);
            return !(is_attacked(e1) || is_attacked(f1) || is_attacked(g1));
        case c1: // White castle long
            move_piece<UPDATE>(WHITE_ROOK, a1, d1);
            return !(is_attacked(e1) || is_attacked(d1) || is_attacked(c1));
        case g8: // Black castle short
            move_piece<UPDATE>(BLACK_ROOK, h8, f8);
            return !(is_attacked(e8) || is_attacked(f8) || is_attacked(g8));
        case c8: // Black castle long
            move_piece<UPDATE>(BLACK_ROOK, a8, d8);
            return !(is_attacked(e8) || is_attacked(d8) || is_attacked(c8));
        default:
            __builtin_unreachable();
    }
}

template <bool UPDATE>
void Position::make_promotion(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(from);
    remove_piece<UPDATE>(piece, from);
    piece = get_piece(move.promotee(), m_stm);
    add_piece<UPDATE>(piece, to);
}

template <bool UPDATE>
void Position::make_en_passant(const Move &move) {
    m_curr_state.fifty_move_ply = 0;
    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(from);
    Square captured_square = static_cast<Square>(to - static_cast<int>(get_pawn_offset(m_stm)));
    Piece captured = consult(captured_square);
    m_curr_state.captured = captured;
    remove_piece<UPDATE>(captured, captured_square);
    move_piece<UPDATE>(piece, from, to);
}

void Position::update_castling_rights(const Move &move) {
    Square from = move.from();
    Square to = move.to();
    PieceType moved_piece_type = get_piece_type(consult(to), m_stm); // Piece has already been moved

    if (moved_piece_type == KING) { // Moved king
        switch (m_stm) {
            case WHITE:
                unset_mask(m_curr_state.castling_rights, static_cast<uint8_t>(WHITE_CASTLING));
                break;
            case BLACK:
                unset_mask(m_curr_state.castling_rights, static_cast<uint8_t>(BLACK_CASTLING));
                break;
            default:
                __builtin_unreachable();
        }
    } else if (moved_piece_type == ROOK) { // Moved rook
        switch (from) {
            case a1:
                unset_mask(m_curr_state.castling_rights, static_cast<uint8_t>(WHITE_OOO));
                break;
            case h1:
                unset_mask(m_curr_state.castling_rights, static_cast<uint8_t>(WHITE_OO));
                break;
            case a8:
                unset_mask(m_curr_state.castling_rights, static_cast<uint8_t>(BLACK_OOO));
                break;
            case h8:
                unset_mask(m_curr_state.castling_rights, static_cast<uint8_t>(BLACK_OO));
                break;
            default:
                break;
        }
    }
    if (get_piece_type(m_curr_state.captured) == ROOK) { // Captured rook
        switch (to) {
            case a1:
                unset_mask(m_curr_state.castling_rights, static_cast<uint8_t>(WHITE_OOO));
                break;
            case h1:
                unset_mask(m_curr_state.castling_rights, static_cast<uint8_t>(WHITE_OO));
                break;
            case a8:
                unset_mask(m_curr_state.castling_rights, static_cast<uint8_t>(BLACK_OOO));
                break;
            case h8:
                unset_mask(m_curr_state.castling_rights, static_cast<uint8_t>(BLACK_OO));
                break;
            default:
                break;
        }
    }
}

template <bool UPDATE>
void Position::unmake_move(const Move &move) {
    assert(m_history_ply > 0); // check if there is a move to unmake
    if constexpr (UPDATE)
        m_nnue.pop();

    --m_game_clock_ply;

    change_side();

    Square from = move.from();
    Square to = move.to();
    Piece piece = consult(to);

    if (move.is_regular()) {
        move_piece<false>(piece, to, from);
    } else if (move.is_capture() && !move.is_ep()) {
        remove_piece<false>(piece, to);
        add_piece<false>(m_curr_state.captured, to);
        if (move.is_promotion()) {
            piece = get_piece(PAWN, m_stm);
        }
        add_piece<false>(piece, from);
    } else if (move.is_castle()) {
        move_piece<false>(piece, to, from);
        switch (to) {
            case g1: // White castle short
                move_piece<false>(WHITE_ROOK, f1, h1);
                break;
            case c1: // White castle long
                move_piece<false>(WHITE_ROOK, d1, a1);
                break;
            case g8: // Black castle short
                move_piece<false>(BLACK_ROOK, f8, h8);
                break;
            case c8: // Black castle long
                move_piece<false>(BLACK_ROOK, d8, a8);
                break;
            default:
                __builtin_unreachable();
        }
    } else if (move.is_promotion()) {
        remove_piece<false>(piece, to);
        piece = get_piece(PAWN, m_stm);
        add_piece<false>(piece, from);
    } else if (move.is_ep()) {
        move_piece<false>(piece, to, from);
        Square captured_square = static_cast<Square>(to - static_cast<int>(get_pawn_offset(m_stm)));
        add_piece<false>(m_curr_state.captured, captured_square);
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

template void Position::unmake_move<true>(const Move &move);
template void Position::unmake_move<false>(const Move &move);

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
}

void Position::unmake_null_move() {
    --m_history_ply;
    m_curr_state = m_history_stack[m_history_ply];
    m_hash_key = m_played_positions[m_history_ply];
    --m_game_clock_ply;
    change_side();
}

Move Position::get_movement(const std::string &algebraic_notation) const {
    Square from = get_square(algebraic_notation[0] - 'a', algebraic_notation[1] - '1');
    Square to = get_square(algebraic_notation[2] - 'a', algebraic_notation[3] - '1');
    MoveType move_type = MoveType::REGULAR;

    if (algebraic_notation.size() == 5) { // Pawn promotion
        if (tolower(algebraic_notation[4]) == 'q')
            move_type = PAWN_PROMOTION_QUEEN;
        else if (tolower(algebraic_notation[4]) == 'n')
            move_type = PAWN_PROMOTION_KNIGHT;
        else if (tolower(algebraic_notation[4]) == 'r')
            move_type = PAWN_PROMOTION_ROOK;
        else if (tolower(algebraic_notation[4]) == 'b')
            move_type = PAWN_PROMOTION_BISHOP;
    }

    if (consult(to) != EMPTY) { // Capture
        move_type = static_cast<MoveType>(move_type | CAPTURE);
    } else if (get_piece_type(consult(from)) == KING && get_file(from) == 4 &&
               (get_file(to) == 2 || get_file(to) == 6)) { // Castle
        move_type = CASTLING;
    } else if (get_piece_type(consult(from)) == PAWN && get_file(to) != get_file(from)) { // En passant
        // Note that if this point is reached, consult(to) == empty, which is why this simple clause suffices, i.e. if
        // it's not a simple capture and the pawn is not on its original file, it must be an en passant
        assert(get_file(to) == get_file(get_en_passant()));
        move_type = EP;
    }

    return Move(from, to, move_type);
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
    for (ScoredMove *begin = moves; begin != end; ++begin) {
        if (make_move<false>(begin->move))
            ++legal_amount;
        unmake_move<false>(begin->move);
    }
    return legal_amount;
}

bool Position::no_legal_moves() {
    ScoredMove moves[MAX_MOVES_PER_POS];
    ScoredMove *end = gen_moves(moves, *this, GEN_ALL);
    for (ScoredMove *curr = moves; curr != end; ++curr) {
        bool legal = make_move<false>(curr->move);
        unmake_move<false>(curr->move);

        if (legal)
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
            char piece_char = ' ';
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

            std::cout << "| " << piece_char << " ";
        }
        std::cout << "| " << rank + 1 << "\n";
    }

    print_line();
    for (char rank_simbol = 'a'; rank_simbol <= 'h'; ++rank_simbol)
        std::cout << "  " << rank_simbol << " ";

    std::cout << "\n\nFEN: " << get_fen();
    std::cout << "\nHash: " << m_hash_key;
    std::cout << "\nEval: " << eval() << "\n";
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
    assert(m_history_ply >= m_curr_state.fifty_move_ply);

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

// TODO it would be a little more efficient to do a specific legality check for the move
bool Position::fifty_move_draw() {
    if (m_curr_state.fifty_move_ply >= 100) {
        ScoredMove moves[MAX_MOVES_PER_POS];
        ScoredMove *end = gen_moves(moves, *this, GEN_ALL);
        for (ScoredMove *begin = moves; begin != end; ++begin) {
            bool legal = make_move<false>(begin->move);
            unmake_move<false>(begin->move);
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
