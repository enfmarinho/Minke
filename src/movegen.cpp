/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "movegen.h"
#include "game_elements.h"
#include "position.h"
#include <cassert>
#include <utility>

bool under_attack(const Position &position, const PiecePlacement &pp,
                  const Player &player) {
  Player opponent;
  IndexType opponent_pawn_offset;
  if (player == Player::Black) {
    opponent = Player::White;
    opponent_pawn_offset = offsets::North;
  } else if (player == Player::White) {
    opponent = Player::Black;
    opponent_pawn_offset = offsets::South;
  } else {
    assert(false);
  }

  auto under_pawn_thread = [](const Position &position,
                              const PiecePlacement &pawn_atacker_pos,
                              const Player &opponent) -> bool {
    if (!pawn_atacker_pos.out_of_bounds()) {
      const Square &sq = position.consult(pawn_atacker_pos);
      if (sq.player == opponent && sq.piece == Piece::Pawn)
        return true;
    }
    return false;
  };
  PiecePlacement east_pawn_attacker(pp.index() + opponent_pawn_offset +
                                    offsets::East);
  PiecePlacement west_pawn_attacker(pp.index() + opponent_pawn_offset +
                                    offsets::West);
  if (under_pawn_thread(position, east_pawn_attacker, opponent) ||
      under_pawn_thread(position, west_pawn_attacker, opponent))
    return true;

  for (const IndexType &offset : offsets::Knight) {
    PiecePlacement to(pp.index() + offset);
    if (!to.out_of_bounds()) {
      const Square &to_square = position.consult(to);
      if (to_square.player == opponent && to_square.piece == Piece::Knight)
        return true;
    }
  }
  for (const IndexType &offset : offsets::Sliders[0]) { // Bishop directions
    if (offset == 0) {
      break;
    }
    for (PiecePlacement to(pp.index() + offset); !to.out_of_bounds();
         to.index() += offset) {
      const Square &to_square = position.consult(to);
      if (to_square.player == opponent &&
          (to_square.piece == Piece::Bishop || to_square.piece == Piece::Queen))
        return true;
      if (to_square.player != Player::None)
        break;
    }
  }
  for (const IndexType &offset : offsets::Sliders[1]) { // Rook directions
    if (offset == 0) {
      break;
    }
    for (PiecePlacement to(pp.index() + offset); !to.out_of_bounds();
         to.index() += offset) {
      const Square &to_square = position.consult(to);
      if (to_square.player == opponent &&
          (to_square.piece == Piece::Rook || to_square.piece == Piece::Queen))
        return true;
      if (to_square.player != Player::None)
        break;
    }
  }
  return false;
}

