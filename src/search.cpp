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

void search::perft(GameState &game_state, Thread &thread) {
  iterative_deepening<false>(game_state, thread);
}

void search::search(GameState &game_state, Thread &thread) {
  iterative_deepening<true>(game_state, thread);
}

static void print_search_info(const CounterType &depth, const WeightType &eval,
                              const PvList &pv_list, const Thread &thread) {
  std::cout << "info depth " << depth << " score cp " << eval << " nodes "
            << thread.nodes_searched() << " time " << thread.time_passed()
            << " pv ";
  pv_list.print();
  std::cout << std::endl;
}

template <bool print_moves>
void search::iterative_deepening(GameState &game_state, Thread &thread) {
  bool found;
  PvList pv_list;
  WeightType eval =
      alpha_beta(ScoreNone, -ScoreNone, 1, game_state, thread, pv_list);
  Move best_move =
      TranspositionTable::get().probe(game_state.top(), found)->best_move();
  if (best_move == MoveNone) {
    if constexpr (print_moves)
      std::cout << "bestmove none\n";
    return;
  }
  print_search_info(1, eval, pv_list, thread);

  for (CounterType depth = 2; !thread.should_stop(depth); ++depth) {
    eval =
        alpha_beta(ScoreNone, -ScoreNone, depth, game_state, thread, pv_list);

    if (!thread.should_stop()) { // Search was successful
      TTEntry *entry = TranspositionTable::get().probe(game_state.top(), found);
      assert(found && entry->depth_ply() >= depth);

      best_move = entry->best_move();
      if constexpr (print_moves) {
        print_search_info(depth, eval, pv_list, thread);
      }
    }
  }
  if constexpr (print_moves) {
    std::cout << "bestmove " << best_move.get_algebraic_notation() << std::endl;
  }
}

TTEntry *search::aspiration(const CounterType &depth, GameState &game_state,
                            Thread &thread, PvList &pv_list) {
  bool found;
  TTEntry *ttentry = TranspositionTable::get().probe(game_state.top(), found);
  if (!found) {
    // alpha_beta(ScoreNone, -ScoreNone, depth, game_state, thread, pv_list);
    return ttentry;
  }

  // WeightType alpha = ttentry->evaluation() - weights::EndGamePawn;
  // WeightType beta = ttentry->evaluation() + weights::EndGamePawn;
  // // TODO check alpha and beta arguments
  // WeightType eval = alpha_beta(alpha, beta, depth, game_state, thread,
  // pv_list); if (eval >= beta) {
  //   alpha_beta(alpha, -ScoreNone, depth, game_state, thread, pv_list);
  // } else if (eval <= alpha) {
  //   alpha_beta(ScoreNone, beta, depth, game_state, thread, pv_list);
  // }
  // assert(eval >= alpha && eval <= beta);

  return ttentry;
}

WeightType search::quiescence(WeightType alpha, WeightType beta,
                              GameState &game_state, Thread &thread) {
  thread.increase_nodes_searched_counter();
  return game_state.eval();
  if (thread.should_stop()) {
    return ScoreNone;
  }

  WeightType stand_pat = game_state.eval();
  if (stand_pat >= beta) {
    return beta;
  }

  // TODO Probably worth to turn off on end game
  WeightType delta = weights::MidGameQueen + 200;
  if (game_state.last_move().move_type == MoveType::PawnPromotionQueen) {
    delta += weights::MidGameQueen;
  }
  if (stand_pat < alpha - delta) { // Delta pruning
    return alpha;
  } else if (alpha < stand_pat) {
    alpha = stand_pat;
  }

  MoveList move_list(game_state, MoveNone);
  move_list.ignore_non_quiet_moves();
  while (!move_list.empty()) {
    Move curr_move = move_list.next_move();
    if (curr_move.move_type != MoveType::Capture) {
      break;
    } else if (!game_state.make_move(curr_move)) {
      continue;
    }

    WeightType eval = -quiescence(-beta, -alpha, game_state, thread);
    game_state.undo_move();
    if (eval >= beta) {
      return beta;
    } else if (eval > alpha) {
      alpha = eval;
    }
  }

  return alpha;
}

WeightType search::alpha_beta(WeightType alpha, WeightType beta,
                              const CounterType &depth_ply,
                              GameState &game_state, Thread &thread,
                              PvList &pv_list) {

  if (thread.should_stop()) { // Out of time
    return ScoreNone;
  } else if (depth_ply == 0) {
    return quiescence(-beta, -alpha, game_state, thread);
  }

  thread.increase_nodes_searched_counter();

  // Transposition table probe
  bool found;
  TTEntry *entry = TranspositionTable::get().probe(game_state.top(), found);
  if (found && entry->depth_ply() >= depth_ply) {
    switch (entry->bound()) {
    case TTEntry::BoundType::Exact:
      pv_list.update(entry->best_move(), PvList());
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

    if (alpha >= beta) {
      pv_list.update(entry->best_move(), PvList());
      return entry->evaluation();
    }
  }

  // TODO check for draw

  Move best_move = MoveNone;
  WeightType best_score = ScoreNone;
  // MoveList move_list(game_state, (found ? entry->best_move() : MoveNone));
  MoveList move_list(game_state, (MoveNone));
  while (!move_list.empty()) {
    PvList curr_pv;
    Move move = move_list.next_move();
    if (!game_state.make_move(move)) { // Avoid illegal moves
      continue;
    }

    WeightType eval =
        alpha_beta(-beta, -alpha, depth_ply - 1, game_state, thread, curr_pv);
    game_state.undo_move();
    assert(eval >= ScoreNone);

    if (eval > best_score) {
      best_score = eval;
      best_move = move;
      pv_list.update(best_move, curr_pv);
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
    Position &pos = game_state.top();
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
    if (best_score <= alpha) {
      bound = TTEntry::BoundType::LowerBound;
    } else if (best_score >= beta) {
      bound = TTEntry::BoundType::UpperBound;
    }
    entry->save(game_state.top().get_hash(), depth_ply, best_move, best_score,
                game_state.top().get_half_move_counter(), bound);
  }

  return alpha;
}
