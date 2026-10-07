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

#include "eval/nnue/accumulator/stack.h"

#include <vector>

#include "core/position.h"
#include "core/types.h"
#include "eval/nnue/accumulator/state.h"

namespace nnue::accumulator {

void Stack::refresh(const Position &pos) {
    const auto &white_perspective_acc = m_finny_table.update(pos, WHITE);
    const auto &black_perspective_acc = m_finny_table.update(pos, BLACK);

    m_stack.clear();
    m_stack.emplace_back(white_perspective_acc, black_perspective_acc, pos.king_sq(WHITE), pos.king_sq(BLACK));

    assert(m_accumulators.back().updated(WHITE));
    assert(m_accumulators.back().updated(BLACK));
    assert(m_accumulators.back().pov(WHITE) == Perspective(pos, WHITE));
    assert(m_accumulators.back().pov(BLACK) == Perspective(pos, BLACK));
}

// TODO: simplify this API
void Stack::push(const DirtyPiece dp, const Square white_king_sq, const Square black_king_sq) {
    assert(!m_accumulators.empty()); // NNUE must have been initialized with the 'refresh' method before pushing
    m_stack.emplace_back(dp, white_king_sq, black_king_sq);
}

void Stack::pop() { m_stack.pop_back(); }

const State &Stack::update_top(const Position &pos) {
    update_pov(pos, WHITE);
    update_pov(pos, BLACK);
    return m_stack.back();
}

void Stack::update_pov(const Position &pos, const Color pov) {
    auto head = m_stack.rbegin();

    if (head->updated(pov))
        return;

    for (auto iter = m_stack.rbegin() + 1; iter != m_stack.rend(); ++iter) {
        if (iter->needs_refresh(pov, pos.king_sq(pov))) {
            const Perspective &acc = m_finny_table.update(pos, pov);
            head->refresh(acc, pov);
            break;
        } else if (iter->updated(pov)) {
            while (iter != head) {
                (iter - 1)->update(iter->pov(pov), pov);
                --iter;
            }
            break;
        }
    }
    assert(head->updated(pov));
    assert(head->pov(pov) == Perspective(pos, pov));
}

} // namespace nnue::accumulator
