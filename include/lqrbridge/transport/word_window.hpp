#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>

namespace lqr {
    template <typename T>
    concept WordWindow = requires(const T w, std::size_t i) {
        // A volitile load of the ith word
        { w.word(i) } noexcept -> std::same_as<std::uint32_t>;
    };
}