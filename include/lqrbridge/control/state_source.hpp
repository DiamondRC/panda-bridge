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
            std::array<float, N_AX> pos_{};
            std::array<float, N_AX> vel_{};
            std::array<float, N_AX> setp_{};
            std::array<float, N_AX> setv_{};
            std::uint32_t stamp_ = 0;
        public:
            constexpr ConstantStateSource() noexcept = default;

            constexpr ConstantStateSource(
                std::array<float, N_AX> pos,
                std::array<float, N_AX> vel,
                std::array<float, N_AX> setp,
                std::array<float, N_AX> setv
            ) noexcept : pos_(pos), vel_(vel), setp_(setp), setv_(setv) {}

            [[nodiscard]] OperatingPoint read() noexcept {
                ++stamp_; // Fresh snapshot per tick
                // return by value, copies the views not the data
                return OperatingPoint{
                    .stamp = stamp_,
                    .pos = pos_, .vel = vel_, .set_p = setp_, .set_v = setv_
                };
            }
    };
}