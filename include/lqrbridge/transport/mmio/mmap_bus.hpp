#pragma once

#include "lqrbridge/transport/mmio/bus.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
  
namespace lqr {

    // View over an mmap 32-bit register window.
    class MmapBus {
        volatile std::uint32_t* base_; // Pointer to the window base
    public:
        explicit MmapBus(volatile std::uint32_t* base) noexcept : base_(base) {}

        [[nodiscard]] std::uint32_t read32(std::size_t off) const noexcept {
            // off in bytes but base_ is u32 => indexing scales by 4.
            assert(off % sizeof(std::uint32_t) == 0);
            return base_[off / sizeof(std::uint32_t)];
        }
        void write32(std::size_t off, std::uint32_t val) noexcept {
            assert(off % sizeof(std::uint32_t) == 0);
            base_[off / sizeof(std::uint32_t)] = val;
        }
    };

    // Fail immediately if wrong spec
    static_assert(Bus<MmapBus>);

}