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

#ifndef POV_ACCUMULATOR_H
#define POV_ACCUMULATOR_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <span>

#include "../../move.h"
#include "../../types.h"
#include "../../utils.h"
#include "arch.h"

struct PieceSquare {
    Piece piece;
    Square sq;

    inline PieceSquare() : piece(EMPTY), sq(NO_SQ) {}
    inline PieceSquare(Piece _piece, Square _sq) : piece(_piece), sq(_sq) {}

    inline size_t feature_idx(const Color pov) const { return feature_idx(piece, sq, pov); }
    static size_t feature_idx(Piece piece, Square sq, const Color pov) {
        constexpr size_t COLOR_STRIDE = 64 * 6;
        constexpr size_t PIECE_STRIDE = 64;

        const Color piece_color = get_color(piece);
        int pov_sq = sq;
        if (pov == BLACK) // Convert square to pov, i.e. flip rank if stm is black
            pov_sq = pov_sq ^ 56;

        const size_t idx = (piece_color != pov) * COLOR_STRIDE +               // Perspective offset
                           get_piece_type(piece, piece_color) * PIECE_STRIDE + // Piece type offset
                           pov_sq;
        return idx * HIDDEN_LAYER_SIZE;
    }
};

struct DirtyPiece {
    PieceSquare add0, add1, sub0, sub1;
    MoveType move_type;
};

// NOTE: must be initialized by init()
class alignas(64) PovAccumulator {
  public:
    PovAccumulator() = default;
    ~PovAccumulator() = default;

    inline void reset() { std::ranges::copy(network.hidden_bias, m_neurons.data()); }

    const std::span<const int16_t> neurons() const { return m_neurons; }

    template <size_t NumAdds, size_t NumSubs>
    void update(const PovAccumulator &input, const std::array<size_t, NumAdds> &adds,
                const std::array<size_t, NumSubs> &subs);

    template <size_t NumAdds, size_t NumSubs>
    void self_update(const std::array<size_t, NumAdds> &adds, const std::array<size_t, NumSubs> &subs);

  private:
    std::array<int16_t, HIDDEN_LAYER_SIZE> m_neurons;
};

#endif // #ifndef POV_ACCUMULATOR_H
