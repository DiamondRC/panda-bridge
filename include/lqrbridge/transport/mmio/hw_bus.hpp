#pragma once

// Compiled into the forked panda_server
#include "lqrbridge/transport/mmio/bus.hpp"
#include <cstddef>
#include <cstdint>

// The vendored server's register accessors.
// Version pinned => no drift
extern "C" {
    void hw_write_register(
        unsigned block_base,
        unsigned block_number,
        unsigned reg,
        std::uint32_t value
    );

    std::uint32_t hw_read_register(
        unsigned block_base,
        unsigned block_number,
        unsigned reg
    );
}

namespace lqr {
    class HwBus {
        unsigned block_base_; // Block-type number
        unsigned block_number_; // Instance
    public:
        HwBus(unsigned block_base, unsigned block_number) noexcept :
            block_base_(block_base), block_number_(block_number) {}

        [[nodiscard]] std::uint32_t read32(std::size_t reg) const noexcept {
            return hw_read_register(
                block_base_,
                block_number_,
                static_cast<unsigned>(reg)
            );
        }

        void write32(std::size_t reg, std::uint32_t val) noexcept {
            hw_write_register(
                block_base_,
                block_number_,
                static_cast<unsigned>(reg),
                val
            );
        }

    };

    static_assert(Bus<HwBus>);
}