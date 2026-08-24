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

#include "search/cuckoo.h"

#include <utility>

#include "core/attacks.h"
#include "core/types.h"
#include "core/zobrist.h"
#include "utils/utils.h"

namespace Cuckoo {

std::array<HashType, 8192> keys{};
std::array<Move, 8192> moves{};

// Implementation based on Stockfish's and Stormphrax's
void init() {
    [[maybe_unused]] int count = 0;

    // skip pawns
    for (int pti = KNIGHT; pti <= KING; ++pti) {
        for (int colori = WHITE; colori <= BLACK; ++colori) {
            const PieceType pt = static_cast<PieceType>(pti);
            const Piece piece = get_piece(pt, static_cast<Color>(colori));

            for (int sqi0 = a1; sqi0 <= h8; ++sqi0) {
                const Square sq0 = static_cast<Square>(sqi0);

                for (int sqi1 = sqi0 + 1; sqi1 <= h8; ++sqi1) {
                    const Square sq1 = static_cast<Square>(sqi1);

                    if (!Attacks::piece_attack(pt, sq0, 0).is_set(sq1)) {
                        continue;
                    }

                    Move move = Move(sq0, sq1, REGULAR);
                    HashType key = Zobrist::piece_square_key({piece, sq0}) ^ Zobrist::piece_square_key({piece, sq1}) ^
                                   Zobrist::color_key();

                    size_t slot = h1(key);

                    while (true) {
                        std::swap(keys[slot], key);
                        std::swap(moves[slot], move);

                        if (move == Move::none()) {
                            break;
                        }

                        slot = (slot == h1(key) ? h2(key) : h1(key));
                    }

                    ++count;
                }
            }
        }
    }
    assert(count == 3668);
}

}; // namespace Cuckoo
