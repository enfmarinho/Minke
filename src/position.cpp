/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "position.h"
#include "game_elements.h"
#include "hashing.h"
#include "movegen.h"
#include <array>
#include <cassert>
#include <cctype>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>

Position::Position() { assert(reset(StartFEN)); }

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
  for (char c : fen_arguments[0]) {
    if (c == '/') {
      --file;
      rank = 0;
      continue;
    }
    IndexType n_of_empty_squares = std::isdigit(c);
    if (n_of_empty_squares == 0) {
      char piece = std::tolower(c);
      Player player = std::isupper(c) ? Player::White : Player::Black;
      if (piece == 'p')
        consult_legal_position(file, rank) = Square(Piece::Pawn, player);
      else if (piece == 'n')
        consult_legal_position(file, rank) = Square(Piece::Knight, player);
      else if (piece == 'b')
        consult_legal_position(file, rank) = Square(Piece::Bishop, player);
      else if (piece == 'r')
        consult_legal_position(file, rank) = Square(Piece::Rook, player);
      else if (piece == 'q')
        consult_legal_position(file, rank) = Square(Piece::Queen, player);
      else if (piece == 'k') {
        consult_legal_position(file, rank) = Square(Piece::King, player);
        if (player == Player::White) {
          m_white_king_position = PiecePlacement(file, rank);
        } else {
          m_black_king_position = PiecePlacement(file, rank);
        }
      }
      ++rank;
    }
    for (; n_of_empty_squares > 0; --n_of_empty_squares, ++rank) {
      consult_legal_position(file, rank) = Square(Piece::None, Player::None);
    }
  }

  if (fen_arguments[1] == "w") {
    m_side_to_move = Player::White;
  } else if (fen_arguments[1] == "b") {
    m_side_to_move = Player::Black;
  } else {
    std::cerr << "INVALID FEN: invalid player, it should be 'w' or 'b'."
              << std::endl;
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

  if (fen_arguments[3] == "-") {
    m_en_passant = -1;
  } else {
    try {
      m_en_passant = std::stoi(fen_arguments[3]) - 1;
    } catch (const std::exception &) {
      std::cerr << "INVALID FEN: en passant square is not a number nor '-'."
                << std::endl;
      return false;
    }
  }
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

Square Position::consult_legal_position(const IndexType &file,
                                        const IndexType &rank) const {
  return m_board[file + (rank << 4)];
}

Square Position::consult_legal_position(const PiecePlacement &placement) const {
  return m_board[placement.index()];
}

Square &Position::consult_legal_position(const IndexType &file,
                                         const IndexType &rank) {
  return m_board[file + (rank << 4)];
}

Square &Position::consult_legal_position(const PiecePlacement &placement) {
  return m_board[placement.index()];
}

bool Position::move(const Movement &movement) {
  CastlingRights past_white_castling_rights = m_white_castling_rights;
  CastlingRights past_black_castling_rights = m_black_castling_rights;
  CounterType past_fifty_move_counter = m_fifty_move_counter_ply++;
  IndexType past_en_passant = m_en_passant;
  m_en_passant = -1;
  Square captured = consult_legal_position(movement.to);
  Square empty_square = Square(Piece::None, Player::None);

  Piece piece_being_moved = consult_legal_position(movement.from).piece;
  if (consult_legal_position(movement.to).piece != Piece::None) {
    m_fifty_move_counter_ply = 0;
  } else if (piece_being_moved == Piece::Pawn) {
    m_fifty_move_counter_ply = 0;
    if (abs(movement.to.file() - movement.from.file()) == 2) {
      m_en_passant = movement.to.rank();
    }
  }

  CastlingRights &current_player_castling_rights =
      m_side_to_move == Player::White ? m_white_castling_rights
                                      : m_black_castling_rights;
  if (piece_being_moved == Piece::King) {
    current_player_castling_rights.king_side = false;
    current_player_castling_rights.queen_side = false;
  } else if (piece_being_moved == Piece::Rook) {
    IndexType player_perspective_first_file =
        m_side_to_move == Player::White ? 0 : 7;
    if (movement.from.rank() == 7 &&
        movement.from.file() == player_perspective_first_file) {
      current_player_castling_rights.king_side = false;
    } else if (movement.from.rank() == 0 &&
               movement.from.file() == player_perspective_first_file) {
      current_player_castling_rights.queen_side = false;
    }
  }

  if (movement.move_type == MoveType::Regular) {
    consult_legal_position(movement.to) = consult_legal_position(movement.from);
    consult_legal_position(movement.from) = empty_square;
  } else if (movement.move_type == MoveType::EnPassant) {
    IndexType offset = captured.player == Player::White ? -1 : 1;
    captured =
        consult_legal_position(movement.to.file() + offset, movement.to.rank());
    consult_legal_position(movement.to) = consult_legal_position(movement.from);
    consult_legal_position(movement.from) = empty_square;
  } else if (movement.move_type == MoveType::PawnPromotionQueen) {
    consult_legal_position(movement.from) = empty_square;
    consult_legal_position(movement.to) = Square(Piece::Queen, m_side_to_move);
  } else if (movement.move_type == MoveType::PawnPromotionKnight) {
    consult_legal_position(movement.from) = empty_square;
    consult_legal_position(movement.to) = Square(Piece::Knight, m_side_to_move);
  } else if (movement.move_type == MoveType::PawnPromotionRook) {
    consult_legal_position(movement.from) = empty_square;
    consult_legal_position(movement.to) = Square(Piece::Rook, m_side_to_move);
  } else if (movement.move_type == MoveType::PawnPromotionBishop) {
    consult_legal_position(movement.from) = empty_square;
    consult_legal_position(movement.to) = Square(Piece::Bishop, m_side_to_move);
  } else if (movement.move_type == MoveType::KingSideCastling) {
    IndexType file;
    if (m_side_to_move == Player::White) {
      m_white_castling_rights.king_side = false;
      m_white_castling_rights.queen_side = false;
      file = 0;
    } else {
      m_black_castling_rights.king_side = false;
      m_black_castling_rights.queen_side = false;
      file = 7;
    }
    consult_legal_position(file, 6) = consult_legal_position(file, 4);
    consult_legal_position(file, 5) = consult_legal_position(file, 7);
    consult_legal_position(file, 4) = empty_square;
    consult_legal_position(file, 7) = empty_square;
  } else if (movement.move_type == MoveType::QueenSideCastling) {
    IndexType file;
    if (m_side_to_move == Player::White) {
      m_white_castling_rights.king_side = false;
      m_white_castling_rights.queen_side = false;
      file = 0;
    } else {
      m_black_castling_rights.king_side = false;
      m_black_castling_rights.queen_side = false;
      file = 7;
    }
    consult_legal_position(file, 2) = consult_legal_position(file, 4);
    consult_legal_position(file, 3) = consult_legal_position(file, 0);
    consult_legal_position(file, 4) = empty_square;
    consult_legal_position(file, 0) = empty_square;
  }
  // Required because of the pseudo-legal move generator, less than not ideal
  if (under_attack(*this,
                   m_side_to_move == Player::White ? m_white_king_position
                                                   : m_black_king_position,
                   m_side_to_move)) {
    return false;
  }
  m_game_history.push(PastMovement(movement, captured, past_fifty_move_counter,
                                   past_en_passant, past_white_castling_rights,
                                   past_black_castling_rights));
  m_side_to_move =
      (m_side_to_move == Player::White) ? Player::Black : Player::White;
  m_hash = zobrist::rehash(*this);
  return true;
}

Movement Position::get_movement(const std::string &algebraic_notation) const {
  PiecePlacement from(static_cast<IndexType>(algebraic_notation[1] - '1'),
                      static_cast<IndexType>(algebraic_notation[0] - 'a'));
  PiecePlacement to(static_cast<IndexType>(algebraic_notation[3] - '1'),
                    static_cast<IndexType>(algebraic_notation[2] - 'a'));
  MoveType move_type = MoveType::Regular;

  // Castling
  if ((from.rank() == 4 && to.rank() == 6) &&
      consult_legal_position(from).piece == Piece::King)
    move_type = MoveType::KingSideCastling;
  else if ((from.rank() == 4 && to.rank() == 2) &&
           consult_legal_position(from).piece == Piece::King)
    move_type = MoveType::QueenSideCastling;
  // En passant
  else if (to.rank() == en_passant_rank() && from.file() != to.file() &&
           consult_legal_position(from).piece == Piece::Pawn) {
    move_type = MoveType::EnPassant;
  }
  // Pawn promotion
  else if (algebraic_notation.size() == 5) {
    if (tolower(algebraic_notation[4]) == 'q')
      move_type = MoveType::PawnPromotionQueen;
    else if (tolower(algebraic_notation[4]) == 'n')
      move_type = MoveType::PawnPromotionKnight;
    else if (tolower(algebraic_notation[4]) == 'r')
      move_type = MoveType::PawnPromotionRook;
    else if (tolower(algebraic_notation[4]) == 'b')
      move_type = MoveType::PawnPromotionBishop;
  }

  return Movement(from, to, move_type);
}

const Player &Position::side_to_move() const { return m_side_to_move; }

const CastlingRights &Position::white_castling_rights() const {
  return m_white_castling_rights;
}

const CastlingRights &Position::black_castling_rights() const {
  return m_black_castling_rights;
}

const IndexType &Position::en_passant_rank() const { return m_en_passant; }

const PastMovement &Position::last_move() const { return m_game_history.top(); }

const PiecePlacement &Position::black_king_position() const {
  return m_black_king_position;
}

const PiecePlacement &Position::white_king_position() const {
  return m_white_king_position;
}

const HashType &Position::get_hash() const { return m_hash; }

std::string Position::get_fen() const {
  std::string fen;
  for (IndexType file = 7; file >= 0; --file) {
    IndexType counter = 0;
    for (IndexType rank = 0; rank < BoardWidth; ++rank) {
      const Square &curr_square = consult_legal_position(file, rank);
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
    if (counter > 0) {
      fen += ('0' + counter);
    }
    if (file != 0) {
      fen += '/';
    }
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

  fen += static_cast<char>(m_fifty_move_counter_ply + '0');
  fen += ' ';
  fen += static_cast<char>(m_game_clock_ply + '0');

  return fen;
}

const CounterType &Position::get_half_move_counter() const {
  return m_game_clock_ply;
}

void Position::print_board() const {
  auto print_line = []() -> void {
    for (IndexType i = 0; i < 8; ++i) {
      std::cout << "+";
      for (IndexType j = 0; j < 3; ++j) {
        std::cout << "-";
      }
    }
    std::cout << "+\n";
  };

  for (IndexType file = 7; file >= 0; --file) {
    print_line();
    for (IndexType rank = 0; rank < BoardWidth; ++rank) {
      std::cout << "| ";
      const Square &curr_square = consult_legal_position(file, rank);
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
      std::cout << (curr_square.player == Player::White
                        ? static_cast<char>(toupper(piece))
                        : piece)
                << " ";
    }
    std::cout << "| " << file + 1 << std::endl;
  }
  print_line();
  for (char rank_simbol = 'a'; rank_simbol <= 'h'; ++rank_simbol) {
    std::cout << "  " << rank_simbol << " ";
  }
  std::cout << "\n\nFEN: " << get_fen() << std::endl;
};
