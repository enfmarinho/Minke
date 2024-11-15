/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "position.h"

#include <array>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>

#include "game_elements.h"
#include "hash.h"
#include "movegen.h"

bool Position::reset(const std::string &fen) {
    std::stringstream iss(fen);
    std::array<std::string, 6> fen_arguments;
    for (IndexType index = 0; index < 6; ++index) {
        iss >> std::skipws >> fen_arguments[index];
        if (iss.fail()) {
            std::cerr << "INVALID FEN: wrong format." << std::endl;
            return false;
        }
    }

    IndexType file = 7, rank = 0;
    m_total_material_count = 0;
    memset(m_material_count, 0, 2 * 5 * 4);
    for (char c : fen_arguments[0]) {
        if (c == '/') {
            --file;
            rank = 0;
            continue;
        }
        if (!std::isdigit(c)) {
            char piece = std::tolower(c);
            Player player = std::isupper(c) ? Player::White : Player::Black;
            IndexType player_index = std::isupper(c) ? 0 : 1;
            if (piece == 'p') {
                consult(file, rank) = Square(Piece::Pawn, player);
                ++m_material_count[player_index][0];
            } else if (piece == 'n') {
                consult(file, rank) = Square(Piece::Knight, player);
                ++m_material_count[player_index][1];
            } else if (piece == 'b') {
                consult(file, rank) = Square(Piece::Bishop, player);
                ++m_material_count[player_index][2];
            } else if (piece == 'r') {
                consult(file, rank) = Square(Piece::Rook, player);
                ++m_material_count[player_index][3];
            } else if (piece == 'q') {
                consult(file, rank) = Square(Piece::Queen, player);
                ++m_material_count[player_index][4];
            } else if (piece == 'k') {
                consult(file, rank) = Square(Piece::King, player);
                if (player == Player::White)
                    m_white_king_position = PiecePlacement(file, rank);
                else
                    m_black_king_position = PiecePlacement(file, rank);
            } else {
                assert(false);
            }
            ++m_total_material_count;
            ++rank;
        } else {
            for (IndexType n_of_empty_squares = c - '0'; n_of_empty_squares > 0; --n_of_empty_squares, ++rank) {
                consult(file, rank) = Square(Piece::None, Player::None);
            }
        }
    }

    if (fen_arguments[1] == "w") {
        m_side_to_move = Player::White;
    } else if (fen_arguments[1] == "b") {
        m_side_to_move = Player::Black;
    } else {
        std::cerr << "INVALID FEN: invalid player, it should be 'w' or 'b'." << std::endl;
        return false;
    }

    for (char castling : fen_arguments[2]) {
        if (castling == 'K')
            m_white_castling_rights.king_side = true;
        else if (castling == 'Q')
            m_white_castling_rights.queen_side = true;
        else if (castling == 'k')
            m_black_castling_rights.king_side = true;
        else if (castling == 'q')
            m_black_castling_rights.queen_side = true;
    }

    if (fen_arguments[3] == "-")
        m_en_passant = -1;
    else
        m_en_passant = fen_arguments[3][0] - 'a';

    try {
        m_fifty_move_counter_ply = std::stoi(fen_arguments[4]);
    } catch (const std::exception &) {
        std::cerr << "INVALID FEN: halfmove clock is not a number." << std::endl;
        return false;
    }
    try {
        m_game_clock_ply = std::stoi(fen_arguments[5]);
    } catch (const std::exception &) {
        std::cerr << "INVALID FEN: game clock is not a number." << std::endl;
        return false;
    }

    m_hash = zobrist::hash(*this);
    return true;
}

Square Position::consult(const IndexType &file, const IndexType &rank) const { return m_board[file + (rank << 4)]; }

Square Position::consult(const PiecePlacement &placement) const { return m_board[placement.index()]; }

Square &Position::consult(const IndexType &file, const IndexType &rank) { return m_board[file + (rank << 4)]; }

Square &Position::consult(const PiecePlacement &placement) { return m_board[placement.index()]; }

