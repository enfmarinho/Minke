/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "search.h"
#include "evaluate.h"
#include "game_elements.h"
#include "movegen.h"
#include "position.h"
#include "thread.h"
#include "tt.h"
#include <cassert>
#include <iostream>

void search::iterative_deepening(Position &position, Thread &thread) {
  bool found;
  TTEntry *move;
  for (CounterType depth_ply = 1; !thread.should_stop(depth_ply); ++depth_ply) {
    alpha_beta_search(ScoreNone, -ScoreNone, depth_ply, position, thread);
    if (!thread.should_stop()) {
      move = TranspositionTable::get().probe(position, found);
      assert(found);
      std::cout << "depth " << depth_ply << " bestmove "
                << move->best_move().get_algebraic_notation() << std::endl;
    }
  }
  std::cout << "bestmove " << move->best_move().get_algebraic_notation()
            << std::endl;
}

WeightType search::alpha_beta_search(WeightType alpha, WeightType beta,
                                     const CounterType &depth_ply,
                                     Position &position, Thread &thread) {
  thread.increase_nodes_searched_counter();
  bool found;
  TTEntry *entry = TranspositionTable::get().probe(position, found);
  if (found && entry->depth_ply() >= depth_ply)
    return entry->evaluation();
  if (thread.should_stop())
    return ScoreNone;
  if (depth_ply == 0)
    return eval::evaluate(position); // TODO do a quiesce search

  Move best_move;
  MoveList move_list(position);
  while (!move_list.empty()) {
    Move move = move_list.next_move();
    if (!position.move(move))
      continue;
    WeightType score =
        alpha_beta_search(-beta, -alpha, depth_ply - 1, position, thread);
    position.undo_move();
    if (score >= beta) {
      position.increment_history(move, depth_ply);
      return beta;
    } else if (score > alpha)
      alpha = score, best_move = move;
  }
  if (!thread.should_stop()) {
    entry->save(
        position.get_hash(), depth_ply, best_move, alpha,
        position.get_half_move_counter(),
        TTEntry::BoundType::Empty); // TODO bound is just a STUB, needs to
                                    // be change, i.e. IT'S NOT CORRECT
  }
  return alpha;
}
