/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "game_elements.hpp"
#include "position.hpp"
#include "thread.hpp"

#ifndef MOVE_GENERATION_HPP
#define MOVE_GENERATION_HPP

namespace move_generation {
Movement progressive_deepening(Position &position, Thread &thread);
Movement minimax(Position &position, const IndexType &depth);
} // namespace move_generation

#endif // #ifndef MOVE_GENERATION_HPP
