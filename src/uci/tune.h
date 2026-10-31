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

#ifdef TUNE

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "core/types.h"

namespace minke::tunable {

struct TunableParam {
    std::string name;
    int default_value;
    int curr_value;
    int min_value;
    int max_value;
    float cend;
    float rend;

    TunableParam() = delete;
    TunableParam(std::string name, int default_value, int min_value, int max_value, float cend, float rend)
        : name(name),
          default_value(default_value),
          curr_value(default_value),
          min_value(min_value),
          max_value(max_value),
          cend(cend),
          rend(rend) {}
    ~TunableParam() = default;

    // This function print the TunableParam in the UCI option format
    void print() const {
        std::cout << "option name " << name << " type spin default " << default_value << " min " << min_value << " max "
                  << max_value << "\n";
    }

    // This function print the TunableParam in the OpenBench input format
    void print_ob_format() const {
        std::cout << name << ", int, " << curr_value << ", " << min_value << ", " << max_value << ", " << cend << ", "
                  << rend << "\n";
    }
};

// Some parts of this implementation were inspired by Stormphrax's tune code
class TunableParamList {
  public:
    static TunableParamList &get() {
        static TunableParamList tunable_param_list;
        return tunable_param_list;
    }

    auto begin() { return m_params.begin(); }
    auto end() { return m_params.end(); }

    inline const TunableParam &insert(TunableParam param) {
        if (m_params.size() == m_params.capacity()) {
            std::cerr << "Attempted to add an element to a full TunableParamList, resizing it would invalidate "
                         "existing references.\n";
            exit(1);
        }

        return m_params.emplace_back(param);
    }

    inline TunableParam *find(const std::string &param_name) {
        for (TunableParam &param : m_params) {
            if (param.name == param_name) {
                return &param;
            }
        }

        return nullptr;
    }

  private:
    TunableParamList() {
        m_params.reserve(128); // This should be way more than enough
    }
    ~TunableParamList() = default;

    // Delete copy and move operations to enforce Singleton behavior
    TunableParamList(const TunableParamList &) = delete;
    TunableParamList operator=(const TunableParamList &) = delete;
    TunableParamList(const TunableParamList &&) = delete;
    TunableParamList operator=(const TunableParamList &&) = delete;

