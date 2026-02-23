/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef TB_H
#define TB_H

#include "position.h"
#include "pyrrhic/tbprobe.h"
#include "types.h"

enum class ProbeResult {
    FAILED,
    WIN,
    LOSS,
    DRAW,
};

inline ProbeResult probe_tb(const Position& pos) {
    const unsigned int probe_result = tb_probe_wdl(
        pos.get_occupancy(WHITE), pos.get_occupancy(BLACK), pos.get_piece_bb(KING), pos.get_piece_bb(QUEEN),
        pos.get_piece_bb(ROOK), pos.get_piece_bb(BISHOP), pos.get_piece_bb(KNIGHT), pos.get_piece_bb(PAWN),
        pos.get_en_passant() == NO_SQ ? 0 : static_cast<int32_t>(pos.get_en_passant()), pos.get_stm() == WHITE ? 1 : 0);

    switch (probe_result) {
        case TB_RESULT_FAILED:
            return ProbeResult::FAILED;
        case TB_WIN:
            return ProbeResult::WIN;
        case TB_LOSS:
            return ProbeResult::LOSS;
        default:
            return ProbeResult::DRAW;
    }
}

#endif // #ifndef TB_H
