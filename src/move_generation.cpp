/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "move_generation.hpp"
#include "game_elements.hpp"
#include "position.hpp"
#include <cstdint>

bool under_atack(const Position &position, const PiecePlacement &pp) {
  // TODO implement this
  return false;
}

MoveList::MoveList(const Position &position) {
  m_end = m_movement_list;
  for (IndexType file = 0; file < BoardHeight; ++file) {
    for (IndexType rank = 0; rank < BoardWidth; ++rank) {
      PiecePlacement current = PiecePlacement(file, rank);
      const Square &square = position.consult_legal_position(current);
      if (square.player == Player::None ||
          position.side_to_move() != square.player) {
        continue;
      }
      switch (square.piece) {
      case Piece::Pawn:
        pseudolegal_pawn_moves(position, current);
        break;
      case Piece::Knight:
        pseudolegal_knight_moves(position, current);
        break;
      case Piece::King:
        pseudolegal_king_moves(position, current);
        break;
      default:
        pseudolegal_sliders_moves(position, current);
        break;
      }
    }
  }
  // TODO check for castling
}

void MoveList::pseudolegal_pawn_moves(const Position &position,
                                      const PiecePlacement &from) {
  IndexType original_file;
  int8_t offset;
  Player adversary;
  if (position.side_to_move() == Player::White) {
    original_file = 1;
    offset = directions::North;
    adversary = Player::Black;
  } else {
    original_file = 6;
    offset = directions::South;
    adversary = Player::White;
  }
  PiecePlacement doublemove(from.index() + 2 * offset);
  if (from.file() == original_file &&
      position.consult_legal_position(doublemove).piece ==
          Piece::None) { // double move
    *(m_end++) = Movement(from, doublemove, MoveType::Regular);
  }
  PiecePlacement singlemove(from.index() + offset);
  if (position.consult_legal_position(singlemove).piece == Piece::None) {
    *(m_end++) = Movement(from, singlemove, MoveType::Regular);
  }

  PiecePlacement capture_left(from.index() + offset + directions::West),
      capture_right(from.index() + offset + directions::East);
  if (!capture_left.out_of_bounds() &&
      position.consult_legal_position(capture_left).player == adversary) {
    *(m_end++) = Movement(from, capture_left, MoveType::Regular);
  }
  if (!capture_right.out_of_bounds() &&
      position.consult_legal_position(capture_right).player == adversary) {
    *(m_end++) = Movement(from, capture_right, MoveType::Regular);
  }
}

void MoveList::pseudolegal_knight_moves(const Position &position,
                                        const PiecePlacement &from) {
  for (IndexType offset : move_offsets::Knight) {
    PiecePlacement to(from.index() + offset);
    if (!to.out_of_bounds() &&
        position.consult_legal_position(to).player != position.side_to_move()) {
      *(m_end++) = Movement(from, to, MoveType::Regular);
    }
  }
}

void MoveList::pseudolegal_sliders_moves(const Position &position,
                                         const PiecePlacement &from) {
  IndexType piece_index =
      static_cast<IndexType>(position.consult_legal_position(from).piece);
  for (IndexType offset : move_offsets::Sliders[piece_index - 2]) {
    PiecePlacement to(from.index() + offset);
    while (!to.out_of_bounds()) {
      if (position.consult_legal_position(to).piece == Piece::None) {
        *(m_end++) = Movement(from, to, MoveType::Regular);
        to.index() += offset;
      } else if (position.consult_legal_position(to).player !=
                 position.side_to_move()) {
        *(m_end++) = Movement(from, to, MoveType::Capture);
        break;
      } else {
        break;
      }
    }
  }
}

void MoveList::pseudolegal_king_moves(const Position &position,
                                      const PiecePlacement &from) {
  for (IndexType offset : move_offsets::AllDirections) {
    PiecePlacement to(from.index() + offset);
    if (!to.out_of_bounds() &&
        position.consult_legal_position(to).player == position.side_to_move()) {
      *(m_end++) = Movement(from, to, MoveType::Regular);
    }
  }
}

void MoveList::sort_moves() {
  // TODO implement this
}