    // This could be a hash table to save lookup time but this is fine for now
    std::vector<TunableParam> m_params;
};

#define TUNABLE_PARAM(name, default, min, max, cend, rend)                                                            \
    static_assert(cend > 0);                                                                                          \
    inline const TunableParam &tuned_##name = TunableParamList::get().insert({#name, default, min, max, cend, rend}); \
    inline int name() { return tuned_##name.curr_value; }

#else

namespace minke::tunable {

#define TUNABLE_PARAM(name, default, min, max, cend, rend) \
    constexpr int name() { return default; }

#endif // #ifdef TUNE

#define FIXED_PARAM(name, default, min, max, cend, rend) \
    constexpr int name() { return default; }

// Aspiration Windows
FIXED_PARAM(aw_min_depth, 3, 1, 10, 0.5, 0.002)
TUNABLE_PARAM(aw_first_window, 5, 1, 25, 1, 0.002)
TUNABLE_PARAM(aw_widening_factor, 35, 1, 100, 5, 0.002)

// Null move pruning
TUNABLE_PARAM(nmp_base_reduction, 316, 200, 400, 10, 0.002)
TUNABLE_PARAM(nmp_depth_factor, 22, 10, 40, 1.5, 0.002)
TUNABLE_PARAM(nmp_beta_base_margin, 201, 100, 300, 10, 0.002)
TUNABLE_PARAM(nmp_beta_improving_margin, 53, 0, 100, 5, 0.002)
TUNABLE_PARAM(nmp_beta_depth_factor, 1339, 500, 2000, 75, 0.002)
FIXED_PARAM(nmp_min_depth, 2, 2, 8, 0.5, 0.002)

// Hindsight
TUNABLE_PARAM(hindsight_eval, 151, 50, 300, 12.5, 0.002)

// Reverse Futility Pruning
TUNABLE_PARAM(rfp_depth_factor, 115, 50, 150, 5, 0.002)
TUNABLE_PARAM(rfp_improving_margin, -98, -150, 0, 7.5, 0.002)
TUNABLE_PARAM(rfp_opp_worsening_margin, -54, -120, 40, 8, 0.002)
TUNABLE_PARAM(rfp_complexity_factor, 262, 100, 600, 25, 0.002)
FIXED_PARAM(rfp_max_depth, 10, 5, 15, 0.5, 0.002)

// Late Move Reductions
TUNABLE_PARAM(lmr_base, 100, 0, 400, 20, 0.002)
TUNABLE_PARAM(lmr_scale, 610, 400, 900, 25, 0.002)
TUNABLE_PARAM(lmr_gives_check_delta, 1019, 500, 2000, 75, 0.002)
TUNABLE_PARAM(lmr_non_improving_delta, 1255, 500, 2000, 75, 0.002)
TUNABLE_PARAM(lmr_cutnode_delta, 1013, 500, 2000, 75, 0.002)
TUNABLE_PARAM(lmr_ttpv_delta, 1040, 500, 2000, 75, 0.002)
TUNABLE_PARAM(lmr_corrhist_divisor, 244, 100, 600, 25, 0.002)
TUNABLE_PARAM(lmr_ttmove_noisy_reduction, 883, 500, 2000, 75, 0.002)
TUNABLE_PARAM(lmr_quiet_hist_divisor, 10000, 4000, 16000, 300, 0.002)
TUNABLE_PARAM(lmr_deeper_margin, 23, 1, 50, 2, 0.002)
TUNABLE_PARAM(lmr_deeper_depth_factor, 2, 0, 8, 0.5, 0.002)
TUNABLE_PARAM(lmr_shallower_margin, 7, 0, 40, 2, 0.002)
TUNABLE_PARAM(lmr_shallower_depth_factor, 1, -2, 6, 0.5, 0.002)

TUNABLE_PARAM(lmr_noisy_cutnode_delta, 927, 500, 2000, 75, 0.002)

// Late move pruning
TUNABLE_PARAM(lmp_base, 157, 0, 400, 20, 0.002)
TUNABLE_PARAM(lmp_scale, 34, 20, 120, 5, 0.002)
TUNABLE_PARAM(lmp_improving_base, 240, 50, 550, 25, 0.002)
TUNABLE_PARAM(lmp_improving_scale, 78, 30, 150, 6, 0.002)

// Quiet History Pruning
TUNABLE_PARAM(history_pruning_max_depth_scaled, 5029, 2000, 8000, 300, 0.002)
TUNABLE_PARAM(quiet_hist_pruning_factor, -1680, -2500, -500, 100, 0.002)
TUNABLE_PARAM(quiet_hist_pruning_base, -474, -1200, 200, 70, 0.002)

// SEE pruning
TUNABLE_PARAM(see_noisy_pruning_factor, -19, -100, 50, 7.5, 0.002)

// Singular Extension
FIXED_PARAM(singular_extension_min_depth, 6, 4, 10, 0.5, 0.002)
TUNABLE_PARAM(singular_extension_depth_factor, 17, -20, 60, 4, 0.002)
TUNABLE_PARAM(double_extension_base, 8, -20, 40, 3, 0.002)
TUNABLE_PARAM(double_extension_pv_factor, 192, 100, 400, 15, 0.002)
TUNABLE_PARAM(triple_extension_base, 97, 40, 140, 5, 0.002)

// Razoring
FIXED_PARAM(razoring_max_depth, 5, 2, 6, 0.5, 0.002)
TUNABLE_PARAM(razoring_mult, 246, 150, 300, 7.5, 0.002)

// Futility Pruning
TUNABLE_PARAM(fp_max_depth, 12456, 8000, 16000, 400, 0.002)
TUNABLE_PARAM(fp_margin, 243, 100, 400, 15, 0.002)
TUNABLE_PARAM(fp_depth_factor, 95, 50, 150, 5, 0.002)

// Prob Cut
TUNABLE_PARAM(probcut_margin, 324, 250, 450, 10, 0.002)
TUNABLE_PARAM(probcut_improving_margin, 23, -100, 100, 10, 0.002)
FIXED_PARAM(probcut_min_depth, 5, 4, 8, 0.5, 0.002)

// History Formulas Parameters
TUNABLE_PARAM(hist_bonus_mult, 298, 1, 500, 25, 0.002)
TUNABLE_PARAM(hist_bonus_offset, 281, -500, 500, 50, 0.002)
TUNABLE_PARAM(hist_bonus_max, 2391, 1500, 3500, 100, 0.002)

TUNABLE_PARAM(hist_penalty_mult, 111, 0, 300, 15, 0.002)
TUNABLE_PARAM(hist_penalty_offset, 968, 200, 1600, 70, 0.002)
TUNABLE_PARAM(hist_penalty_max, 1285, 600, 2200, 80, 0.002)

TUNABLE_PARAM(pawnhist_bonus_mult, 192, 1, 500, 25, 0.002)
TUNABLE_PARAM(pawnhist_bonus_offset, 258, -500, 500, 50, 0.002)
TUNABLE_PARAM(pawnhist_bonus_max, 2537, 1500, 3500, 100, 0.002)

TUNABLE_PARAM(pawnhist_penalty_mult, 113, 0, 300, 15, 0.002)
TUNABLE_PARAM(pawnhist_penalty_offset, 949, 200, 1600, 70, 0.002)
TUNABLE_PARAM(pawnhist_penalty_max, 1247, 600, 2200, 80, 0.002)

TUNABLE_PARAM(cont_bonus_mult, 246, 1, 512, 25, 0.002)
TUNABLE_PARAM(cont_bonus_offset, 502, 300, 1000, 35, 0.002)
TUNABLE_PARAM(cont_bonus_max, 2477, 1500, 3500, 100, 0.002)

TUNABLE_PARAM(cont_penalty_mult, 75, 0, 250, 12, 0.002)
TUNABLE_PARAM(cont_penalty_offset, 838, 200, 1400, 60, 0.002)
TUNABLE_PARAM(cont_penalty_max, 919, 400, 1800, 70, 0.002)

TUNABLE_PARAM(capt_hist_bonus_mult, 294, 1, 512, 25, 0.002)
TUNABLE_PARAM(capt_hist_bonus_offset, -113, -512, 512, 50, 0.002)
TUNABLE_PARAM(capt_hist_bonus_max, 1498, 700, 2300, 80, 0.002)

TUNABLE_PARAM(capt_hist_penalty_mult, 310, 50, 550, 25, 0.002)
TUNABLE_PARAM(capt_hist_penalty_offset, 668, 0, 1200, 60, 0.002)
TUNABLE_PARAM(capt_hist_penalty_max, 3497, 1500, 5000, 175, 0.002)

// Continuation history weights
TUNABLE_PARAM(conthist_1ply_weight, 982, 0, 1536, 75, 0.002)
TUNABLE_PARAM(conthist_2ply_weight, 1150, 0, 1536, 75, 0.002)
TUNABLE_PARAM(conthist_4ply_weight, 473, 0, 1536, 75, 0.002)

// Correction History
TUNABLE_PARAM(pawn_corr_factor, 44, 1, 100, 5, 0.002)
TUNABLE_PARAM(nonpawn_corr_factor, 37, 1, 100, 5, 0.002)
TUNABLE_PARAM(cont_corr_1ply_factor, 32, 1, 100, 5, 0.002)
TUNABLE_PARAM(cont_corr_2ply_factor, 34, 1, 100, 5, 0.002)
TUNABLE_PARAM(cont_corr_4ply_factor, 35, 1, 100, 5, 0.002)
TUNABLE_PARAM(major_pieces_corr_factor, 33, 1, 100, 5, 0.002)

// Time Manager
TUNABLE_PARAM(tm_default_mtg, 27, 15, 45, 1.5, 0.002)
TUNABLE_PARAM(tm_increment_factor, 79, 20, 120, 5, 0.002)
TUNABLE_PARAM(tm_opt_time_factor, 87, 50, 130, 4, 0.002)
TUNABLE_PARAM(tm_max_time_factor, 77, 40, 100, 3, 0.002)

TUNABLE_PARAM(tm_pv_stability_base, 1316, 800, 1800, 50, 0.002)
TUNABLE_PARAM(tm_pv_stability_factor, 51, 10, 110, 5, 0.002)
TUNABLE_PARAM(tm_pv_stability_min_scale, 726, 400, 1200, 40, 0.002)

TUNABLE_PARAM(tm_score_stability_delta, 13, 0, 40, 2, 0.002)
TUNABLE_PARAM(tm_score_stability_base, 1361, 800, 1800, 50, 0.002)
TUNABLE_PARAM(tm_score_stability_factor, 51, 10, 110, 5, 0.002)
TUNABLE_PARAM(tm_score_stability_min_scale, 720, 400, 1200, 40, 0.002)

TUNABLE_PARAM(tm_node_spent_base, 1520, 1200, 1900, 35, 0.002)
TUNABLE_PARAM(tm_node_spent_factor, 1725, 1200, 2200, 50, 0.002)
TUNABLE_PARAM(tm_min_scale, 527, 250, 850, 30, 0.002)
TUNABLE_PARAM(tm_max_scale, 2665, 1800, 3400, 80, 0.002)

// Material scale
TUNABLE_PARAM(pawn_scaling_factor, 96, 0, 300, 15, 0.002)
TUNABLE_PARAM(knight_scaling_factor, 311, 0, 800, 40, 0.002)
TUNABLE_PARAM(bishop_scaling_factor, 327, 0, 800, 40, 0.002)
TUNABLE_PARAM(rook_scaling_factor, 546, 100, 1100, 50, 0.002)
TUNABLE_PARAM(queen_scaling_factor, 929, 200, 1800, 80, 0.002)
TUNABLE_PARAM(material_scaling_base, 25808, 24000, 28000, 200, 0.002)

// SEE values
TUNABLE_PARAM(pawn_see_value, 100, 60, 160, 5, 0.002)
TUNABLE_PARAM(knight_see_value, 308, 200, 400, 10, 0.002)
TUNABLE_PARAM(bishop_see_value, 293, 200, 400, 10, 0.002)
TUNABLE_PARAM(rook_see_value, 525, 400, 700, 15, 0.002)
TUNABLE_PARAM(queen_see_value, 994, 750, 1250, 25, 0.002)

// Move picker
TUNABLE_PARAM(mp_see_threshold_base, 66, 0, 150, 7.5, 0.002)
TUNABLE_PARAM(mp_killer1_bonus, 9070, 2500, 14500, 600, 0.002)

} // namespace minke::tunable
