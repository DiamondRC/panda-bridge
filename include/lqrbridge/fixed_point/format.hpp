#pragma once
#include <cstdint>

namespace lqr {

    struct QFormat {
        int width; // Includes sign
        int frac;

        constexpr int int_bits() const noexcept {
            return width - frac;
        }
        // int64_t to guard widths
        constexpr std::int64_t max_code() const noexcept {
            return (std::int64_t{1} << (width - 1)) - 1;
        }
        constexpr std::int64_t min_code() const noexcept {
            return -(std::int64_t{1} << (width - 1));
        }
    };

    inline constexpr QFormat gain_q{.width = 32, .frac = 25};

    // Rounding type
    enum class Rounding { 
        HalfAway,
        HalfEven
    };

}