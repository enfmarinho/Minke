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
#include <cassert>
#include <cstddef>
#include <utility>

/// A wrapper for std::array.
/// Make it move convenient to use arrays by tracking it's own size, just like a std::vector
template <typename T, size_t MAX_SIZE>
class StaticVector {
  public:
    inline void push(const T &v) {
        assert(m_size < MAX_SIZE);
        m_array[m_size++] = v;
    }
    inline void push(const T &&v) {
        assert(m_size < MAX_SIZE);
        m_array[m_size++] = std::move(v);
    }

    inline void pop() {
        assert(m_size > 0);
        --m_size;
    }

    inline void resize(const size_t size) {
        assert(size <= MAX_SIZE);
        m_size = size;
    }
    inline void clear() { m_size = 0; }

    [[nodiscard]] inline const T &operator[](size_t idx) const {
        assert(idx < m_size);
        return m_array[idx];
    }

    [[nodiscard]] inline T &operator[](size_t idx) {
        assert(idx < m_size);
        return m_array[idx];
    }

    [[nodiscard]] bool empty() const { return m_size == 0; }
    [[nodiscard]] size_t size() const { return m_size; }
    [[nodiscard]] size_t capacity() const { return MAX_SIZE; }

    [[nodiscard]] inline auto begin() { return m_array.begin(); }
    [[nodiscard]] inline auto end() { return m_array.begin() + static_cast<std::ptrdiff_t>(m_size); }

    [[nodiscard]] inline auto begin() const { return m_array.begin(); }
    [[nodiscard]] inline auto end() const { return m_array.begin() + static_cast<std::ptrdiff_t>(m_size); }

    [[nodiscard]] inline T front() const { return m_array[0]; }
    [[nodiscard]] inline T back() const {
        assert(m_size > 0);
        return m_array[m_size - 1];
    }

  private:
    std::array<T, MAX_SIZE> m_array;
    size_t m_size{};
};
