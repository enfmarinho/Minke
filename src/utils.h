/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef UTILS_H
#define UTILS_H

#include <bit>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>

#include "types.h"

inline void print_bb(Bitboard bb) {
    for (int line = 7; line >= 0; --line) {
        for (int column = 0; column < 8; ++column) {
            int sq = line * 8 + column;
            if (!column)
                std::cout << "  " << line + 1 << "  ";
            std::cout << (bb & (1ULL << sq) ? "\033[32m1\033[0m" : "0") << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n     a b c d e f g h\n\n";
}

inline void set_bit(Bitboard &bitboard, const Square &sq) { bitboard |= (1ULL << sq); }
inline void set_bit(Bitboard &bitboard, const int &sq) { bitboard |= (1ULL << sq); }

inline void unset_bit(Bitboard &bitboard, const Square &sq) { bitboard &= ~(1ULL << sq); }
inline void unset_bit(Bitboard &bitboard, const int &sq) { bitboard &= ~(1ULL << sq); }

template <typename TYPE>
inline void set_bits(TYPE &bits, const TYPE &mask) {
    bits |= mask;
}
template void set_bits<uint8_t>(uint8_t &bits, const uint8_t &mask);
template void set_bits<Bitboard>(Bitboard &bits, const Bitboard &mask);

template <typename TYPE>
inline void unset_mask(TYPE &bits, const TYPE &mask) {
    bits &= (~mask);
}
template void unset_mask<uint8_t>(uint8_t &bits, const uint8_t &mask);
template void unset_mask<Bitboard>(Bitboard &bits, const Bitboard &mask);

// Returns the number of '1' bit in bitboard, just like popcount
inline int count_bits(const Bitboard &bitboard) { return std::popcount(bitboard); }

// Compiler specific definitions, taken from Stockfish
#if defined(__GNUC__) // GCC, Clang, ICX

// Returns the least significant bit in a non-zero bitboard.
inline Square lsb(Bitboard b) {
    assert(b);
    return Square(__builtin_ctzll(b));
}

// Returns the most significant bit in a non-zero bitboard.
inline Square msb(Bitboard b) {
    assert(b);
    return Square(63 ^ __builtin_clzll(b));
}

#elif defined(_MSC_VER)
inline Square lsb(Bitboard b) {
    assert(b);
#ifdef _WIN64 // MSVC, WIN64

    unsigned long idx;
    _BitScanForward64(&idx, b);
    return Square(idx);

#else // MSVC, WIN32
    unsigned long idx;

    if (b & 0xffffffff) {
        _BitScanForward(&idx, int32_t(b));
        return Square(idx);
    } else {
        _BitScanForward(&idx, int32_t(b >> 32));
        return Square(idx + 32);
    }
#endif
}

inline Square msb(Bitboard b) {
    assert(b);
#ifdef _WIN64 // MSVC, WIN64

    unsigned long idx;
    _BitScanReverse64(&idx, b);
    return Square(idx);

#else // MSVC, WIN32

    unsigned long idx;

    if (b >> 32) {
        _BitScanReverse(&idx, int32_t(b >> 32));
        return Square(idx + 32);
    } else {
        _BitScanReverse(&idx, int32_t(b));
        return Square(idx);
    }
#endif
}
#else
#error "Compiler not suported."
#endif

// Returns the least significant bit in a non-zero bitboard and sets it to 0.
inline Square poplsb(Bitboard &bitboard) {
    Square sq = lsb(bitboard);
    bitboard &= bitboard - 1;
    return sq;
}

inline Bitboard shift(const Bitboard &bitboard, const int &direction) {
    if (bitboard == 0)
        return 0;

    if (direction < 0) {
        assert(lsb(bitboard) + direction >= a1); // Checks for overflow
        return bitboard >> std::abs(direction);
    } else {
        assert(msb(bitboard) + direction <= h8); // Checks for overflow
        return bitboard << direction;
    }
}
inline Bitboard shift(const Bitboard &bitboard, const Direction &direction) {
    return shift(bitboard, static_cast<int>(direction));
}

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

inline int64_t rand(int64_t min, int64_t max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int64_t> dist(min, max);
    return dist(gen);
}

#if defined(__linux__)
#include <sys/mman.h>
#endif

inline void *aligned_malloc(size_t alignment, size_t requiredBytes) {
    void *ptr = nullptr;
#if defined(_WIN32)
    ptr = _aligned_malloc(requiredBytes, alignment);
#elif defined(__APPLE__) || defined(__ANDROID__)
    posix_memalign(&ptr, alignment, size);
#else
    ptr = std::aligned_alloc(alignment, requiredBytes);
    madvise(ptr, requiredBytes, MADV_HUGEPAGE);
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

#endif // #ifndef UTILS_H