bool Position::move(const Move &movement) {
    CastlingRights past_white_castling_rights = m_white_castling_rights;
    CastlingRights past_black_castling_rights = m_black_castling_rights;
    CounterType past_fifty_move_counter = m_fifty_move_counter_ply;
    IndexType past_en_passant = m_en_passant;
    m_en_passant = -1;
    Square captured = consult(movement.to);

    ++m_fifty_move_counter_ply;
    ++m_game_clock_ply;
    Piece piece_being_moved = consult(movement.from).piece;
    if (consult(movement.to).piece != Piece::None) { // Capture
        m_fifty_move_counter_ply = 0;
    } else if (piece_being_moved == Piece::Pawn) {
        m_fifty_move_counter_ply = 0;
        if (abs(movement.to.file() - movement.from.file()) == 2) {
            PiecePlacement left = movement.to.index() + offsets::West;
            PiecePlacement right = movement.to.index() + offsets::East;
            Square enemy_pawn(Piece::Pawn, (m_side_to_move == Player::White ? Player::Black : Player::White));
            if ((!left.out_of_bounds() && consult(left) == enemy_pawn) ||
                (!right.out_of_bounds() && consult(right) == enemy_pawn)) {
                m_en_passant = movement.to.rank();
            }
        }
    }

    CastlingRights &current_player_castling_rights =
        m_side_to_move == Player::White ? m_white_castling_rights : m_black_castling_rights;
    if (piece_being_moved == Piece::King) {
        current_player_castling_rights.king_side = false;
        current_player_castling_rights.queen_side = false;
        (m_side_to_move == Player::White ? m_white_king_position : m_black_king_position) = movement.to;
    } else if (piece_being_moved == Piece::Rook) {
        IndexType player_perspective_first_file = m_side_to_move == Player::White ? 0 : 7;
        if (movement.from.rank() == 7 && movement.from.file() == player_perspective_first_file) {
            current_player_castling_rights.king_side = false;
        } else if (movement.from.rank() == 0 && movement.from.file() == player_perspective_first_file) {
            current_player_castling_rights.queen_side = false;
        }
    }
    CastlingRights &adversary_player_castling_rights =
        m_side_to_move == Player::White ? m_black_castling_rights : m_white_castling_rights;
    if (captured.piece == Piece::Rook) {
        IndexType adversary_perspective_first_file = m_side_to_move == Player::White ? 7 : 0;
        if (movement.to.rank() == 7 && movement.to.file() == adversary_perspective_first_file) {
            adversary_player_castling_rights.king_side = false;
        } else if (movement.to.rank() == 0 && movement.to.file() == adversary_perspective_first_file) {
            adversary_player_castling_rights.queen_side = false;
        }
    }

    if (movement.move_type == MoveType::Regular || movement.move_type == MoveType::Capture) {
        consult(movement.to) = consult(movement.from);
        consult(movement.from) = EmptySquare;
    } else if (movement.move_type == MoveType::EnPassant) {
        IndexType offset = side_to_move() == Player::White ? offsets::South : offsets::North;
        consult(movement.to) = consult(movement.from);
        consult(movement.from) = EmptySquare;
        captured = consult(movement.to.index() + offset);
        consult(movement.to.index() + offset) = EmptySquare;
    } else if (movement.move_type == MoveType::PawnPromotionQueen) {
        consult(movement.from) = EmptySquare;
        consult(movement.to) = Square(Piece::Queen, m_side_to_move);
    } else if (movement.move_type == MoveType::PawnPromotionKnight) {
        consult(movement.from) = EmptySquare;
        consult(movement.to) = Square(Piece::Knight, m_side_to_move);
    } else if (movement.move_type == MoveType::PawnPromotionRook) {
        consult(movement.from) = EmptySquare;
        consult(movement.to) = Square(Piece::Rook, m_side_to_move);
    } else if (movement.move_type == MoveType::PawnPromotionBishop) {
        consult(movement.from) = EmptySquare;
        consult(movement.to) = Square(Piece::Bishop, m_side_to_move);
    } else if (movement.move_type == MoveType::KingSideCastling) {
        IndexType file = m_side_to_move == Player::White ? 0 : 7;
        consult(file, 6) = consult(file, 4);
        consult(file, 5) = consult(file, 7);
        consult(file, 4) = EmptySquare;
        consult(file, 7) = EmptySquare;
    } else if (movement.move_type == MoveType::QueenSideCastling) {
        IndexType file = m_side_to_move == Player::White ? 0 : 7;
        consult(file, 2) = consult(file, 4);
        consult(file, 3) = consult(file, 0);
        consult(file, 4) = EmptySquare;
        consult(file, 0) = EmptySquare;
    }

    if (captured.piece != Piece::None) {
        --m_material_count[(captured.player == Player::White ? 0 : 1)][piece_index(captured.piece)];
        --m_total_material_count;
    }

    m_side_to_move = (m_side_to_move == Player::White) ? Player::Black : Player::White;
    m_hash = zobrist::rehash(*this, PastMove(movement, captured, past_en_passant, past_fifty_move_counter,
                                             past_white_castling_rights, past_black_castling_rights));

    assert(m_hash == zobrist::hash(*this)); // Checks if rehash is correct

    // Required because of the pseudo-legal move generator
    return !under_attack(*this, m_side_to_move,
                         m_side_to_move == Player::Black ? m_white_king_position : m_black_king_position);
}