MoveList::MoveList(const Position &position) : m_start_index{0} {
  m_end = m_move_list;
  for (IndexType file = 0; file < BoardHeight; ++file) {
    for (IndexType rank = 0; rank < BoardWidth; ++rank) {
      PiecePlacement current = PiecePlacement(file, rank);
      const Square &square = position.consult(current);
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

  pseudolegal_castling_moves(position);
  calculate_see();
}

[[nodiscard]] MoveList::size_type MoveList::remaining_moves() const {
  return m_end - (m_move_list + m_start_index);
}

[[nodiscard]] Move MoveList::next_move() {
  size_type best_move_index = m_start_index;
  WeightType best_score = m_see[best_move_index];
  for (size_type index = 0, remaining = remaining_moves(); index < remaining;
       ++index) {
    if (best_score < m_see[m_start_index + index]) {
      best_move_index = m_start_index + index;
      best_score = m_see[best_move_index];
    }
  }
  std::swap(m_move_list[m_start_index], m_move_list[best_move_index]);
  std::swap(m_see[m_start_index], m_see[best_move_index]);
  return m_move_list[m_start_index++];
}

void MoveList::pseudolegal_pawn_moves(const Position &position,
                                      const PiecePlacement &from) {
  IndexType original_file;
  IndexType offset;
  Player adversary;
  if (position.side_to_move() == Player::White) {
    original_file = 1;
    offset = offsets::North;
    adversary = Player::Black;
  } else {
    original_file = 6;
    offset = offsets::South;
    adversary = Player::White;
  }
  PiecePlacement doublemove(from.index() + 2 * offset);
  if (from.file() == original_file &&
      position.consult(doublemove).piece == Piece::None) { // double move
    *(m_end++) = Move(from, doublemove, MoveType::Regular);
  }
  PiecePlacement singlemove(from.index() + offset);
  if (position.consult(singlemove).piece == Piece::None) {
    *(m_end++) = Move(from, singlemove, MoveType::Regular);
  }

  PiecePlacement capture_left(from.index() + offset + offsets::West);
  if (!capture_left.out_of_bounds() &&
      position.consult(capture_left).player == adversary) {
    *(m_end++) = Move(from, capture_left, MoveType::Capture);
  }
  PiecePlacement capture_right(from.index() + offset + offsets::East);
  if (!capture_right.out_of_bounds() &&
      position.consult(capture_right).player == adversary) {
    *(m_end++) = Move(from, capture_right, MoveType::Capture);
  }
}

void MoveList::pseudolegal_knight_moves(const Position &position,
                                        const PiecePlacement &from) {
  for (IndexType offset : offsets::Knight) {
    PiecePlacement to(from.index() + offset);
    if (!to.out_of_bounds() &&
        position.consult(to).player != position.side_to_move()) {
      *(m_end++) = Move(from, to, MoveType::Regular);
    }
  }
}

void MoveList::pseudolegal_sliders_moves(const Position &position,
                                         const PiecePlacement &from) {
  IndexType piece_index = static_cast<IndexType>(position.consult(from).piece);
  for (IndexType offset : offsets::Sliders[piece_index - 2]) {
    PiecePlacement to(from.index() + offset);
    while (!to.out_of_bounds()) {
      if (position.consult(to).piece == Piece::None) {
        *(m_end++) = Move(from, to, MoveType::Regular);
        to.index() += offset;
      } else if (position.consult(to).player != position.side_to_move()) {
        *(m_end++) = Move(from, to, MoveType::Capture);
        break;
      } else {
        break;
      }
    }
  }
}

void MoveList::pseudolegal_king_moves(const Position &position,
                                      const PiecePlacement &from) {
  for (IndexType offset : offsets::AllDirections) {
    PiecePlacement to(from.index() + offset);
    if (!to.out_of_bounds()) {
      const Player &to_player = position.consult(to).player;
      if (to_player == Player::None) {
        *(m_end++) = Move(from, to, MoveType::Regular);
      } else if (to_player != position.side_to_move()) {
        *(m_end++) = Move(from, to, MoveType::Capture);
      }
    }
  }
}

void MoveList::pseudolegal_castling_moves(const Position &position) {
  CastlingRights castling_rights;
  PiecePlacement king_placement;
  IndexType player_perspective_first_file;
  if (position.side_to_move() == Player::White) {
    castling_rights = position.white_castling_rights();
    king_placement = position.white_king_position();
    player_perspective_first_file = 0;
  } else {
    castling_rights = position.black_castling_rights();
    king_placement = position.black_king_position();
    player_perspective_first_file = 7;
  }

  if (under_attack(position, king_placement, position.side_to_move()))
    return;

  if (castling_rights.king_side &&
      position.consult(player_perspective_first_file, 5).piece == Piece::None &&
      position.consult(player_perspective_first_file, 6).piece == Piece::None)
    *(m_end++) =
        Move(king_placement, PiecePlacement(player_perspective_first_file, 6),
             MoveType::KingSideCastling);

  if (castling_rights.queen_side &&
      position.consult(player_perspective_first_file, 1).piece == Piece::None &&
      position.consult(player_perspective_first_file, 2).piece == Piece::None &&
      position.consult(player_perspective_first_file, 3).piece == Piece::None)
    *(m_end++) =
        Move(king_placement, PiecePlacement(player_perspective_first_file, 2),
             MoveType::QueenSideCastling);
}

void MoveList::calculate_see() {
  // TODO implement this
}
