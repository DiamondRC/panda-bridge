#pragma once

#include <concepts>
#include <cstdint>
#include "lqrbridge/types.hpp"

namespace lqr {
    // Swap version tag
    using Generation = std::uint32_t;
    inline constexpr unsigned kGenWidth = 16; // GEN_W
    // Keep counter 16 bit within 32 bit container =>
    // ms overflow in testing versus ~6hrs for 32 bit
    //
    // Shift the 1st to the 16th bit and flip + all lower bits to 0x0000FFFF,
    // type width remains as the single point of truth not hardcoded val.
    inline constexpr Generation kGenMask = (Generation{1} << kGenWidth) - 1;

    enum class Swap {
        Confirmed,
        TimedOut
    };

    // Transport contract
    // Mirrors LQR VHDL to prevent tearing.
    template <typename T>
    concept Transport = requires(T t, Frame frame) {
        // Write frame to inactive bank
        // Must never throw to lock determinism!
        { t.stage(frame)} noexcept -> std::same_as<void>;

        // Publish the generation we expect PandA FPGA to
        // reach after swap.
        {t.commit()} noexcept -> std::same_as<Generation>;

        // Last generation acknoledged to confirm swap processed.
        {t.generation()} noexcept -> std::same_as<Generation>;
    };

    template <Transport T>
    [[nodiscard]] Swap confirm(
        const T& t,
        Generation expected,
        std::size_t max_polls
    ) noexcept {
        for (std::size_t i = 0; i < max_polls; ++i) {
            if (t.generation() == expected) {
                return Swap::Confirmed;
            }
        }
        return t.generation() == expected ? Swap::Confirmed : Swap::TimedOut;
    }

}
