#pragma once
#include "lqrbridge/fixed_point/format.hpp"
#include "lqrbridge/types.hpp"
#include <cmath>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <bit>
#include <span>
#include <cassert>

namespace lqr {
    [[nodiscard]] inline Word to_word(std::int32_t code) noexcept {
        return std::bit_cast<Word>(code); // 2's comp bits
    }

    // For keneral - value in FP, claim + round only
    template <Rounding Mode>
    [[nodiscard]] inline std::int32_t quantise_scaled(
        double scaled,
        double lo,
        double hi
    ) noexcept {
        // Saturate in float domain before rounding.
        // Clamps inf and keeps llround/nearbyint UB-free.
        const double clamped = std::min(std::max(scaled, lo), hi);
        const double bounded = (scaled == scaled) ? clamped : 0.0; // NaN is not equal to itself

        // Apply rounding after FP scaling
        if constexpr (Mode == Rounding::HalfAway) {
            return static_cast<std::int32_t>(std::llround(bounded));
        } else {
            return static_cast<std::int32_t>(std::nearbyint(bounded));
        }
    }

    // Public interface for testing
    template <Rounding Mode>
    [[nodiscard]] inline std::int32_t quantise(double x, QFormat q) noexcept {
        const double scale = std::ldexp(1.0, q.frac);

        // Call kernal to execute background logic
        return quantise_scaled<Mode>(x * scale,
            static_cast<double>(q.min_code()),
            static_cast<double>(q.max_code()));
    } 

    template<Rounding Mode>
    inline void quantise_into(
        std::span<const double> gains,
        QFormat q,
        std::span<Word> out
    ) noexcept {
        // Enforce I/O
        assert(gains.size() == out.size());

        // Pre-calculate the limits + scale factor
        const double scaled = std::ldexp(1.0, q.frac);
        const auto lo = static_cast<double>(q.min_code());
        const auto hi = static_cast<double>(q.max_code());

        // BRAM gains laid row-major (row * N + col)
        for (std::size_t i = 0; i < gains.size(); ++i) {
            out[i] = to_word(quantise_scaled<Mode>(gains[i] * scaled, lo, hi));
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

    [[nodiscard]] inline double dequantise(
        std::int32_t code,
        QFormat q
    ) noexcept {
        return std::ldexp(static_cast<double>(code), -q.frac);
    }
}