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
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <cerrno>
#include <cstring>

extern "C" void log_message(const char *message, ...);

namespace {

    constexpr std::size_t N = 4; // test frame width
    constexpr int BRIDGE_CPU  = 1; // the isolated RT core
    constexpr int BRIDGE_PRIO = 80; // SCHED_FIFO priority 1..99
    constexpr int K_PAGE_SIZE = 4096; // vpages

    std::thread bridge_thread; // bridge worekr
    std::atomic<bool> bridge_stop{false}; // cooperative stop flag

    void configure_rt(void) {
        // Pin LQR optimiser to isolated core
        cpu_set_t set; // CPU core bitmask
        CPU_ZERO(&set); // clear btis
        CPU_SET(BRIDGE_CPU, &set); // set nth bit - mask with only our core
        if (
            int rc = pthread_setaffinity_np(
                pthread_self(),
                sizeof(set),
                &set
            );
            rc != 0
        ) {
            log_message(
                "LQR bridge: affinity CPU%d failed: %s",
                BRIDGE_CPU,
                std::strerror(rc)
            );
        } else {
            log_message(
                "LQR bridge: pinned to CPU%d",
                BRIDGE_CPU
            );
        }

        // Real-time scheduling
        struct sched_param sp{};
        sp.sched_priority = BRIDGE_PRIO;
        if (
            int rc = pthread_setschedparam(
                pthread_self(),
                SCHED_FIFO,
                &sp
            )
        ) {
            log_message(
                "LQR bridge: SCHED_FIFO(%d) failed: %s (normal sched)",
                BRIDGE_PRIO,
                std::strerror(rc)
            );
        } else {
            log_message("LQR bridge: SCHED_FIFO prio %d", BRIDGE_PRIO);
        }

        // Lock all pages
        if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
            log_message(
                "LQR bridge: mlockall failed: %s",
                std::strerror(errno)
            );
        } else {
            log_message("LQR bridge: memory locked");
        }

        // Pre-fault stack pages so the hot loop never faults them in
        unsigned char probe[16 * K_PAGE_SIZE];
        for (std::size_t i = 0; i < sizeof(probe); i += K_PAGE_SIZE) {
            // touch just one byte per page
            probe[i] = 0;
        }

        // Compute the probes address and ahnd to the ASM
        // The ASM is never discarded + allowed to r/w and memory =>
        // all stores are commited and array cannot be cached/altered
        asm volatile("" :: "r"(probe) : "memory");
    }

    // Pass coords by val => no lifetime dependancy to start's stack c
    void bridge_loop(lqr_coords c) {
        // Configure thread state
        configure_rt();

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
                log_message("LQR bridge: publish back-pressured (GEN not advanced)");
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
    bridge_thread = std::thread(bridge_loop, c); // spawn
}

extern "C" void lqr_bridge_stop(void)
{
    // Halt the LQR bridge thread
    if (bridge_thread.joinable()) {
        bridge_stop.store(true);
        bridge_thread.join();
    }
    log_message("LQR bridge: stopped");
}
