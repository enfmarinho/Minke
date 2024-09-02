/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "movegen.h"
#include "game_elements.h"
#include "game_state.h"
#include "position.h"
#include "weights.h"
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <utility>

bool under_attack(const Position &position, const Player &attacker_player,
                  const PiecePlacement &pp_defender) {
  PiecePlacement trash;
  return cheapest_attacker(position, attacker_player, pp_defender, trash) !=
         Piece::None;
}

MoveList::MoveList(const GameState &game_state, const Move &move) {
  gen_pseudolegal_moves(game_state.top());
  calculate_scores(game_state, move);
}

MoveList::MoveList(const Position &position) {
  gen_pseudolegal_moves(position);
}

void MoveList::gen_pseudolegal_moves(const Position &position) {
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
}

[[nodiscard]] bool MoveList::empty() const { return m_end == m_begin; }

[[nodiscard]] MoveList::size_type MoveList::size() const {
  return m_end - m_begin;
}

[[nodiscard]] MoveList::size_type
MoveList::n_legal_moves(Position position) const {
  size_type counter = 0;
  for (uint8_t index = m_begin; index < size(); ++index) {
    Position cp_position = position;
    if (cp_position.move(m_move_list[index])) {
      ++counter;
    }
  }
  return counter;
}

[[nodiscard]] Move MoveList::next_move() {
  size_type best_move_index = m_begin;
  WeightType best_score = m_move_scores[best_move_index];
  for (size_type index = 0, remaining = size(); index < remaining; ++index) {
    if (best_score < m_move_scores[m_begin + index]) {
      best_move_index = m_begin + index;
      best_score = m_move_scores[best_move_index];
    }
  }
  std::swap(m_move_list[m_begin], m_move_list[best_move_index]);
  std::swap(m_move_scores[m_begin], m_move_scores[best_move_index]);
  return m_move_list[m_begin++];
}

void MoveList::pseudolegal_pawn_moves(const Position &position,
                                      const PiecePlacement &from) {
  IndexType original_file;
  IndexType promotion_file;
  IndexType en_passant_file;
  IndexType offset;
  Player adversary;
  if (position.side_to_move() == Player::White) {
    original_file = 1;
    promotion_file = 7;
    en_passant_file = 5;
    offset = offsets::North;
    adversary = Player::Black;
  } else {
    original_file = 6;
    promotion_file = 0;
    en_passant_file = 2;
    offset = offsets::South;
    adversary = Player::White;
  }

  PiecePlacement capture_left(from.index() + offset + offsets::West);
  if (!capture_left.out_of_bounds() &&
      position.consult(capture_left).player == adversary) { // left capture
    if (capture_left.file() == promotion_file) {
      m_move_list[m_end++] =
          Move(from, capture_left, MoveType::PawnPromotionQueen);
      m_move_list[m_end++] =
          Move(from, capture_left, MoveType::PawnPromotionRook);
      m_move_list[m_end++] =
          Move(from, capture_left, MoveType::PawnPromotionBishop);
      m_move_list[m_end++] =
          Move(from, capture_left, MoveType::PawnPromotionKnight);
    } else {
      m_move_list[m_end++] = Move(from, capture_left, MoveType::Capture);
    }
  } else if (!capture_left.out_of_bounds() &&
             position.en_passant_rank() == capture_left.rank() &&
             capture_left.file() == en_passant_file) {
    m_move_list[m_end++] = Move(from, capture_left, MoveType::EnPassant);
  }

  PiecePlacement capture_right(from.index() + offset + offsets::East);
  if (!capture_right.out_of_bounds() &&
      position.consult(capture_right).player == adversary) { // right capture
    if (capture_right.file() == promotion_file) {
      m_move_list[m_end++] =
          Move(from, capture_right, MoveType::PawnPromotionQueen);
      m_move_list[m_end++] =
          Move(from, capture_right, MoveType::PawnPromotionRook);
      m_move_list[m_end++] =
          Move(from, capture_right, MoveType::PawnPromotionBishop);
      m_move_list[m_end++] =
          Move(from, capture_right, MoveType::PawnPromotionKnight);
    } else {
      m_move_list[m_end++] = Move(from, capture_right, MoveType::Capture);
    }
  } else if (!capture_right.out_of_bounds() &&
             position.en_passant_rank() == capture_right.rank() &&
             capture_right.file() == en_passant_file) {
    m_move_list[m_end++] = Move(from, capture_right, MoveType::EnPassant);
  }

  PiecePlacement singlemove(from.index() + offset);
  if (position.consult(singlemove).piece == Piece::None) {
    if (singlemove.file() == promotion_file) {
      m_move_list[m_end++] =
          Move(from, singlemove, MoveType::PawnPromotionQueen);
      m_move_list[m_end++] =
          Move(from, singlemove, MoveType::PawnPromotionRook);
      m_move_list[m_end++] =
          Move(from, singlemove, MoveType::PawnPromotionBishop);
      m_move_list[m_end++] =
          Move(from, singlemove, MoveType::PawnPromotionKnight);
    } else {
      m_move_list[m_end++] = Move(from, singlemove, MoveType::Regular);
    }

    PiecePlacement doublemove(from.index() + 2 * offset);
    if (from.file() == original_file &&
        position.consult(doublemove).piece == Piece::None) {
      m_move_list[m_end++] = Move(from, doublemove, MoveType::Regular);
    }
  }
}

