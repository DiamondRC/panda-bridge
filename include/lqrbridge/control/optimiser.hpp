#pragma once
#include "lqrbridge/control/operating_point.hpp"
#include <array>
#include <concepts>
#include <cstddef>
#include <span>

namespace lqr {
    template <typename O, std::size_t N> 
    concept Optimiser = requires(O o, const OperatingPoint& op) {
        // Solver for the gain frame at this operating point.
        // Returns a view into optimiser-owned storage so warm-start
        // persists across ticks + no allocation on the hotpath.
        {o.solve(op)} noexcept -> std::same_as<std::span<const double, N>>;
    };

    template <std::size_t N>
    class ConstantOptimiser {
        std::array<double, N> k_;
    public:
        explicit constexpr ConstantOptimiser(std::array<double, N> k) noexcept : k_(k) {}

        // Stub until real algorithm
        [[nodiscard]] std::span<const double, N> solve(const OperatingPoint&) noexcept {
            return k_;
        }
    };
}