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
#include <cstring>

#include "../../position.h"
#include "arch.h"

PovAccumulator::PovAccumulator(const Position &pos, const Color pov) {
    // Debug-only constructor. Computes a PovAccumulator from scratch and uses it as a
    // source of truth to validate incremental updates and finny tables.
    reset();
    for (int sqi = a1; sqi <= h8; ++sqi) {
        const Square sq = static_cast<Square>(sqi);
        Piece piece = pos.consult(sq);
        if (piece != EMPTY)
            self_add(feature_idx(piece, sq, pos.get_king_placement(pov), pov));
    }
}

void PovAccumulator::add(const PovAccumulator &input, const size_t add0) {
    for (int column{0}; column < L1_SIZE; ++column) {
        m_neurons[column] = input.m_neurons[column] + network.ft_weights[add0 + column];
    }
}

void PovAccumulator::sub(const PovAccumulator &input, const size_t sub0) {
    for (int column{0}; column < L1_SIZE; ++column) {
        m_neurons[column] = input.m_neurons[column] - network.ft_weights[sub0 + column];
    }
}

void PovAccumulator::add_sub(const PovAccumulator &input, const size_t add0, const size_t sub0) {
    for (int column{0}; column < L1_SIZE; ++column) {
        m_neurons[column] =
            input.m_neurons[column] + network.ft_weights[add0 + column] - network.ft_weights[sub0 + column];
    }
}

void PovAccumulator::add_sub2(const PovAccumulator &input, const size_t add0, const size_t sub0, const size_t sub1) {
    for (int column{0}; column < L1_SIZE; ++column) {
        m_neurons[column] = input.m_neurons[column] + network.ft_weights[add0 + column] -
                            network.ft_weights[sub0 + column] - network.ft_weights[sub1 + column];
    }
}

void PovAccumulator::add2_sub2(const PovAccumulator &input, const size_t add0, const size_t add1, const size_t sub0,
                               const size_t sub1) {
    for (int column{0}; column < L1_SIZE; ++column) {
        m_neurons[column] = input.m_neurons[column] + network.ft_weights[add0 + column] +
                            network.ft_weights[add1 + column] - network.ft_weights[sub0 + column] -
                            network.ft_weights[sub1 + column];
    }
}

void PovAccumulator::self_add(const size_t add0) { add(*this, add0); }

void PovAccumulator::self_sub(const size_t sub0) { sub(*this, sub0); }

void PovAccumulator::self_add_sub(const size_t add0, const size_t sub0) { add_sub(*this, add0, sub0); }

bool operator==(const PovAccumulator &lhs, const PovAccumulator &rhs) {
    for (size_t i = 0; i < lhs.m_neurons.size(); ++i) {
        if (lhs.m_neurons[i] != rhs.m_neurons[i])
            return false;
    }
    return true;
}
