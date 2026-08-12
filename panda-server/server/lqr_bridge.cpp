#include "lqr_bridge.h"
#include "lqr_resolve.h"

#include "lqrbridge/transport/mmio/hw_bus.hpp"
#include "lqrbridge/transport/mmio/mmio_transport.hpp"
#include "lqrbridge/publisher.hpp"
#include "lqrbridge/fixed_point/format.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>

extern "C" void log_message(const char *message, ...);

namespace {

    constexpr std::size_t N = 4; // test frame width

    std::thread bridge_thread; // bridge worekr
    std::atomic<bool> bridge_stop{false}; // cooperative stop flag

    // Pass coords by val => no lifetime dependancy to start's stack c
    void bridge_loop(lqr_coords c) {
        // Build out the transport chain from the resolved coords
        lqr::HwBus bus(c.block_base, c.block_number);
        lqr::Reg reg{.start = c.start, .data = c.data,
            .commit = c.commit, .gen = c.gen };
        lqr::MmioTransport<lqr::HwBus> transport(bus, reg);
        lqr::Publisher<lqr::MmioTransport<lqr::HwBus>, N> pub(transport);

        // tmp
        std::array<double, N> gains = { 1.0, -1.0, 0.5, -0.25 };

        while (!bridge_stop.load(std::memory_order_relaxed)) {
            auto gen = pub.publish(gains, lqr::gain_q, lqr::Rounding::HalfAway);
            if (gen) {
                log_message("LQR bridge: published test frame, gen=%u",
                    static_cast<unsigned>(*gen));
            } else {
                log_message("LQR bridge: publish back-pressured (unexpected on first write)");
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // stand in for the servo-period
        }
    }
}

extern "C" void lqr_bridge_start(void)
{       
    lqr_coords c;
    if (!lqr_resolve(&c)) {
        log_message("LQR bridge: LQR resolve failed");
        return;
    }
    log_message(
        "LQR bridge: resolved base=%u number=%u start=%u data=%u commit=%u gen=%u",
        c.block_base, c.block_number, c.start, c.data, c.commit, c.gen);

    // Create the LQR bridge in it's own thread
    bridge_stop.store(false);
    bridge_thread = std::thread(bridge_loop, c);
}

extern "C" void lqr_bridge_stop(void)
{
    // Halt the LQR brideg thread
    if (bridge_thread.joinable()) {
        bridge_stop.store(true);
        bridge_thread.join();
    }
    log_message("LQR bridge: stopped");
}
