#include "utils.h"

#include <cassert>

#include "types.h"

inline int count_bits(const Bitboard &bitboard) { return std::popcount(bitboard); }

inline Square poplsb(Bitboard &bitboard) {
    Square sq = lsb(bitboard);
    bitboard &= bitboard - 1;
    return sq;
}

// Compiler specific definitions, taken from Stockfish
#if defined(__GNUC__) // GCC, Clang, ICX

inline Square lsb(Bitboard b) {
    assert(b);
    return Square(__builtin_ctzll(b));
}

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

inline int rank(Square sq) { return sq >> 3; }

inline int file(Square sq) { return sq & 0b111; }
