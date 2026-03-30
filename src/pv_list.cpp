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

#include "pv_list.h"

#include <iostream>

void PvList::update(Move new_move, const PvList &list) {
    std::copy(list.m_pv.begin(), list.m_pv.begin() + list.m_size, m_pv.begin() + 1);
    m_pv[0] = new_move;

    m_size = list.m_size + 1;
}

void PvList::print(const bool chess960, const Bitboard castle_rooks) const {
    for (int i = 0; i < m_size; ++i) {
        std::cout << m_pv[i].get_algebraic_notation(chess960, castle_rooks) << ' ';
    }
}

void PvList::clear() { m_size = 0; }
