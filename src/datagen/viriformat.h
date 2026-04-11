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

#ifndef VIRIFORMAT_H
#define VIRIFORMAT_H

#include <cstdint>
#include <fstream>

#include "../move.h"
#include "../position.h"
#include "packed_position.h"

class Viriformat {
  public:
    Viriformat() : m_initial_pos(PackedPosition(GameState(), 0)) { m_moves_scores.reserve(MAX_MOVES_PER_POS); }

    Viriformat(const GameState &pos) : m_initial_pos(PackedPosition(pos, 0)) {
        m_moves_scores.reserve(MAX_MOVES_PER_POS);
    }

    ~Viriformat() = default;

    void reset(const GameState &pos) {
        m_initial_pos = PackedPosition(pos, 0);
        m_moves_scores.clear();
    }

    void push(const Move &move, const ScoreType &score) {
        uint16_t packed_move = 0;
        packed_move = move.from_and_to();
        if (move.is_ep()) {
            packed_move |= 0b01 << 14;
        } else if (move.is_castle()) {
            packed_move = move.from();
            // TODO this won't work for FRC
            // Viriformat expects that a castling move destiny is the rook source sq
            if (move.to() == g1) {
                packed_move |= h1 << 6;
            } else if (move.to() == c1) {
                packed_move |= a1 << 6;
            } else if (move.to() == g8) {
                packed_move |= h8 << 6;
            } else if (move.to() == c8) {
                packed_move |= a8 << 6;
            } else {
                assert(false);
            }
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
