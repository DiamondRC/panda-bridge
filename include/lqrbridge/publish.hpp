#pragma once
#include "lqrbridge/fixed_point/format.hpp"
#include "lqrbridge/fixed_point/quantise.hpp"
#include "lqrbridge/transport/transport.hpp"
#include "lqrbridge/types.hpp"
 
#include <cassert>
#include <span>

namespace lqr {

    // Stage and commit calculated LQR gains,
    // returns the expected vals the FPGA should
    // reach after the deferred swap.
    //
    // TODO - quantise directly into the inactive bank
    //  and eliminate working scratch area.
    template <Transport T>
    Generation publish(
        T& transport,
        std::span<const double> gains, // 16 bytes => pass by val
        // TODO - make reused scratch own type
        std::span<Word> scratch, // allocate once then reuse
        QFormat q,
        Rounding mode
    ) noexcept {
        assert(scratch.size() == gains.size());

        // Send to caller's buffer (gains -> scratch)
        quantise_into(gains, q, mode, scratch);
        // Words to inactive bank (scratch -> bank)
        transport.stage(scratch);

        // Never block this hot-path
        return transport.commit();
    }

}