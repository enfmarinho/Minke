/*
 *  Copyright (c) 2024 Eduardo Marinho <eduardo.nestor.marinho@proton.me>
 *
 *  Licensed under the MIT License.
 *  See the LICENSE file in the project root for more information.
 */

#ifndef WDL_H
#define WDL_H

#include <cmath>
#include <utility>

#include "types.h"

inline std::pair<double, double> wdl_params(int material_count) {
    // TODO these params were generated with terrible data, regenerate it
    constexpr double as[] = {-227.94583896, 747.68060207, -1001.65274826, 924.95005977};
    constexpr double bs[] = {-199.22343617, 563.09782966, -384.92825183, 246.99147853};

    double m = std::clamp(material_count, 17, 78) / 58.0;
    double a = (((as[0] * m + as[1]) * m + as[2]) * m) + as[3];
    double b = (((bs[0] * m + bs[1]) * m + bs[2]) * m) + bs[3];

    return {a, b};
}

inline ScoreType win_rate_model(ScoreType score, int material_count) {
    const auto [a, b] = wdl_params(material_count);
    return std::round(1000.0 / (1.0 + std::exp((a - static_cast<double>(score)) / b)));
}

inline ScoreType normalize_score(ScoreType score, int material_count) {
    if (score == 0 || std::abs(score) > MATE_FOUND)
        return score;

    auto [a, b] = wdl_params(material_count);

    return std::round(static_cast<double>(score) * 100.0 / a);
}

#endif // #ifndef WDL_H
