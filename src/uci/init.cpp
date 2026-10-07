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

#include "uci/init.h"

#include <cmath>

#include "core/attacks.h"
#include "eval/nnue/arch.h"
#include "search/cuckoo.h"
#include "search/search.h"
#include "uci/tune.h"
#include "utils/incbin.h"
#include "utils/utils.h"

INCBIN(NetParameters, EVALFILE);

namespace minke {

int LMR_TABLE[64][64];
int LMP_TABLE[2][LMP_DEPTH];

// Heap allocated so the feature transformer weights can be backed by huge pages, which greatly reduces TLB misses
nnue::Network &nnue::network =
    *static_cast<nnue::Network *>(aligned_malloc(alignof(nnue::Network), sizeof(nnue::Network)));

void init_all() {
    init_search_params();
    init_network_params();
    attacks::init();
    cuckoo::init();
}

void init_search_params() {
    for (int depth = 1; depth < 64; ++depth) {
        for (int move_counter = 1; move_counter < 64; ++move_counter) {
            LMR_TABLE[depth][move_counter] = lmr_base() + lmr_scale() * std::log(depth) * std::log(move_counter);
        }
    }
    LMR_TABLE[0][0] = 0;

    for (int depth = 0; depth < LMP_DEPTH; ++depth) {
        LMP_TABLE[0][depth] = (lmp_base() / 100.0) + (lmp_scale() / 100.0) * depth * depth;
        LMP_TABLE[1][depth] = (lmp_improving_base() / 100.0) + (lmp_improving_scale() / 100.0) * depth * depth;
    }
}

void init_network_params() { nnue::network = *reinterpret_cast<const nnue::Network *>(&gNetParametersData); }

} // namespace minke
