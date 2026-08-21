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

#include <array>
#include <cstddef>

#include "core/move.h"
#include "core/types.h"

namespace Cuckoo {

extern std::array<HashType, 8192> keys;
extern std::array<Move, 8192> moves;

constexpr size_t h1(HashType hash) { return static_cast<size_t>(hash & 0x1FFF); }

constexpr size_t h2(HashType hash) { return static_cast<size_t>((hash >> 16) & 0x1FFF); }

void init();

}; // namespace Cuckoo
