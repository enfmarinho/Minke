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

#include "correction.h"

#include <cstring>

#include "move.h"
#include "position.h"
#include "search.h"
#include "tune.h"
#include "types.h"

void CorrectionHistory::reset() {
    m_pov_tables = {}; //
}

void CorrectionHistory::update(const ThreadData& td, const int depth, const int diff) {
    const HistoryType bonus = std::clamp(diff * depth / 8, -CORRHIST_MAX / 4, CORRHIST_MAX / 4);

    PovTables& tables = m_pov_tables[td.position.get_stm()];

    tables.pawn[td.position.get_pawn_hash() % CORRHIST_SIZE].update(bonus);
    tables.white_nonpawn[td.position.get_white_nonpawn_hash() % CORRHIST_SIZE].update(bonus);
    tables.black_nonpawn[td.position.get_black_nonpawn_hash() % CORRHIST_SIZE].update(bonus);

    if (td.height >= 2) {
        const PieceMove pmove1 = td.nodes[td.height - 1].curr_pmove;
        const PieceMove pmove2 = td.nodes[td.height - 2].curr_pmove;

        if (pmove1 != PIECE_MOVE_NONE && pmove2 != PIECE_MOVE_NONE) {
            m_cont_corr[pmove1.piece * pmove1.move.to()][pmove2.piece * pmove1.move.to()].update(bonus);
        }
    }
}

HistoryType CorrectionHistory::correction(const ThreadData& td) const {
    const PovTables& tables = m_pov_tables[td.position.get_stm()];

    HistoryType adjustment = pawn_corr_factor() * tables.pawn[td.position.get_pawn_hash() % CORRHIST_SIZE];
    adjustment += nonpawn_corr_factor() * tables.white_nonpawn[td.position.get_white_nonpawn_hash() % CORRHIST_SIZE];
    adjustment += nonpawn_corr_factor() * tables.black_nonpawn[td.position.get_black_nonpawn_hash() % CORRHIST_SIZE];

    if (td.height >= 2) {
        const PieceMove pmove1 = td.nodes[td.height - 1].curr_pmove;
        const PieceMove pmove2 = td.nodes[td.height - 2].curr_pmove;
        if (pmove1 != PIECE_MOVE_NONE && pmove2 != PIECE_MOVE_NONE) {
            adjustment +=
                cont_corr_factor() * m_cont_corr[pmove1.piece * pmove1.move.to()][pmove2.piece * pmove1.move.to()];
        }
    }

    return adjustment / CORRHIST_GRAIN;
}
