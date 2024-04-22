/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#include "game_elements.h"
#include "position.h"

#ifndef MOVE_GENERATION_H
#define MOVE_GENERATION_H

namespace move_generation {
Movement progressive_deepening(Position &position);
Movement minimax(Position &position, const IndexType &depth);
} // namespace move_generation

#endif // #ifndef MOVE_GENERATION_H
