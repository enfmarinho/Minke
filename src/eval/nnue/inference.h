/*
 *  Minke is a UCI chess engine
 *  Copyright (C) 2026 Eduardo Marinho <eduardomarinho@pm.me>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cassert>
#include <cstdint>
#include <span>

#include "eval/nnue/arch.h"

#ifdef TRACK_ACTIVATIONS
#include <array>
#include <cstddef>
inline std::array<size_t, nnue::PAIR_COUNT> activation_table;
#endif // TRACK_ACTIVATIONS

namespace nnue {

int32_t propagate(std::span<const int16_t, L1_SIZE> stm_inputs, std::span<const int16_t, L1_SIZE> ntm_inputs,
                  const int bucket);

}
