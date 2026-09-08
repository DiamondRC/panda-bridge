#pragma once
#include <cstdint>
#include <span>

namespace lqr {
    struct OperatingPoint {
        std::uint32_t stamp = 0;
        std::span<const float> pos; // measured position / axis
        std::span<const float> vel; // measured velocity / axis
        std::span<const float> set_p; // setpoint position / axis
        std::span<const float> set_v; // setpoint velocity / axis
    };
}