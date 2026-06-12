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

#ifndef POV_ACCUMULATOR_H
#define POV_ACCUMULATOR_H

#include <array>
#include <cstdint>
#include <cstring>
#include <span>

#include "arch.h"

class Position;

class alignas(64) PovAccumulator {
  public:
    PovAccumulator() = default;
    PovAccumulator(const Position &pos, const Color pov);
    PovAccumulator(const PovAccumulator &copy) = default;
    ~PovAccumulator() = default;

    PovAccumulator &operator=(const PovAccumulator &) = default;

    inline void reset() { std::memcpy(m_neurons.data(), network.hidden_bias.data(), sizeof(network.hidden_bias)); }

    std::span<const int16_t> neurons() const { return m_neurons; }

    void add(const PovAccumulator &input, const size_t add0);
    void sub(const PovAccumulator &input, const size_t sub0);
    void add_sub(const PovAccumulator &input, const size_t add0, const size_t sub0);
    void add_sub2(const PovAccumulator &input, const size_t add0, const size_t sub0, const size_t sub1);
    void add2_sub2(const PovAccumulator &input, const size_t add0, const size_t add1, const size_t sub0,
                   const size_t sub1);

    void self_add(const size_t add0);
    void self_sub(const size_t sub0);
    void self_add_sub(const size_t add0, const size_t sub0);

    friend bool operator==(const PovAccumulator &lhs, const PovAccumulator &rhs);

  private:
    std::array<int16_t, HIDDEN_LAYER_SIZE> m_neurons;
};

#endif // #ifndef POV_ACCUMULATOR_H
