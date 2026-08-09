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
#include <cstddef>

#include "core/types.h"

template <typename TYPE>
inline void set_bits(TYPE &bits, const TYPE &mask) {
    bits |= mask;
}
template void set_bits<uint8_t>(uint8_t &bits, const uint8_t &mask);

template <typename TYPE>
inline void unset_mask(TYPE &bits, const TYPE &mask) {
    bits &= (~mask);
}
template void unset_mask<uint8_t>(uint8_t &bits, const uint8_t &mask);

// Returns the rank of "sq"
inline int get_rank(Square sq) { return sq >> 3; }

// Returns the file of "sq"
inline int get_file(Square sq) { return sq & 0b111; }

inline Piece get_piece(const PieceType &piece_type, const Color &color) {
    return static_cast<Piece>(piece_type + color * COLOR_OFFSET);
}

inline PieceType get_piece_type(const Piece &piece, const Color &color) {
    return static_cast<PieceType>(piece - color * COLOR_OFFSET);
}

inline PieceType get_piece_type(const Piece &piece) {
    assert(piece >= WHITE_PAWN && piece <= EMPTY);
    if (piece >= 6)
        return static_cast<PieceType>(piece - 6);
    return static_cast<PieceType>(piece);
}

inline Color get_color(const Piece &piece) { return static_cast<Color>(piece / COLOR_OFFSET); }

inline Square get_square(const int file, const int rank) { return static_cast<Square>(rank * 8 + file); }

inline int get_pawn_start_rank(const Color &color) { return color == WHITE ? 1 : 6; }

inline int get_pawn_promotion_rank(const Color &color) { return color == WHITE ? 7 : 0; }

inline Direction get_pawn_offset(const Color &color) { return color == WHITE ? NORTH : SOUTH; }

#if defined(__linux__)
#include <sys/mman.h>
#endif

inline void *aligned_malloc(size_t alignment, size_t required_bytes) {
    void *ptr = nullptr;
    size_t remainder = required_bytes % alignment;
    if (remainder != 0)
        required_bytes += (alignment - remainder);

#if defined(_WIN32)
    ptr = _aligned_malloc(required_bytes, alignment);
#elif defined(__APPLE__) || defined(__ANDROID__)
    posix_memalign(&ptr, alignment, required_bytes);
#else
    ptr = std::aligned_alloc(alignment, required_bytes);
    madvise(ptr, required_bytes, MADV_HUGEPAGE);
#endif
    return ptr;
}

inline void aligned_free(void *ptr) {
#if defined(_WIN32)
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

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
