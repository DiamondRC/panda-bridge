#pragma once
#include <cstddef>

namespace lqr {
    struct StateAbi {
        static constexpr std::size_t word_bytes = 4;
        static constexpr std::size_t beat_bytes = 8;
        static constexpr std::size_t line_bytes = 32; // A9 cache line

        // States exported from the FPGA
        static constexpr std::size_t export_frac = 6;
        static constexpr float export_scale = 1.0f / static_cast<float>(1u << export_frac);

        // Line 0: seq word + allignment padding
        static constexpr std::size_t seq_off = 0;

        // Line 1 onwards: payload
        static constexpr std::size_t payload_off = line_bytes; // 0x20
        static constexpr std::size_t stamp_off = payload_off; // 0x20
        static constexpr std::size_t axis0_off = payload_off + beat_bytes; // 0x28
        static constexpr std::size_t axis_stride = 16; // 4 words per axis

        // Sub-offsets within the one axis
        static constexpr std::size_t pos_off = 0;
        static constexpr std::size_t vel_off = 4;
        static constexpr std::size_t setp_off = 8;
        static constexpr std::size_t setv_off = 12;

        // locate each axis
        static constexpr std::size_t axis_off(std::size_t k) noexcept {
            return axis0_off + k * axis_stride;
        }
        // Footprint
        static constexpr std::size_t buffer_bytes(std::size_t n_ax) noexcept {
            const std::size_t used = axis0_off + n_ax * axis_stride;
            return (used + line_bytes - 1) / line_bytes * line_bytes;
        }
    };

    // Do not let this fall out of sync with the VHDL!
    static_assert(StateAbi::payload_off == 32);
    static_assert(StateAbi::axis0_off == 40);
    static_assert(StateAbi::axis_off(0) == 40);
    static_assert(StateAbi::axis_off(1) == 56);
    static_assert(StateAbi::setv_off == 12);
}