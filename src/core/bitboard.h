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

#include <bit>
#include <cassert>
#include <cstdint>
#include <iostream>

#include "core/types.h"

class Bitboard {
  public:
    using UnderlyingT = uint64_t;

    constexpr Bitboard(uint64_t bb = 0) : m_bb(bb) {}
    constexpr Bitboard(Square sq) : m_bb(1ULL << sq) {}

    constexpr void set_mask(Bitboard mask) { m_bb |= mask.m_bb; }
    constexpr void unset_mask(Bitboard mask) { m_bb &= ~mask.m_bb; }

    constexpr void set_sq(Square sq) { m_bb |= (1ULL << sq); }
    constexpr void unset_sq(Square sq) { m_bb &= ~(1ULL << sq); }

    constexpr bool is_set(Square sq) const { return m_bb & (1ULL << sq); }

    constexpr int popcount() const { return std::popcount(m_bb); }

    constexpr Square msb() const {
        assert(m_bb);
        return Square(63 ^ __builtin_clzll(m_bb));
    }

    constexpr Square lsb() const {
        assert(m_bb);
        return Square(__builtin_ctzll(m_bb));
    }

    constexpr Square poplsb() {
        Square sq = lsb();
        m_bb &= m_bb - 1;
        return sq;
    }

    constexpr Bitboard isolate_lsb() const { return m_bb & -m_bb; }

    constexpr UnderlyingT raw() const { return m_bb; }

    ///=== Shifts
    // rays
    constexpr Bitboard shift_north() const { return (m_bb << NORTH); }
    constexpr Bitboard shift_south() const { return (m_bb >> -SOUTH); }
    constexpr Bitboard shift_east() const { return (m_bb << EAST) & ~FILE_A; }
    constexpr Bitboard shift_west() const { return (m_bb >> -WEST) & ~FILE_H; }

    constexpr Bitboard shift_north_east() const { return (m_bb << NORTH_EAST) & ~FILE_A; }
    constexpr Bitboard shift_north_west() const { return (m_bb << NORTH_WEST) & ~FILE_H; }

    constexpr Bitboard shift_south_east() const { return (m_bb >> -SOUTH_EAST) & ~FILE_A; }
    constexpr Bitboard shift_south_west() const { return (m_bb >> -SOUTH_WEST) & ~FILE_H; }

    // knight moves
    constexpr Bitboard shift_double_north_east() const { return (m_bb << DOUBLE_NORTH_EAST) & ~FILE_A; }
    constexpr Bitboard shift_double_north_west() const { return (m_bb << DOUBLE_NORTH_WEST) & ~FILE_H; }

    constexpr Bitboard shift_double_south_east() const { return (m_bb >> -DOUBLE_SOUTH_EAST) & ~FILE_A; }
    constexpr Bitboard shift_double_south_west() const { return (m_bb >> -DOUBLE_SOUTH_WEST) & ~FILE_H; }

    constexpr Bitboard shift_double_east_north() const { return (m_bb << DOUBLE_EAST_NORTH) & ~(FILE_A | FILE_B); }
    constexpr Bitboard shift_double_east_south() const { return (m_bb >> -DOUBLE_EAST_SOUTH) & ~(FILE_A | FILE_B); }

    constexpr Bitboard shift_double_west_north() const { return (m_bb << DOUBLE_WEST_NORTH) & ~(FILE_H | FILE_G); }
    constexpr Bitboard shift_double_west_south() const { return (m_bb >> -DOUBLE_WEST_SOUTH) & ~(FILE_H | FILE_G); }

    // perspective pawn pushes
    Bitboard shift_up_pov(Color c) const {
        if (c == WHITE)
            return shift_north();
        else
            return shift_south();
    }

    Bitboard shift_up_east_pov(Color c) const {
        if (c == WHITE)
            return shift_north_east();
        else
            return shift_south_east();
    }

    Bitboard shift_up_west_pov(Color c) const {
        if (c == WHITE)
            return shift_north_west();
        else
            return shift_south_west();
    }
    ///===

    constexpr explicit operator uint64_t() const { return uint64_t(m_bb); }
    constexpr explicit operator bool() const { return m_bb != 0; }

    constexpr Bitboard operator&(const Bitboard rhs) const { return m_bb & rhs.m_bb; }
    constexpr Bitboard operator&(const UnderlyingT rhs) const { return m_bb & rhs; }

