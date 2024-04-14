/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "board.h"
#include "game_elements.h"
#include "weights.h"

Board::Board()
    : m_white_castling_rights(CastlingRights::BothSides),
      m_black_castling_rights(CastlingRights::BothSides),
      m_white_king_position(3, 3), m_black_king_position(3, 3),
      m_en_passant(0, 0), m_fifty_move_counter(0), m_turn(Player::White) {
  reset(); // initializes m_board
}

void Board::reset() {
  // TODO
}

Square Board::consult_position(const Position &position) const {
  return consult_position(position.file, position.rank);
}

Square Board::consult_position(const IndexType &file,
                               const IndexType &rank) const {
  return m_board[file + FileOffset][rank + RankOffset];
}

void Board::move_piece(const Movement &movement) {
  CounterType past_fifty_move_counter = m_fifty_move_counter++;
  PiecePlacement past_en_passant = m_en_passant;
  m_en_passant =
      PiecePlacement(0, 0); // denotes that an en passant move are not possible.
  Square captured =
      m_board[movement.to.file + FileOffset][movement.to.rank + RankOffset];
  Square empty_square = Square(Piece::None, Player::None);
  if (m_board[movement.to.file + FileOffset][movement.to.rank + RankOffset]
          .piece != Piece::None) {
    m_fifty_move_counter = 0;
  } else if (m_board[movement.from.file + FileOffset]
                    [movement.from.rank + RankOffset]
                        .piece == Piece::Pawn) {
    m_fifty_move_counter = 0;
    if (movement.to.file - movement.from.file == 2) {
      m_en_passant = movement.to;
    }
  }
  if (movement.move_type == MoveType::Regular) {
    // check for capture
    m_board[movement.to.file + FileOffset][movement.to.rank + RankOffset] =
        m_board[movement.from.file + FileOffset]
               [movement.from.rank + RankOffset];
    m_board[movement.from.file + FileOffset][movement.from.rank + RankOffset] =
        empty_square;
  } else if (movement.move_type == MoveType::EnPassant) {
    IndexType offset = captured.player == Player::White ? -1 : 1;
    captured = m_board[movement.to.file + offset + FileOffset]
                      [movement.to.rank + RankOffset];
    m_board[movement.to.file + FileOffset][movement.to.rank + RankOffset] =
        m_board[movement.from.file + FileOffset]
               [movement.from.rank + RankOffset];
    m_board[movement.from.file + FileOffset][movement.from.rank + RankOffset] =
        empty_square;
  } else if (movement.move_type == MoveType::PawnPromotion) {
    m_board[movement.from.file + FileOffset][movement.from.rank + RankOffset] =
        empty_square;
    m_board[movement.to.file + FileOffset][movement.to.rank + RankOffset] =
        Square(Piece::Queen, m_side_to_move);
  } else if (movement.move_type == MoveType::KingSideCastling) {
    IndexType file;
    if (m_board[movement.to.file][movement.to.rank].player == Player::White) {
      m_white_castling_rights.king_side = false;
      file = 0;
    } else {
      m_black_castling_rights.king_side = false;
      file = 7;
    }
    m_board[file + FileOffset][6 + RankOffset] =
        m_board[file + FileOffset][4 + RankOffset];
    m_board[file + FileOffset][5 + RankOffset] =
        m_board[file + FileOffset][7 + RankOffset];
    m_board[file + FileOffset][4 + RankOffset] = empty_square;
    m_board[file + FileOffset][7 + RankOffset] = empty_square;
  } else if (movement.move_type == MoveType::QueenSideCastling) {
    IndexType file;
    if (m_board[movement.to.file][movement.to.rank].player == Player::White) {
      m_white_castling_rights.queen_side = false;
      file = 0;
    } else {
      m_black_castling_rights.queen_side = false;
      file = 7;
    }
    m_board[file + FileOffset][2 + RankOffset] =
        m_board[file + FileOffset][4 + RankOffset];
    m_board[file + FileOffset][3 + RankOffset] =
        m_board[file + FileOffset][0 + RankOffset];
    m_board[file + FileOffset][4 + RankOffset] = empty_square;
    m_board[file + FileOffset][0 + RankOffset] = empty_square;
  }
  m_game_history.push(PastMovement(movement, captured, past_fifty_move_counter,
                                   past_en_passant));
  m_side_to_move =
      (m_side_to_move == Player::White) ? Player::Black : Player::White;
}

