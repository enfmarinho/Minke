/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef VIRIFORMAT_H
#define VIRIFORMAT_H

#include <cstdint>
#include <fstream>

#include "../move.h"
#include "../position.h"
#include "packed_position.h"

class Viriformat {
  public:
    Viriformat() : m_initial_pos(PackedPosition(Position(), 0)) { m_moves_scores.reserve(MAX_MOVES_PER_POS); }

    Viriformat(const Position &pos) : m_initial_pos(PackedPosition(pos, 0)) {
        m_moves_scores.reserve(MAX_MOVES_PER_POS);
    }

    ~Viriformat() = default;

    void reset(const Position &pos) {
        m_initial_pos = PackedPosition(pos, 0);
        m_moves_scores.clear();
    }

    void push(const Move &move, const ScoreType &score) {
        uint16_t packed_move = 0;
        packed_move = move.from_and_to();
        if (move.is_ep()) {
            packed_move |= 0b01 << 14;
        } else if (move.is_castle()) {
            packed_move |= 0b10 << 14;
        } else if (move.is_promotion()) {
            packed_move |= (move.promotee() - 1) << 12;
            packed_move |= 0b11 << 14;
        }

        m_moves_scores.emplace_back(packed_move, score);
    }

    void write(std::ofstream &file_out, GameResult result) {
        constexpr char null_terminator[sizeof(MoveScore)] = {};

        m_initial_pos.set_result(result);
        file_out.write(reinterpret_cast<const char *>(&m_initial_pos), sizeof(PackedPosition));
        file_out.write(reinterpret_cast<const char *>(m_moves_scores.data()),
                       sizeof(MoveScore) * m_moves_scores.size());
        file_out.write(reinterpret_cast<const char *>(null_terminator), sizeof(MoveScore));
    }

  private:
    struct MoveScore {
        uint16_t packed_move;
        int16_t score;

        MoveScore(uint16_t _packed_move, int16_t _score) : packed_move(_packed_move), score(_score) {}
    };
    static_assert(sizeof(MoveScore) == 4, "MoveScore struct is not 4 bytes");

    PackedPosition m_initial_pos;
    std::vector<MoveScore> m_moves_scores; // move and score for this ply
};

#endif // #ifndef VIRIFORMAT_H
