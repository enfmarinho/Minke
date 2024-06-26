/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef EVALUATE_H
#define EVALUATE_H

#include "game_elements.h"
#include "position.h"

namespace eval {
WeightType evaluate(const Position &position);
}; // namespace eval

#endif // #ifndef EVALUATE_H
