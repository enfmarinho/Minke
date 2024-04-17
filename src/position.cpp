/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "position.h"
#include "game_elements.h"
#include "weights.h"

Position::Position()
    : m_white_castling_rights(), m_black_castling_rights(),
      m_white_king_position(0 + FileOffset, 4 + RankOffset),
      m_black_king_position(7 + FileOffset, 4 + RankOffset), m_en_passant(-1),
      m_fifty_move_counter(0), m_side_to_move(Player::White) {
  reset(); // initialize m_board
}

void Position::reset() {
  // TODO
}

Square Position::consult_position(const PiecePlacement &position) const {
  return consult_position(position.file, position.rank);
}

Square Position::consult_position(const IndexType &file,
                                  const IndexType &rank) const {
  return m_board[file][rank];
}

Square Position::consult_legal_position(const IndexType &file,
                                        const IndexType &rank) const {
  return m_board[file + FileOffset][rank + RankOffset];
}

Square Position::consult_legal_position(const PiecePlacement &position) const {
  return m_board[position.file + FileOffset][position.rank + RankOffset];
}

Square &Position::consult_legal_position(const IndexType &file,
                                         const IndexType &rank) {
  return m_board[file + FileOffset][rank + RankOffset];
}

Square &Position::consult_legal_position(const PiecePlacement &position) {
  return m_board[position.file + FileOffset][position.rank + RankOffset];
}

