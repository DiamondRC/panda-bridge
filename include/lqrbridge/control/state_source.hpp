#pragma once
#include "lqrbridge/control/operating_point.hpp"
  
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

namespace lqr {
    template <typename S>
    concept StateSource = requires(S s) {
        // Read lastest state snapshot.
        {s.read()} noexcept -> std::same_as<OperatingPoint>;
    };

    // Stub: fixed states + fake monotonic counter
    template <std::size_t N_AX>
    class ConstantStateSource {
            std::array<float, N_AX> pv_{};
            std::array<float, N_AX> sp_{};
            std::uint32_t stamp_ = 0;
        public:
            constexpr ConstantStateSource() noexcept = default;

            constexpr ConstantStateSource(
                std::array<float, N_AX> pv,
                std::array<float, N_AX> sp
            ) noexcept : pv_(pv), sp_(sp) {}

            [[nodiscard]] OperatingPoint read() noexcept {
                ++stamp_; // Fresh snapshot per tick
                // return by value, copies the views not the data
                return OperatingPoint{.stamp = stamp_, .pv = pv_, .sp = sp_};
            }
    };
}