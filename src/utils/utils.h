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
inline void set_bits(TYPE &bits, TYPE mask) {
    bits |= mask;
}
template void set_bits<uint8_t>(uint8_t &bits, uint8_t mask);

template <typename TYPE>
inline void unset_mask(TYPE &bits, TYPE mask) {
    bits &= (~mask);
}
template void unset_mask<uint8_t>(uint8_t &bits, uint8_t mask);

// Returns the rank of "sq"
constexpr inline int get_rank(Square sq) { return sq >> 3; }

// Returns the file of "sq"
constexpr inline int get_file(Square sq) { return sq & 0b111; }

inline Piece get_piece(PieceType piece_type, Color color) {
    return static_cast<Piece>(piece_type + color * COLOR_OFFSET);
}

inline PieceType get_piece_type(Piece piece, Color color) {
    return static_cast<PieceType>(piece - color * COLOR_OFFSET);
}

inline PieceType get_piece_type(Piece piece) {
    assert(piece >= WHITE_PAWN && piece <= EMPTY);
    if (piece >= 6)
        return static_cast<PieceType>(piece - 6);
    return static_cast<PieceType>(piece);
}

inline Color get_color(Piece piece) { return static_cast<Color>(piece / COLOR_OFFSET); }

inline Square get_square(int file, int rank) { return static_cast<Square>(rank * 8 + file); }

inline int get_pawn_start_rank(Color color) { return color == WHITE ? 1 : 6; }

inline int get_pawn_promotion_rank(Color color) { return color == WHITE ? 7 : 0; }

inline Direction get_pawn_offset(Color color) { return color == WHITE ? NORTH : SOUTH; }

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
