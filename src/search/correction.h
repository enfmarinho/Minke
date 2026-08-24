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

#include <array>

#include "core/types.h"

constexpr HistoryType CORRHIST_SIZE = 16384;
constexpr HistoryType CORRHIST_MAX = 1024;
constexpr HistoryType CORRHIST_GRAIN = 256;

struct ThreadData;

class CorrectionHistory {
  public:
    CorrectionHistory() = default;
    ~CorrectionHistory() = default;

    void reset();

    void update(const ThreadData& td, int depth, int ply, int diff);

    HistoryType correction(const ThreadData& td, int ply) const;

  private:
    struct CorrectionEntry {
        HistoryType value{};

        inline void update(HistoryType bonus) {
            const int scaled_bonus = bonus - value * std::abs(bonus) / CORRHIST_MAX;
            value = std::clamp<int>(value + scaled_bonus, -CORRHIST_MAX, CORRHIST_MAX);
        }

        [[nodiscard]] inline operator HistoryType() const { return value; }
    };

    struct PovTables {
        std::array<CorrectionEntry, CORRHIST_SIZE> pawn{};
        std::array<CorrectionEntry, CORRHIST_SIZE> white_nonpawn{};
        std::array<CorrectionEntry, CORRHIST_SIZE> black_nonpawn{};
    };

    std::array<PovTables, 2> m_pov_tables;
    std::array<std::array<CorrectionEntry, 64 * 12>, 64 * 12> m_cont_corr{};
};
