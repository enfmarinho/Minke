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

#include "pov_accumulator.h"

#include <cstddef>
#include <cstdint>

template <size_t NumAdds, size_t NumSubs>
void PovAccumulator::update(const PovAccumulator &input, const std::array<size_t, NumAdds> &adds,
                            const std::array<size_t, NumSubs> &subs) {
    for (int column = 0; column < HIDDEN_LAYER_SIZE; ++column) {
        int16_t sum = input.m_neurons[column];

        for (size_t add_idx : adds) {
            sum += network.hidden_weights[add_idx + column];
        }
        for (size_t sub_idx : subs) {
            sum -= network.hidden_weights[sub_idx + column];
        }

        m_neurons[column] = sum;
    }
}

// add-sub
template void PovAccumulator::update(const PovAccumulator &input, const std::array<size_t, 1> &adds,
                                     const std::array<size_t, 1> &subs);
// add-sub2
template void PovAccumulator::update(const PovAccumulator &input, const std::array<size_t, 1> &adds,
                                     const std::array<size_t, 2> &subs);
// add2-sub2
template void PovAccumulator::update(const PovAccumulator &input, const std::array<size_t, 2> &adds,
                                     const std::array<size_t, 2> &subs);

template <size_t NumAdds, size_t NumSubs>
void PovAccumulator::self_update(const std::array<size_t, NumAdds> &adds, const std::array<size_t, NumSubs> &subs) {
    update(*this, adds, subs);
}

// add
template void PovAccumulator::self_update(const std::array<size_t, 1> &adds, const std::array<size_t, 0> &subs);