    constexpr Bitboard operator|(const Bitboard rhs) const { return m_bb | rhs.m_bb; }
    constexpr Bitboard operator|(const UnderlyingT rhs) const { return m_bb | rhs; }

    constexpr Bitboard operator^(const Bitboard rhs) const { return m_bb ^ rhs.m_bb; }
    constexpr Bitboard operator^(const UnderlyingT rhs) const { return m_bb ^ rhs; }

    constexpr Bitboard operator~() const { return ~m_bb; }

    constexpr Bitboard operator&=(const Bitboard rhs) {
        m_bb &= rhs.m_bb;
        return *this;
    }
    constexpr Bitboard operator&=(const UnderlyingT rhs) {
        m_bb &= rhs;
        return *this;
    }

    constexpr Bitboard operator|=(const Bitboard rhs) {
        m_bb |= rhs.m_bb;
        return *this;
    }
    constexpr Bitboard operator|=(const UnderlyingT rhs) {
        m_bb |= rhs;
        return *this;
    }

    constexpr Bitboard operator^=(const Bitboard rhs) {
        m_bb ^= rhs.m_bb;
        return *this;
    }
    constexpr Bitboard operator^=(const UnderlyingT rhs) {
        m_bb ^= rhs;
        return *this;
    }
    constexpr bool operator==(const Bitboard rhs) const { return m_bb == rhs.m_bb; }
    constexpr bool operator==(const UnderlyingT rhs) const { return m_bb == rhs; }
    constexpr bool operator!=(const Bitboard rhs) const { return m_bb != rhs.m_bb; }
    constexpr bool operator!=(const UnderlyingT rhs) const { return m_bb != rhs; }

    // For Carry-Rippler
    constexpr Bitboard operator-(const Bitboard rhs) const { return m_bb - rhs.m_bb; }
    constexpr Bitboard operator*(const Bitboard rhs) const { return m_bb * rhs.m_bb; }

    void print() const {
        for (int line = 7; line >= 0; --line) {
            for (int column = 0; column < 8; ++column) {
                int sqi = line * 8 + column;
                if (!column)
                    std::cout << "  " << line + 1 << "  ";
                std::cout << (is_set(Square(sqi)) ? "\033[32m1\033[0m" : "0") << " ";
            }
            std::cout << "\n";
        }
        std::cout << "\n     a b c d e f g h\n\n";
    }

    static constexpr UnderlyingT RANK_1 = 0xFF;
    static constexpr UnderlyingT RANK_2 = 0xFF00;
    static constexpr UnderlyingT RANK_3 = 0xFF0000;
    static constexpr UnderlyingT RANK_4 = 0xFF000000;
    static constexpr UnderlyingT RANK_5 = 0xFF00000000;
    static constexpr UnderlyingT RANK_6 = 0xFF0000000000;
    static constexpr UnderlyingT RANK_7 = 0xFF000000000000;
    static constexpr UnderlyingT RANK_8 = 0xFF00000000000000;

    static constexpr UnderlyingT FILE_A = 0x101010101010101;
    static constexpr UnderlyingT FILE_B = 0x202020202020202;
    static constexpr UnderlyingT FILE_C = 0x404040404040404;
    static constexpr UnderlyingT FILE_D = 0x808080808080808;
    static constexpr UnderlyingT FILE_E = 0x1010101010101010;
    static constexpr UnderlyingT FILE_F = 0x2020202020202020;
    static constexpr UnderlyingT FILE_G = 0x4040404040404040;
    static constexpr UnderlyingT FILE_H = 0x8080808080808080;

    static constexpr UnderlyingT WHITE_OO_CROSSING_MASK = 0x60;
    static constexpr UnderlyingT WHITE_OOO_CROSSING_MASK = 0xe;
    static constexpr UnderlyingT BLACK_OO_CROSSING_MASK = 0x6000000000000000;
    static constexpr UnderlyingT BLACK_OOO_CROSSING_MASK = 0xe00000000000000;

    static constexpr UnderlyingT EMPTY = 0x0;

    static inline UnderlyingT pov_first_rank(Color color) { return (color == WHITE ? RANK_1 : RANK_8); }
    static inline UnderlyingT pawn_promotion_rank(Color color) { return (color == WHITE ? RANK_8 : RANK_1); }

  private:
    UnderlyingT m_bb;
};
