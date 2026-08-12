#include "lqr_bridge.h"
#include "lqr_resolve.h"

#include "lqrbridge/transport/mmio/hw_bus.hpp"
#include "lqrbridge/transport/mmio/mmio_transport.hpp"
#include "lqrbridge/publisher.hpp"
#include "lqrbridge/fixed_point/format.hpp"

#include <array>
#include <cstddef>

extern "C" void log_message(const char *message, ...);

extern "C" void lqr_bridge_start(void)
{
    struct lqr_coords c;
    if (!lqr_resolve(&c)) {
        log_message("LQR bridge: LQR resolve failed");
        return;
    }
    log_message(
        "LQR bridge: resolved base=%u number=%u start=%u data=%u commit=%u gen=%u",
        c.block_base, c.block_number, c.start, c.data, c.commit, c.gen);

    // Build out the transport chain from the resolved coords
    lqr::HwBus bus(c.block_base, c.block_number);
    lqr::Reg reg{.start = c.start, .data = c.data,
        .commit = c.commit, .gen = c.gen };
    lqr::MmioTransport<lqr::HwBus> transport(bus, reg);

    // Pulish a gain for testing
    constexpr std::size_t N = 4;
    lqr::Publisher<lqr::MmioTransport<lqr::HwBus>, N> pub(transport);

    std::array<double, N> gains = { 1.0, -1.0, 0.5, -0.25 };
    auto gen = pub.publish(gains, lqr::gain_q, lqr::Rounding::HalfAway);
    if (gen) {
        log_message("LQR bridge: published test frame, gen=%u",
            static_cast<unsigned>(*gen));
    } else {
        log_message("LQR bridge: publish back-pressured (unexpected on first write)");
    }
}

extern "C" void lqr_bridge_stop(void)
{
    log_message("LQR bridge: stop (skeleton)");
}