#pragma once
#include "lqrbridge/fixed_point/format.hpp"
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace lqr {
    [[nodiscard]] inline std::int32_t quantise(
        double x,
        QFormat q,
        Rounding mode
    ) noexcept {
        const double scaled = std::ldexp(x, q.frac);

        const std::int64_t rounded = (mode == Rounding::HalfAway) ?
        // Env independant - always ties away...
        std::llround(scaled) :
        // ...or ties even
        static_cast<std::int64_t>(std::nearbyint(scaled));

        const std::int64_t code = std::clamp(rounded, q.min_code(), q.max_code());
        
        return static_cast<std::int32_t>(code);
    }
}