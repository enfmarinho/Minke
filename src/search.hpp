/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "game_elements.hpp"
#include "position.hpp"
#include "thread.hpp"

#ifndef SEARCH_HPP
#define SEARCH_HPP

namespace search {
void iterative_deepening(Position &position, Thread &thread);
WeightType alpha_beta_search(WeightType alpha, WeightType beta,
                             const CounterType &depth, const Position &position,
                             Thread &thread);
} // namespace search

#endif // #ifndef SEARCH_HPP
