/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "search.h"
#include "game_elements.h"
#include "game_state.h"
#include "movegen.h"
#include "position.h"
#include "thread.h"
#include "tt.h"
#include "weights.h"
#include <algorithm>
#include <cassert>
#include <iostream>

void search::search(GameState &game_state, Thread &thread) {
  iterative_deepening<true>(game_state, thread);
}

void search::perft(GameState &game_state, Thread &thread) {
  iterative_deepening<false>(game_state, thread);
}

static void print_pv(Position pos, int depth_ply) {
  for (int i = 0; i < depth_ply; ++i) {
    bool found;
    TTEntry *entry = TranspositionTable::get().probe(pos, found);
    assert(found);
    pos.move(entry->best_move());
    std::cout << entry->best_move().get_algebraic_notation() << ' ';
  }
}

template <bool print_moves>
void search::iterative_deepening(GameState &game_state, Thread &thread) {
  TTEntry *entry;
  for (CounterType depth = 1; !thread.should_stop(depth); ++depth) {
    entry = aspiration(depth, game_state, thread);
    if (!thread.should_stop()) { // Search was successful
      if constexpr (print_moves) {
        std::cout << "info depth " << depth << " score cp "
                  << entry->evaluation() << " nodes " << thread.nodes_searched()
                  << " time " << thread.time_passed() << " pv ";
        print_pv(game_state.position(), depth);
        std::cout << std::endl;
      }
    }
  }
  if constexpr (print_moves)
    std::cout << "bestmove " << entry->best_move().get_algebraic_notation()
              << std::endl;
}

TTEntry *search::aspiration(const CounterType &depth, GameState &game_state,
                            Thread &thread) {
  bool found;
  TTEntry *ttentry =
      TranspositionTable::get().probe(game_state.position(), found);
  if (!found) {
    alpha_beta(ScoreNone, -ScoreNone, depth, game_state, thread);
    return ttentry;
  }

  WeightType alpha = ttentry->evaluation() - weights::EndGamePawn;
  WeightType beta = ttentry->evaluation() + weights::EndGamePawn;
  WeightType eval = alpha_beta(alpha, beta, depth, game_state, thread);
  if (eval >= beta)
    alpha_beta(alpha, -ScoreNone, depth, game_state, thread);
  else if (eval <= alpha)
    alpha_beta(ScoreNone, beta, depth, game_state, thread);
  return ttentry;
}

WeightType search::quiescence(GameState &game_state) {
  return game_state.eval();
}

WeightType search::alpha_beta(WeightType alpha, WeightType beta,
                              const CounterType &depth_ply,
                              GameState &game_state, Thread &thread) {
  if (thread.should_stop()) // Out of time
    return ScoreNone;

  if (depth_ply == 0)
    return quiescence(alpha, beta, game_state, thread);

  thread.increase_nodes_searched_counter();

  // Transposition table probe
  bool found;
  TTEntry *entry =
      TranspositionTable::get().probe(game_state.position(), found);
  if (found && entry->depth_ply() >= depth_ply) {
    switch (entry->bound()) {
    case TTEntry::BoundType::Exact:
      return entry->evaluation();
    case TTEntry::BoundType::LowerBound:
      alpha = std::max(entry->evaluation(), alpha);
      break;
    case TTEntry::BoundType::UpperBound:
      beta = std::min(entry->evaluation(), beta);
      break;
    default:
      std::cerr << "TT hit, but entry bound is empty." << std::endl;
      assert(false);
    }
    if (alpha >= beta)
      return entry->evaluation();
  }
  Move best_move = MoveNone;
  WeightType best_score = ScoreNone;
  if (found) {
    assert(entry->best_move() != MoveNone);
    best_move = entry->best_move();
    best_score = entry->evaluation();
  }

  // TODO check for draw

  MoveList move_list(game_state, best_move);
  while (!move_list.empty()) {
    Move move = move_list.next_move();
    if (!game_state.make_move(move)) // Avoid illegal moves
      continue;
    WeightType eval =
        alpha_beta(-beta, -alpha, depth_ply - 1, game_state, thread);
    assert(eval >= ScoreNone);
    game_state.undo_move();

    if (eval >= best_score) {
      best_score = eval;
      best_move = move;
    }

    if (eval >= beta) {
      game_state.increment_history(move, depth_ply);
      return alpha;
    } else if (eval > alpha) {
      alpha = eval;
    }
  }

  if (best_move == MoveNone) { // handle positions under stalemate or checkmate,
                               // i.e. positions with no legal moves to be made
    Position &pos = game_state.position();
    Player adversary;
    PiecePlacement king_placement;
    if (pos.side_to_move() == Player::White) {
      adversary = Player::Black;
      king_placement = pos.white_king_position();
    } else {
      adversary = Player::White;
      king_placement = pos.black_king_position();
    }
    return under_attack(pos, adversary, king_placement) ? -MateScore + depth_ply
                                                        : 0;
  }

  if (!thread.should_stop()) { // Save on TT if search was completed
    TTEntry::BoundType bound = TTEntry::BoundType::Exact;
    if (best_score <= alpha)
      bound = TTEntry::BoundType::LowerBound;
    else if (best_score >= beta)
      bound = TTEntry::BoundType::UpperBound;

    entry->save(game_state.position().get_hash(), depth_ply, best_move,
                best_score, game_state.position().get_half_move_counter(),
                bound);
  }
  return alpha;
}
