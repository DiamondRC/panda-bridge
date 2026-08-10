#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
  
namespace lqr {

    // Bus is a 32-bit AXI register window.
    // R/W one alligned word at a byte offset from the window base.
    template <typename B>
    concept Bus = requires(B b, std::size_t off, std::uint32_t val) {
        { b.read32(off)} noexcept -> std::same_as<std::uint32_t>;
        {b.write32(off, val)} noexcept -> std::same_as<void>;
    };

}