Move Position::get_movement(const std::string &algebraic_notation) const {
    PiecePlacement from(static_cast<IndexType>(algebraic_notation[1] - '1'),
                        static_cast<IndexType>(algebraic_notation[0] - 'a'));
    PiecePlacement to(static_cast<IndexType>(algebraic_notation[3] - '1'),
                      static_cast<IndexType>(algebraic_notation[2] - 'a'));
    MoveType move_type = MoveType::Regular;

    if (algebraic_notation.size() == 5) { // Pawn promotion
        if (tolower(algebraic_notation[4]) == 'q')
            move_type = MoveType::PawnPromotionQueen;
        else if (tolower(algebraic_notation[4]) == 'n')
            move_type = MoveType::PawnPromotionKnight;
        else if (tolower(algebraic_notation[4]) == 'r')
            move_type = MoveType::PawnPromotionRook;
        else if (tolower(algebraic_notation[4]) == 'b')
            move_type = MoveType::PawnPromotionBishop;
    } else if (consult(to).piece != Piece::None) { // Capture
        move_type = MoveType::Capture;
    } else if ((from.rank() == 4 && to.rank() == 6) && consult(from).piece == Piece::King) { // Castling
        move_type = MoveType::KingSideCastling;
    } else if ((from.rank() == 4 && to.rank() == 2) && consult(from).piece == Piece::King) {
        move_type = MoveType::QueenSideCastling;
    } else if (to.rank() == en_passant_rank() && from.rank() != to.rank() &&
               consult(from).piece == Piece::Pawn) { // En passant
        move_type = MoveType::EnPassant;
    }

    return Move(from, to, move_type);
}

const Player &Position::side_to_move() const { return m_side_to_move; }

const CastlingRights &Position::white_castling_rights() const { return m_white_castling_rights; }

const CastlingRights &Position::black_castling_rights() const { return m_black_castling_rights; }

const IndexType &Position::en_passant_rank() const { return m_en_passant; }

const PiecePlacement &Position::black_king_position() const { return m_black_king_position; }

const PiecePlacement &Position::white_king_position() const { return m_white_king_position; }

const CounterType &Position::material_count(const Piece &piece, const Player &player) const {
    return m_material_count[(player == Player::White ? 0 : 1)][piece_index(piece)];
}

const HashType &Position::get_hash() const { return m_hash; }

