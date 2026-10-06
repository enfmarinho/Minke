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
#include <cstdint>
#include <cstring>
#include <span>

#include "eval/nnue/arch.h"

class Position;

namespace accumulator {

class alignas(64) Perspective {
  public:
    Perspective() = default;
    Perspective(const Position &pos, Color pov);
    Perspective(const Perspective &copy) = default;
    ~Perspective() = default;

    Perspective &operator=(const Perspective &) = default;

    inline void reset() { std::memcpy(m_neurons.data(), network.ft_biases, sizeof(network.ft_biases)); }

    std::span<const int16_t, L1_SIZE> neurons() const { return m_neurons; }

    void add(const Perspective &input, size_t add0);
    void sub(const Perspective &input, size_t sub0);
    void add_sub(const Perspective &input, size_t add0, size_t sub0);
    void add_sub2(const Perspective &input, size_t add0, size_t sub0, size_t sub1);
    void add2_sub2(const Perspective &input, size_t add0, size_t add1, size_t sub0, size_t sub1);

    void self_add(size_t add0);
    void self_sub(size_t sub0);
    void self_add_sub(size_t add0, size_t sub0);

    friend bool operator==(const Perspective &lhs, const Perspective &rhs);

  private:
    std::array<int16_t, L1_SIZE> m_neurons;
};

} // namespace accumulator