void Position::move_piece(const Movement &movement) {
  CastlingRights past_white_castling_rights = m_white_castling_rights;
  CastlingRights past_black_castling_rights = m_black_castling_rights;
  CounterType past_fifty_move_counter = m_fifty_move_counter++;
  IndexType past_en_passant = m_en_passant;
  m_en_passant = -1;
  Square captured = consult_legal_position(movement.to);
  Square empty_square = Square(Piece::None, Player::None);

  Piece piece_being_moved = consult_legal_position(movement.from).piece;
  if (consult_legal_position(movement.to).piece != Piece::None) {
    m_fifty_move_counter = 0;
  } else if (piece_being_moved == Piece::Pawn) {
    m_fifty_move_counter = 0;
    if (abs(movement.to.file - movement.from.file) == 2) {
      m_en_passant = movement.to.rank;
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
    if (movement.from.rank == 7 &&
        movement.from.file == player_perspective_first_file) {
      current_player_castling_rights.king_side = false;
    } else if (movement.from.rank == 0 &&
               movement.from.file == player_perspective_first_file) {
      current_player_castling_rights.queen_side = false;
    }
  }

  if (movement.move_type == MoveType::Regular) {
    consult_legal_position(movement.to) = consult_legal_position(movement.from);
    consult_legal_position(movement.from) = empty_square;
  } else if (movement.move_type == MoveType::EnPassant) {
    IndexType offset = captured.player == Player::White ? -1 : 1;
    captured =
        consult_legal_position(movement.to.file + offset, movement.to.rank);
    consult_legal_position(movement.to) = consult_legal_position(movement.from);
    consult_legal_position(movement.from) = empty_square;
  } else if (movement.move_type == MoveType::PawnPromotion) {
    consult_legal_position(movement.from) = empty_square;
    consult_legal_position(movement.to) = Square(Piece::Queen, m_side_to_move);
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
  m_side_to_move =
      (m_side_to_move == Player::White) ? Player::Black : Player::White;
  m_game_history.push(PastMovement(movement, captured, past_fifty_move_counter,
                                   past_en_passant, past_white_castling_rights,
                                   past_black_castling_rights));
}

void Position::undo_move() {
  PastMovement undo = m_game_history.top();
  m_game_history.pop();
  m_en_passant = undo.past_en_passant;
  m_fifty_move_counter = undo.past_fifty_move_counter;
  m_white_castling_rights = undo.past_white_castling_rights;
  m_black_castling_rights = undo.past_black_castling_rights;
  m_side_to_move =
      (m_side_to_move == Player::White) ? Player::Black : Player::White;
  if (undo.movement.move_type == MoveType::Regular) {
    consult_legal_position(undo.movement.from) =
        consult_legal_position(undo.movement.to);
    consult_legal_position(undo.movement.to) = undo.captured;
  } else if (undo.movement.move_type == MoveType::EnPassant) {
    IndexType offset = undo.captured.player == Player::White ? -1 : 1;
    consult_legal_position(undo.movement.from) =
        consult_legal_position(undo.movement.to);
    consult_legal_position(undo.movement.to.file + offset,
                           undo.movement.to.rank) = undo.captured;
  } else if (undo.movement.move_type == MoveType::PawnPromotion) {
    consult_legal_position(undo.movement.to).piece = Piece::Pawn;
    consult_legal_position(undo.movement.from) =
        consult_legal_position(undo.movement.to);
    consult_legal_position(undo.movement.to) = undo.captured;
  } else if (undo.movement.move_type == MoveType::KingSideCastling) {
    IndexType file;
    if (m_side_to_move == Player::White) {
      file = 0;
    } else {
      file = 7;
    }
    consult_legal_position(file, 4) = consult_legal_position(file, 6);
    consult_legal_position(file, 7) = consult_legal_position(file, 5);
    consult_legal_position(file, 6) = Square(Piece::None, Player::None);
    consult_legal_position(file, 5) = Square(Piece::None, Player::None);
  } else if (undo.movement.move_type == MoveType::QueenSideCastling) {
    IndexType file;
    if (m_side_to_move == Player::White) {
      file = 0;
    } else {
      file = 7;
    }
    consult_legal_position(file, 4) = consult_legal_position(file, 2);
    consult_legal_position(file, 0) = consult_legal_position(file, 3);
    consult_legal_position(file, 2) = Square(Piece::None, Player::None);
    consult_legal_position(file, 3) = Square(Piece::None, Player::None);
  }
}

WeightType Position::evaluate() const {
  WeightType mid_game_evaluation = 0, end_game_evaluation = 0, game_state = 0;
  for (IndexType file = 0; file < BoardHeight; ++file) {
    for (IndexType rank = 0; rank < BoardWidth; ++rank) {
      Square square = consult_position(file, rank);
      WeightType piece_multiplication_factor =
          static_cast<WeightType>(square.player);
      IndexType piece_idx = static_cast<IndexType>(square.piece);
      IndexType file_idx;
      if (square.player == Player::White) {
        file_idx = file;
      } else {
        file_idx = file ^ 7;
      }
      mid_game_evaluation +=
          (*weights::MidGamePointerTable[piece_idx])[file_idx][rank] *
          piece_multiplication_factor;
      end_game_evaluation +=
          (*weights::EndGamePointerTable[piece_idx])[file_idx][rank] *
          piece_multiplication_factor;
      game_state += weights::PhaseTable[piece_idx];
    }
  }
  WeightType mid_game_weight;
  if (game_state > weights::MidGamePhaseMax) {
    mid_game_weight = weights::MidGamePhaseMax;
  } else {
    mid_game_weight = game_state;
  }
  WeightType end_game_weight = weights::MidGamePhaseMax - mid_game_weight;
  return (mid_game_evaluation * mid_game_weight +
          end_game_evaluation * end_game_weight) /
         weights::MidGamePhaseMax; // tapered_evaluation
}

MovementList Position::get_legal_moves() const {
  MovementList movement_list;
  if (double_check()) {
    legal_king_moves(m_side_to_move == Player::White ? m_white_king_position
                                                     : m_black_king_position,
                     &movement_list);
    return movement_list;
  }
  for (IndexType file = 0; file < BoardHeight; ++file) {
    for (IndexType rank = 0; rank < BoardWidth; ++rank) {
      PiecePlacement current_square = PiecePlacement(file, rank);
      switch (consult_legal_position(current_square).piece) {
      case Piece::Pawn:
        legal_pawn_moves(current_square, &movement_list);
        break;
      case Piece::Knight:
        legal_knight_moves(current_square, &movement_list);
        break;
      case Piece::Bishop:
        legal_bishop_moves(current_square, &movement_list);
        break;
      case Piece::Rook:
        legal_rook_moves(current_square, &movement_list);
        break;
      case Piece::Queen:
        legal_queen_moves(current_square, &movement_list);
        break;
      case Piece::King:
        legal_king_moves(current_square, &movement_list);
        break;
      default:
        break;
      }
    }
  }
  return movement_list;
}

Player Position::side_to_move() const { return m_side_to_move; }

CastlingRights Position::white_castling_rights() const {
  return m_white_castling_rights;
}

CastlingRights Position::black_castling_rights() const {
  return m_black_castling_rights;
}

IndexType Position::en_passant_rank() const { return m_en_passant; }

PastMovement Position::last_move() const { return m_game_history.top(); }

bool Position::check() const {
  return false; // TODO remove this, just a STUB.
}

bool Position::double_check() const {
  return false; // TODO remove this, just a STUB.
}

void Position::legal_pawn_moves(const PiecePlacement &position,
                                MovementList *movement_list) const {
  // TODO
}

void Position::legal_knight_moves(const PiecePlacement &position,
                                  MovementList *movement_list) const {
  // TODO
}

void Position::legal_bishop_moves(const PiecePlacement &position,
                                  MovementList *movement_list) const {
  // TODO
}

void Position::legal_rook_moves(const PiecePlacement &position,
                                MovementList *movement_list) const {
  // TODO
}

void Position::legal_queen_moves(const PiecePlacement &position,
                                 MovementList *movement_list) const {
  // TODO
}

void Position::legal_king_moves(const PiecePlacement &position,
                                MovementList *movement_list) const {
  // TODO
}
