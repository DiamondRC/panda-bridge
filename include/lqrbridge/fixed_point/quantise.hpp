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
#include <experimental/simd>
#include <array>

// Helper implementations
namespace detail {
    namespace stdx = std::experimental;

    // NaN test for doubles
    [[nodiscard]] inline double nan_to_zero(double scaled, double clamp) noexcept{
        return (scaled == scaled) ? clamp : 0.0;
    }

    // Cross machine-architecture ABI tag to encode SIMD implementation.
    // nan_to_zero matches a double SIMD of any ABI now,
    // including the PandA A9 scalar fallback.
    template <class Abi>
    [[nodiscard]] inline stdx::simd<double, Abi>
    nan_to_zero(
        stdx::simd<double, Abi> scaled,
        stdx::simd<double, Abi> clamp
    ) noexcept {
        // Mask assignment, compute then process result
        stdx::where(scaled != scaled, clamp) = 0.0;
        return clamp;
    }
}

namespace lqr {
    // SIMD w/ C++17 (GCC 15.3 doesn't ship C++26 std::simd)
    namespace stdx = std::experimental;

    // SIMD vector type - native = widest machine supports
    using vd = stdx::native_simd<double>;


    [[nodiscard]] inline Word to_word(std::int32_t code) noexcept {
        return std::bit_cast<Word>(code); // 2's comp bits
    }

    // For kernal - value in FP, clamp + round only
    // Handles the double tail as well as the SIMD body
    template <Rounding Mode, class T>
    [[nodiscard]] inline T quantise_scaled(
        T scaled,
        T lo,
        T hi
    ) noexcept {
        // Arguement dependant lookup will find stdx versions for SIMD whilst doubles get std.
        // When T = double ADl finds nothing => use std
        // T = vd uses stdx SIMD versions.
        using std::min, std::max, std::nearbyint, std::trunc, std::copysign;

        // Saturate in float domain before rounding.
        // Clamps inf and keeps llround/nearbyint UB-free.
        const T clamped = min(max(scaled, lo), hi);
        const T bounded = detail::nan_to_zero(scaled, clamped); // NaN is not equal to itself

        // Apply rounding after FP scaling
        if constexpr (Mode == Rounding::HalfAway) {
            // half-away - nudge by std/stx half branchlessly then truncate
            return trunc(bounded + copysign(T(0.5), bounded));
        } else {
            // half-even
            return nearbyint(bounded);
        }
    }

    // Public interface for testing
    template <Rounding Mode>
    [[nodiscard]] inline std::int32_t quantise(double x, QFormat q) noexcept {
        const double scale = std::ldexp(1.0, q.frac);

        // Call kernal to execute background logic
        return static_cast<std::int32_t>(quantise_scaled<Mode>(
            x * scale,
            static_cast<double>(q.min_code()),
            static_cast<double>(q.max_code())
        ));
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
        const double scale = std::ldexp(1.0, q.frac);
        const auto lo = static_cast<double>(q.min_code());
        const auto hi = static_cast<double>(q.max_code());

        // Lanes per vector to freeze array size
        constexpr std::size_t WIDTH = vd::size(); 
        // Fill all lanes with the scale value
        const vd vscale = scale, vlo = lo, vhi = hi;
        
        const std::size_t n = gains.size();
        std::size_t i = 0;
        for (; i + WIDTH <= n; i += WIDTH) {
            vd x; // Vector body
            x.copy_from(&gains[i], stdx::element_aligned); // copy gains in

            // Quantise the gains
            const vd r = quantise_scaled<Mode>(x * vscale, vlo, vhi);

            // Truncate the packed doubles back to ints
            const auto ri = stdx::static_simd_cast<std::int32_t>(r);

            // Create store - copy per lane int32 -> uint32
            stdx::static_simd_cast<Word>(ri).copy_to(&out[i], stdx::element_aligned);
        }

        // BRAM gains laid row-major (row * N + col)
        for (; i < n; ++i) {
            out[i] = to_word(
                static_cast<std::int32_t>(
                    quantise_scaled<Mode>(gains[i] * scale, lo, hi)
                )
            );
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