void Board::undo_move() {
  PastMovement undo = m_game_history.top();
  m_game_history.pop();
  m_fifty_move_counter = undo.past_fifty_move_counter;
  m_en_passant = undo.past_en_passant;
  if (undo.movement.move_type == MoveType::Regular) {
    m_board[undo.movement.from.file + FileOffset]
           [undo.movement.from.rank + RankOffset] =
               m_board[undo.movement.to.file + FileOffset]
                      [undo.movement.to.rank + RankOffset];
    m_fifty_move_counter = undo.past_fifty_move_counter;
    m_board[undo.movement.to.file + FileOffset]
           [undo.movement.to.rank + RankOffset] = undo.captured;
  } else if (undo.movement.move_type == MoveType::EnPassant) {
    IndexType offset = undo.captured.player == Player::White ? -1 : 1;
    m_board[undo.movement.from.file + FileOffset]
           [undo.movement.from.rank + RankOffset] =
               m_board[undo.movement.to.file + FileOffset]
                      [undo.movement.to.rank + RankOffset];
    m_fifty_move_counter = undo.past_fifty_move_counter;
    m_board[undo.movement.to.file + FileOffset + offset]
           [undo.movement.to.rank + RankOffset] = undo.captured;
  } else if (undo.movement.move_type == MoveType::PawnPromotion) {
    m_board[undo.movement.to.file + FileOffset]
           [undo.movement.to.rank + RankOffset]
               .piece = Piece::Pawn;
    m_board[undo.movement.from.file + FileOffset]
           [undo.movement.from.rank + RankOffset] =
               m_board[undo.movement.to.file + FileOffset]
                      [undo.movement.to.rank + RankOffset];
    m_fifty_move_counter = undo.past_fifty_move_counter;
    m_board[undo.movement.to.file + FileOffset]
           [undo.movement.to.rank + RankOffset] = undo.captured;
  } else if (undo.movement.move_type == MoveType::KingSideCastling) {
    IndexType file;
    if (m_board[undo.movement.to.file][undo.movement.to.rank].player ==
        Player::White) {
      m_white_castling_rights.king_side = true;
      file = 0;
    } else {
      m_black_castling_rights.king_side = true;
      file = 7;
    }
    m_board[file + FileOffset][4 + RankOffset] =
        m_board[file + FileOffset][6 + RankOffset];
    m_board[file + FileOffset][7 + RankOffset] =
        m_board[file + FileOffset][5 + RankOffset];
    m_board[file + FileOffset][6 + RankOffset] =
        Square(Piece::None, Player::None);
    m_board[file + FileOffset][5 + RankOffset] =
        Square(Piece::None, Player::None);
  } else if (undo.movement.move_type == MoveType::QueenSideCastling) {
    IndexType file;
    if (m_board[undo.movement.to.file][undo.movement.to.rank].player ==
        Player::White) {
      m_white_castling_rights.queen_side = true;
      file = 0;
    } else {
      m_black_castling_rights.queen_side = true;
      file = 7;
    }
    m_board[file + FileOffset][4 + RankOffset] =
        m_board[file + FileOffset][2 + RankOffset];
    m_board[file + FileOffset][0 + RankOffset] =
        m_board[file + FileOffset][3 + RankOffset];
    m_board[file + FileOffset][2 + RankOffset] =
        Square(Piece::None, Player::None);
    m_board[file + FileOffset][3 + RankOffset] =
        Square(Piece::None, Player::None);
  }
  m_side_to_move =
      (m_side_to_move == Player::White) ? Player::Black : Player::White;
}

WeightType Board::evaluate() const {
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
      mid_game_evaluation += (*MidGamePointerTable[piece_idx])[file_idx][rank] *
                             piece_multiplication_factor;
      end_game_evaluation += (*EndGamePointerTable[piece_idx])[file_idx][rank] *
                             piece_multiplication_factor;
      game_state += PhaseWeightTable[piece_idx];
    }
  }
  WeightType mid_game_weight;
  if (game_state > MidGamePhaseMax) {
    mid_game_weight = MidGamePhaseMax;
  } else {
    mid_game_weight = game_state;
  }
  WeightType end_game_weight = MidGamePhaseMax - mid_game_weight;
  return (mid_game_evaluation * mid_game_weight +
          end_game_evaluation * end_game_weight) /
         MidGamePhaseMax; // tapered_evaluation
}

MovementList Board::get_legal_moves() const {
  MovementList movement_list;
  return movement_list; // Just a STUB.
}

bool Board::check() const {
  return false; // TODO remove this, just a STUB.
}

bool Board::double_check() const {
  return false; // TODO remove this, just a STUB.
}

void Board::legal_pawn_moves(const Position &position,
                             MovementList *movement_list) const {}

void Board::legal_bishop_moves(const Position &position,
                               MovementList *movement_list) const {}

void Board::legal_knight_moves(const Position &position,
                               MovementList *movement_list) const {}

void Board::legal_rook_moves(const Position &position,
                             MovementList *movement_list) const {}

void Board::legal_queen_moves(const Position &position,
                              MovementList *movement_list) const {}

void Board::legal_king_moves(const Position &position,
                             MovementList *movement_list) const {}
