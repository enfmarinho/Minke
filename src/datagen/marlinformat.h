/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef MARLINFORMAT_H
#define MARLINFORMAT_H

#include <fstream>

#include "../move.h"
#include "../position.h"
#include "packed_position.h"

class Marlinformat {
  public:
    Marlinformat() : m_pos(Position()) { m_packed_positions.reserve(MAX_MOVES_PER_POS); }

    Marlinformat(const Position &pos) : m_pos(pos) { m_packed_positions.reserve(MAX_MOVES_PER_POS); }

    ~Marlinformat() = default;

    void reset(const Position &pos) {
        m_pos = pos;
        m_packed_positions.clear();
    }

    void push(const Move &move, const ScoreType &score) {
        m_packed_positions.emplace_back(PackedPosition(m_pos, score));
        m_pos.make_move<false>(move);
    }

    void write(std::ofstream &file_out, GameResult result) {
        for (PackedPosition &packed_pos : m_packed_positions) {
            packed_pos.set_result(result);
        }

        file_out.write(reinterpret_cast<const char *>(m_packed_positions.data()),
                       sizeof(PackedPosition) * m_packed_positions.size());
    }

  private:
    Position m_pos;
    std::vector<PackedPosition> m_packed_positions;
};

#endif // #ifndef MARLINFORMAT_H
