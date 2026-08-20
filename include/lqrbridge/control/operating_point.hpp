#pragma once
#include <cstdint>
#include <span>

namespace lqr {
    struct OperatingPoint {
        std::uint32_t stamp = 0;
        std::span<const float> pv; // measured [pos, vel, ...]
        std::span<const float> sp; // setpoints [set_p, set_v, ...]
    };
}