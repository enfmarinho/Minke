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
        uint16_t encoded_move = 0;
        encoded_move = move.from_and_to();
        if (move.is_ep()) {
            encoded_move |= 0b01 << 14;
        } else if (move.is_castle()) {
            encoded_move |= 0b10 << 14;
        } else if (move.is_promotion()) {
            encoded_move |= (move.promotee() - 1) << 12;
            encoded_move |= 0b11 << 14;
        }

        m_moves_scores.emplace_back(encoded_move, score);
    }

    void write(std::ofstream &file_out, GameResult result) {
        constexpr char null_terminator[sizeof(ScoredMove)] = {};

        m_initial_pos.set_result(result);
        file_out.write(reinterpret_cast<const char *>(&m_initial_pos), sizeof(PackedPosition));
        file_out.write(reinterpret_cast<const char *>(m_moves_scores.data()),
                       sizeof(ScoredMove) * m_moves_scores.size());
        file_out.write(reinterpret_cast<const char *>(null_terminator), sizeof(ScoredMove));
    }

  private:
    struct MoveScore {
        uint16_t score;
        int16_t packed_move;

        MoveScore(uint16_t score, int16_t packed_move) : score(score), packed_move(packed_move) {}
    };
    static_assert(sizeof(MoveScore) == 4, "MoveScore struct is not 4 bytes");

    PackedPosition m_initial_pos;
    std::vector<MoveScore> m_moves_scores; // move and score for this ply
};

#endif // #ifndef VIRIFORMAT_H
