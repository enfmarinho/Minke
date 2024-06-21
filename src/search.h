/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "game_elements.h"
#include "game_state.h"
#include "thread.h"

#ifndef SEARCH_H
#define SEARCH_H

namespace search {
void search(GameState &game_state, Thread &thread);
void perft(GameState &game_state, Thread &thread);
template <bool print_moves>
void iterative_deepening(GameState &game_state, Thread &thread);
WeightType alpha_beta_search(WeightType alpha, WeightType beta,
                             const CounterType &depth, GameState &game_state,
                             Thread &thread);
} // namespace search

#endif // #ifndef SEARCH_H
