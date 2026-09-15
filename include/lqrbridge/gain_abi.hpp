#pragma once
#include <cstddef>

namespace lqr {
    // Frozen gain-frame shape.
    // This *must* match the VHDL side!
    //
    // GAIN_CNT = M * n_int(N, M, ref, phi, uprev, setpoint, affine)
    //
    struct GainAbi {
        static constexpr std::size_t m = 3; // outputs
        static constexpr std::size_t n_state = 9; // 3 axes x (state+vel+prev)
        static constexpr std::size_t ref = 6; // sp_orders 2 x 3 axes
        static constexpr std::size_t phi = 0; // extra features
        static constexpr std::size_t n_int = // GAIN_CNT
            n_state + phi + m + ref + 1;
        static constexpr std::size_t count = m * n_int;
    };

    inline constexpr std::size_t kGainCount = GainAbi::count;

    // Do not let this fall out of sync with the VHDL!
    static_assert(GainAbi::n_int == 19);
    static_assert(kGainCount == 57);
}
