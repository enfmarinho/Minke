/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "search.h"
#include "evaluate.h"
#include "game_elements.h"
#include "game_state.h"
#include "movegen.h"
#include "position.h"
#include "thread.h"
#include "tt.h"
#include <cassert>
#include <iostream>

void search::search(GameState &game_state, Thread &thread) {
  iterative_deepening<true>(game_state, thread);
}
void search::perft(GameState &game_state, Thread &thread) {
  iterative_deepening<false>(game_state, thread);
}

template <bool print_moves>
void search::iterative_deepening(GameState &game_state, Thread &thread) {
  bool found;
  TTEntry *move;
  for (CounterType depth_ply = 1; !thread.should_stop(depth_ply); ++depth_ply) {
    alpha_beta_search(ScoreNone, -ScoreNone, depth_ply, game_state, thread);
    if (!thread.should_stop()) {
      move = TranspositionTable::get().probe(game_state.position(), found);
      assert(found);
      if constexpr (print_moves)
        std::cout << "depth " << depth_ply << " bestmove "
                  << move->best_move().get_algebraic_notation() << std::endl;
    }
  }
  if constexpr (print_moves)
    std::cout << "bestmove " << move->best_move().get_algebraic_notation()
              << std::endl;
}

WeightType search::alpha_beta_search(WeightType alpha, WeightType beta,
                                     const CounterType &depth_ply,
                                     GameState &game_state, Thread &thread) {
  thread.increase_nodes_searched_counter();
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
      std::cerr << "TT hit, but entry bound is empty.";
      assert(false);
    }
    if (alpha >= beta)
      return entry->evaluation();
  }

  if (thread.should_stop())
    return ScoreNone;
  if (depth_ply == 0)
    return eval::evaluate(
        game_state.position()); // TODO STUB while nn is incomplete
  // return game_state.eval(); // TODO do a quiesce search

  Move best_move = move_none;
  if (found)
    best_move = entry->best_move();
  MoveList move_list(game_state, best_move);
  while (!move_list.empty()) {
    Move move = move_list.next_move();
    if (!game_state.make_move(move))
      continue;
    WeightType score =
        alpha_beta_search(-beta, -alpha, depth_ply - 1, game_state, thread);
    game_state.undo_move();
    if (score >= beta) {
      game_state.increment_history(move, depth_ply);
      return beta;
    } else if (score > alpha)
      alpha = score, best_move = move;
  }
  if (!thread.should_stop())
    entry->save(game_state.position().get_hash(), depth_ply, best_move, alpha,
                game_state.position().get_half_move_counter(),
                TTEntry::BoundType::Empty); // TODO bound is just a STUB

  return alpha;
}
