#pragma once
#include "lqrbridge/fixed_point/format.hpp"
#include "lqrbridge/types.hpp"
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <bit>
#include <span>
#include <cassert>

namespace lqr {
    [[nodiscard]] inline Word to_word(std::int32_t code) noexcept {
        return std::bit_cast<Word>(code); // 2's comp bits
    }

    [[nodiscard]] inline std::int32_t quantise(
        double x,
        QFormat q,
        Rounding mode
    ) noexcept {
        assert(std::isfinite(x) && "quantise: non-finite input");

        const auto lo = static_cast<double>(q.min_code());
        const auto hi = static_cast<double>(q.max_code());
        const double scaled = std::ldexp(x, q.frac);

        // Saturate in float domain before rounding
        const double bounded = std::isnan(scaled) ? 0.0 : 
            std::clamp(scaled, lo, hi);

        return static_cast<std::int32_t>(
            mode == Rounding::HalfAway ?
            std::llround(bounded) :
            static_cast<std::int64_t>(std::nearbyint(bounded))
        );
    }

    [[nodiscard]] inline double dequantise(
        std::int32_t code,
        QFormat q
    ) noexcept {
        return std::ldexp(static_cast<double>(code), -q.frac);
    }

    inline void quantise_into(
        std::span<const double> gains,
        QFormat q,
        Rounding mode,
        std::span<Word> out
    ) noexcept {
        // BRAM gains laid row-major (row*N + col)
        for (std::size_t i = 0; i < gains.size(); ++i) {
            out[i] = to_word(quantise(gains[i], q, mode));
        }
    }
}