void MoveList::pseudolegal_knight_moves(const Position &position,
                                        const PiecePlacement &from) {
  for (IndexType offset : offsets::Knight) {
    PiecePlacement to(from.index() + offset);
    if (!to.out_of_bounds()) {
      if (position.consult(to).piece == Piece::None) {
        m_move_list[m_end++] = Move(from, to, MoveType::Regular);
      } else if (position.consult(to).player != position.side_to_move()) {
        m_move_list[m_end++] = Move(from, to, MoveType::Regular);
      }
    }
  }
}

void MoveList::pseudolegal_sliders_moves(const Position &position,
                                         const PiecePlacement &from) {
  IndexType piece_index = piece_index(position.consult(from).piece);
  for (IndexType offset : offsets::Sliders[piece_index - 2]) {
    PiecePlacement to(from.index() + offset);
    while (!to.out_of_bounds()) {
      if (position.consult(to).piece == Piece::None) {
        m_move_list[m_end++] = Move(from, to, MoveType::Regular);
        to.index() += offset;
      } else if (position.consult(to).player != position.side_to_move()) {
        m_move_list[m_end++] = Move(from, to, MoveType::Capture);
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
        m_move_list[m_end++] = Move(from, to, MoveType::Regular);
      } else if (to_player != position.side_to_move()) {
        m_move_list[m_end++] = Move(from, to, MoveType::Capture);
      }
    }
  }
}

void MoveList::pseudolegal_castling_moves(const Position &position) {
  CastlingRights castling_rights;
  PiecePlacement king_placement;
  IndexType player_perspective_first_file;
  Player adversary;
  if (position.side_to_move() == Player::White) {
    castling_rights = position.white_castling_rights();
    king_placement = position.white_king_position();
    player_perspective_first_file = 0;
    adversary = Player::Black;
  } else {
    castling_rights = position.black_castling_rights();
    king_placement = position.black_king_position();
    player_perspective_first_file = 7;
    adversary = Player::White;
  }

  if (under_attack(position, adversary, king_placement)) {
    return;
  }

  if (castling_rights.king_side &&
      position.consult(player_perspective_first_file, 5).piece == Piece::None &&
      position.consult(player_perspective_first_file, 6).piece == Piece::None &&
      position.consult(player_perspective_first_file, 7) ==
          Square(Piece::Rook, position.side_to_move()) &&
      !under_attack(position, adversary,
                    PiecePlacement(player_perspective_first_file, 5)) &&
      !under_attack(position, adversary,
                    PiecePlacement(player_perspective_first_file, 6)))
    m_move_list[m_end++] =
        Move(king_placement, PiecePlacement(player_perspective_first_file, 6),
             MoveType::KingSideCastling);

  if (castling_rights.queen_side &&
      position.consult(player_perspective_first_file, 1).piece == Piece::None &&
      position.consult(player_perspective_first_file, 2).piece == Piece::None &&
      position.consult(player_perspective_first_file, 3).piece == Piece::None &&
      position.consult(player_perspective_first_file, 0) ==
          Square(Piece::Rook, position.side_to_move()) &&
      !under_attack(position, adversary,
                    PiecePlacement(player_perspective_first_file, 2)) &&
      !under_attack(position, adversary,
                    PiecePlacement(player_perspective_first_file, 3)))
    m_move_list[m_end++] =
        Move(king_placement, PiecePlacement(player_perspective_first_file, 2),
             MoveType::QueenSideCastling);
}

