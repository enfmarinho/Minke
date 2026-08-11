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

#pragma once

#include <cassert>
#include <cstdint>

#include "core/types.h"
#include "utils/random.h"

namespace Zobrist {

// Randoms uint64_t for performing Zobrist Hashing.
struct HashKeys {
    HashType pieces[12][64];
    HashType castle[16];
    HashType en_passant[8];
    HashType side;
};

constexpr HashKeys HASH_KEYS = []() {
    constexpr uint64_t SEED = 1070372;

    PRNG prng(SEED);
    HashKeys hash_keys{};
    for (int piece = WHITE_PAWN; piece <= BLACK_KING; ++piece) {
        for (int sqi = a1; sqi <= h8; ++sqi) {
            hash_keys.pieces[piece][sqi] = prng.rand<HashType>();
        }
    }

    for (int castle = 0; castle < 16; ++castle) {
        hash_keys.castle[castle] = prng.rand<HashType>();
    }

    for (int rank = 0; rank < 8; ++rank) {
        hash_keys.en_passant[rank] = prng.rand<HashType>();
    }

    hash_keys.side = prng.rand<HashType>();

    return hash_keys;
}();

inline HashType piece_square_key(PieceSquare psq) { return HASH_KEYS.pieces[psq.piece][psq.sq]; }

inline HashType castle_key(uint8_t castling_rights) { return HASH_KEYS.castle[castling_rights]; }

inline HashType ep_key(int ep_file) { return HASH_KEYS.en_passant[ep_file]; }

inline HashType color_key() { return HASH_KEYS.side; }

}; // namespace Zobrist
