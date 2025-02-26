#include "utils.h"

#include <cassert>

#include "types.h"

inline void set_bit(Bitboard &bitboard, const Square &sq) { bitboard |= (1ULL << sq); }

inline void set_bit(Bitboard &bitboard, const int &sq) { bitboard |= (1ULL << sq); }

Bitboard shift(const Bitboard &bitboard, const int &direction) {
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

inline int get_rank(Square sq) { return sq >> 3; }

inline int get_file(Square sq) { return sq & 0b111; }

inline Piece get_piece(const PieceType &piece_type, const Color &color) {
    return static_cast<Piece>(piece_type + color * ColorOffset);
}

inline PieceType get_piece_type(const Piece &piece, const Color &color) {
    return static_cast<PieceType>(piece - color * ColorOffset);
}

inline PieceType get_piece_type(const Piece &piece) {
    assert(piece >= WhitePawn && piece <= BlackKing);
    if (piece > 6) {
        return static_cast<PieceType>(piece - 6);
    }
    return static_cast<PieceType>(piece);
}

inline Color get_color(const Piece &piece) { return static_cast<Color>(piece / ColorOffset); }

inline Square get_square(const int file, const int rank) { return static_cast<Square>(file * 8 + rank); }