std::string Position::get_fen() const {
    std::string fen;
    for (IndexType file = 7; file >= 0; --file) {
        IndexType counter = 0;
        for (IndexType rank = 0; rank < BoardWidth; ++rank) {
            const Square &curr_square = consult(file, rank);
            if (curr_square.piece == Piece::None) {
                ++counter;
                continue;
            } else if (counter > 0) {
                fen += ('0' + counter);
                counter = 0;
            }

            char piece;
            if (curr_square.piece == Piece::King)
                piece = 'k';
            else if (curr_square.piece == Piece::Queen)
                piece = 'q';
            else if (curr_square.piece == Piece::Bishop)
                piece = 'b';
            else if (curr_square.piece == Piece::Rook)
                piece = 'r';
            else if (curr_square.piece == Piece::Pawn)
                piece = 'p';
            else if (curr_square.piece == Piece::Knight)
                piece = 'n';

            if (curr_square.player == Player::White)
                fen += toupper(piece);
            else
                fen += piece;
        }
        if (counter > 0)
            fen += ('0' + counter);
        if (file != 0)
            fen += '/';
    }
    fen += (m_side_to_move == Player::White ? " w " : " b ");
    bool none = true;
    if (m_white_castling_rights.king_side) {
        none = false;
        fen += "K";
    }
    if (m_white_castling_rights.queen_side) {
        none = false;
        fen += "Q";
    }
    if (m_black_castling_rights.king_side) {
        none = false;
        fen += "k";
    }
    if (m_black_castling_rights.queen_side) {
        none = false;
        fen += "q";
    }
    if (none)
        fen += "-";

    fen += ' ';
    if (m_en_passant == -1)
        fen += "-";
    else {
        fen += static_cast<char>(m_en_passant + 'a');
        fen += (m_side_to_move == Player::White ? '6' : '3');
    }
    fen += ' ';

    fen += std::to_string(m_fifty_move_counter_ply);
    fen += ' ';
    fen += std::to_string((m_game_clock_ply + 1) / 2);

    return fen;
}

const CounterType &Position::get_half_move_counter() const { return m_game_clock_ply; }

const CounterType &Position::get_fifty_move_counter() const { return m_fifty_move_counter_ply; }

void Position::print() const {
    auto print_line = []() -> void {
        for (IndexType i = 0; i < 8; ++i) {
            std::cout << "+";
            for (IndexType j = 0; j < 3; ++j)
                std::cout << "-";
        }
        std::cout << "+\n";
    };

    for (IndexType file = 7; file >= 0; --file) {
        print_line();
        for (IndexType rank = 0; rank < BoardWidth; ++rank) {
            std::cout << "| ";
            const Square &curr_square = consult(file, rank);
            char piece;
            if (curr_square.piece == Piece::King)
                piece = 'k';
            else if (curr_square.piece == Piece::Queen)
                piece = 'q';
            else if (curr_square.piece == Piece::Bishop)
                piece = 'b';
            else if (curr_square.piece == Piece::Rook)
                piece = 'r';
            else if (curr_square.piece == Piece::Pawn)
                piece = 'p';
            else if (curr_square.piece == Piece::Knight)
                piece = 'n';
            else
                piece = ' ';
            std::cout << (curr_square.player == Player::White ? static_cast<char>(toupper(piece)) : piece) << " ";
        }
        std::cout << "| " << file + 1 << "\n";
    }
    print_line();
    for (char rank_simbol = 'a'; rank_simbol <= 'h'; ++rank_simbol) {
        std::cout << "  " << rank_simbol << " ";
    }
    std::cout << "\n\nFEN: " << get_fen();
    std::cout << "\nHash: " << m_hash << "\n";
};

bool Position::insufficient_material() const {
    if (m_total_material_count <= 2) {
        return true;
    } else if (m_total_material_count == 3) {
        if (m_material_count[0][piece_index(Piece::Knight)] == 1 ||
            m_material_count[1][piece_index(Piece::Knight)] == 1) // KN v K
            return false;
        else if (m_material_count[0][piece_index(Piece::Bishop)] == 1 ||
                 m_material_count[1][piece_index(Piece::Bishop)] == 1) // KB v K
            return false;
    } else if (m_total_material_count == 4) {
        if (m_material_count[0][piece_index(Piece::Knight)] == 2 ||
            m_material_count[1][piece_index(Piece::Knight)] == 2) // KNN v K
            return false;
        else if (m_material_count[0][piece_index(Piece::Knight)] == 1 &&
                 m_material_count[1][piece_index(Piece::Knight)] == 1) // KN v KN
            return false;
        else if (m_material_count[0][piece_index(Piece::Bishop)] == 1 &&
                 m_material_count[1][piece_index(Piece::Bishop)] == 1) // KB v KB
            return false;
    }
    return false;
}
