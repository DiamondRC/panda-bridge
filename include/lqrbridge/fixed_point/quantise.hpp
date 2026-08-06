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

    template <Rounding Mode>
    [[nodiscard]] inline std::int32_t quantise(
        double x,
        QFormat q
    ) noexcept {
        const auto lo = static_cast<double>(q.min_code());
        const auto hi = static_cast<double>(q.max_code());
        const double scaled = std::ldexp(x, q.frac);

        // Saturate in float domain before rounding.
        // Clamps inf and keeps llround/nearbyint UB free.
        const double bounded = std::isnan(scaled) ? 0.0 : 
            std::clamp(scaled, lo, hi);

        
        if constexpr (Mode == Rounding::HalfAway) {
            return static_cast<std::int32_t>(std::llround(bounded));
        } else {
            return static_cast<std::int32_t>(std::nearbyint(bounded));
        }
    }

    [[nodiscard]] inline double dequantise(
        std::int32_t code,
        QFormat q
    ) noexcept {
        return std::ldexp(static_cast<double>(code), -q.frac);
    }


    template<Rounding Mode>
    inline void quantise_into(
        std::span<const double> gains,
        QFormat q,
        std::span<Word> out
    ) noexcept {
        // Enforce I/O
        assert(gains.size() == out.size());

        // BRAM gains laid row-major (row * N + col)
        for (std::size_t i = 0; i < gains.size(); ++i) {
            out[i] = to_word(quantise<Mode>(gains[i], q));
        }
    }

    inline void quantise_into(
        std::span<const double> gains,
        QFormat q,
        Rounding mode,
        std::span<Word> out
    ) noexcept {
        if (mode == Rounding::HalfAway) {
            quantise_into<Rounding::HalfAway>(gains, q, out);
        } else {
            quantise_into<Rounding::HalfEven>(gains, q, out);
        }
    }
}