Piece cheapest_attacker(const Position &position, const Player &attacker_player,
                        const PiecePlacement &pp_defender,
                        PiecePlacement &pp_atacker) {
  IndexType opponent_pawn_offset;
  if (attacker_player == Player::White)
    opponent_pawn_offset = offsets::North;
  else
    opponent_pawn_offset = offsets::South;
  assert(attacker_player != Player::None);

  auto under_pawn_thread = [&position, &attacker_player](
                               const PiecePlacement &pawn_atacker_pos) -> bool {
    if (!pawn_atacker_pos.out_of_bounds()) {
      const Square &sq = position.consult(pawn_atacker_pos);
      if (sq.player == attacker_player && sq.piece == Piece::Pawn)
        return true;
    }
    return false;
  };

  PiecePlacement east_pawn_attacker(pp_defender.index() - opponent_pawn_offset +
                                    offsets::East);
  if (under_pawn_thread(east_pawn_attacker))
    return pp_atacker = east_pawn_attacker, Piece::Pawn;

  PiecePlacement west_pawn_attacker(pp_defender.index() - opponent_pawn_offset +
                                    offsets::West);
  if (under_pawn_thread(west_pawn_attacker))
    return pp_atacker = west_pawn_attacker, Piece::Pawn;

  for (const IndexType &offset : offsets::Knight) {
    PiecePlacement to(pp_defender.index() + offset);
    if (!to.out_of_bounds()) {
      const Square &to_square = position.consult(to);
      if (to_square.player == attacker_player &&
          to_square.piece == Piece::Knight)
        return pp_atacker = to, Piece::Knight;
    }
  }

  bool queen_attacks = false;
  for (const IndexType &offset : offsets::Sliders[0]) { // Bishop directions
    if (offset == 0)
      break;

    for (PiecePlacement to(pp_defender.index() + offset); !to.out_of_bounds();
         to.index() += offset) {
      const Square &to_square = position.consult(to);
      if (to_square.player == attacker_player) {
        if (to_square.piece == Piece::Bishop)
          return pp_atacker = to, Piece::Bishop;
        if (to_square.piece == Piece::Queen) {
          queen_attacks = true;
          pp_atacker = to;
          break;
        }
      }
      if (to_square.player != Player::None)
        break;
    }
  }
  for (const IndexType &offset : offsets::Sliders[1]) { // Rook directions
    if (offset == 0)
      break;

    for (PiecePlacement to(pp_defender.index() + offset); !to.out_of_bounds();
         to.index() += offset) {
      const Square &to_square = position.consult(to);
      if (to_square.player == attacker_player) {
        if (to_square.piece == Piece::Rook)
          return pp_atacker = to, Piece::Rook;
        if (to_square.piece == Piece::Queen) {
          pp_atacker = to;
          queen_attacks = true;
          break;
        }
      }
      if (to_square.player != Player::None)
        break;
    }
  }

  if (queen_attacks)
    return Piece::Queen;

  for (const IndexType &offset : offsets::AllDirections) {
    PiecePlacement to(pp_defender.index() + offset);
    if (!to.out_of_bounds()) {
      const Square &to_square = position.consult(to);
      if (to_square.player == attacker_player && to_square.piece == Piece::King)
        return pp_atacker = to, Piece::King;
    }
  }

  return Piece::None;
}

bool SEE(const Position &position, const Move &move) {
  Player player = position.side_to_move(),
         opponent = player == Player::White ? Player::Black : Player::White;
  Position copy_position = position;
  WeightType material_gain =
      weights::SEE_table[piece_index(copy_position.consult(move.to).piece)];
  WeightType material_risk =
      weights::SEE_table[piece_index(copy_position.consult(move.from).piece)];
  copy_position.consult(move.from) = EmptySquare;

  while (material_gain <= material_risk) {
    PiecePlacement pp_atacker;
    Piece attacker =
        cheapest_attacker(copy_position, opponent, move.to, pp_atacker);
    if (attacker == Piece::None)
      return true;

    copy_position.consult(pp_atacker) = EmptySquare;
    material_gain -= material_risk;
    material_risk = weights::SEE_table[piece_index(attacker)];

    if (material_gain < -material_risk)
      return false;

    attacker = cheapest_attacker(copy_position, player, move.to, pp_atacker);
    if (attacker == Piece::None)
      return false;

    copy_position.consult(pp_atacker) = EmptySquare;
    material_gain += material_risk;
    material_risk = weights::SEE_table[piece_index(attacker)];
  }

  return true;
}

void MoveList::calculate_scores(const GameState &game_state,
                                const Move &tt_move) {
  static constexpr WeightType TTHitScore = 70000;
  static constexpr WeightType QueenPromotionScore = 50000;
  static constexpr WeightType CaptureScore = 20000;

  for (size_type index = 0, remaining = size(); index < remaining; ++index) {
    Move move = m_move_list[index];
    assert(move != MoveNone);
    if (move == tt_move) {
      m_move_scores[index] = TTHitScore;
    } else if (move.move_type == MoveType::Capture) {
      m_move_scores[index] = CaptureScore +
                             5 * weights::SEE_table[piece_index(
                                     game_state.top().consult(move.to).piece)] -
                             weights::SEE_table[piece_index(
                                 game_state.top().consult(move.from).piece)] /
                                 5 -
                             TTHitScore * !SEE(game_state.top(), move);
    } else if (move.move_type == MoveType::PawnPromotionQueen) {
      m_move_scores[index] = QueenPromotionScore;
    } else {
      m_move_scores[index] = game_state.consult_history(move);
    }
  }
}

void MoveList::ignore_non_quiet_moves() {
  for (size_type index = 0, remaining = size(); index < remaining; ++index) {
    Move curr_move = m_move_list[index];
    if (curr_move.move_type == MoveType::Capture) {
      break;
    }
  }
}
