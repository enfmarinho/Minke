/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef PACKED_POSITION_H
#define PACKED_POSITION_H

#include <cstdint>
#include <cstring>

#include "../position.h"

// White perspective
enum GameResult : uint8_t {
    LOSS,
    DRAW,
    WIN,
    NO_RESULT
};

class __attribute__((packed)) PackedPosition {
  public:
    PackedPosition(const Position &position, ScoreType score) {
        m_occupancy = position.get_occupancy();

        Bitboard occ = m_occupancy;
        int idx = 0;
        bool high_nibble = false;

        std::memset(m_pieces, 0, sizeof(m_pieces));
        while (occ) {
            Square sq = poplsb(occ);
            Piece pc = position.consult(sq);
            uint8_t piece_type = get_piece_type(pc);

            if (piece_type == ROOK && (position.get_castle_rooks() & (1ULL << sq)))
                piece_type = 6; // special "unmoved rook" id

            uint8_t color = (get_color(pc) == BLACK);
            uint8_t packed_piece = piece_type | (color << 3);

            if (high_nibble) {
                m_pieces[idx] |= (packed_piece << 4);
                idx++;
            } else {
                m_pieces[idx] = packed_piece;
            }
            high_nibble = !high_nibble;
        }

        m_stm_ep_sq = position.get_stm() == WHITE ? 0 : 0x80;
        if (position.get_en_passant() != NO_SQ)
            m_stm_ep_sq |= static_cast<uint8_t>(position.get_en_passant());

        m_half_move_counter = position.get_fifty_move_ply();
        m_game_clock = static_cast<uint16_t>((position.get_game_ply() / 2) + 1);
        m_score = score;
        m_result = DRAW; // should be given later
        m_padding = 0;
    }

    void set_result(uint8_t result) { m_result = result; }

  private:
    uint64_t m_occupancy;
    uint8_t m_pieces[16];
    uint8_t m_stm_ep_sq;
    uint8_t m_half_move_counter;
    uint16_t m_game_clock;
    int16_t m_score;
    uint8_t m_result;
    uint8_t m_padding;
};
static_assert(sizeof(PackedPosition) == 32, "PackedPosition struct is not 32 bytes");

#endif // #ifndef PACKED_POSITION_H
