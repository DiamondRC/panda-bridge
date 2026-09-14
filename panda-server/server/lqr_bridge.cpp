#include "lqr_bridge.h"
#include "lqr_resolve.h"

#include "lqrbridge/state_abi.hpp"
#include "lqrbridge/transport/mapped_region.hpp"
#include "lqrbridge/transport/mmio/hw_bus.hpp"
#include "lqrbridge/transport/mmio/mmio_transport.hpp"
#include "lqrbridge/publisher.hpp"
#include "lqrbridge/control/optimiser.hpp"
#include "lqrbridge/fixed_point/format.hpp"
#include "lqrbridge/control/state_source.hpp"
#include "lqrbridge/util/cpu_relax.hpp"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <ratio>
#include <thread>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <cerrno>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <limits>

extern "C" void log_message(const char *message, ...);

namespace {

    constexpr std::size_t N = 4; // test frame width
    constexpr std::size_t N_AX = 3; // axes
    constexpr int BRIDGE_CPU  = 1; // the isolated RT core
    constexpr int BRIDGE_PRIO = 80; // SCHED_FIFO priority 1..99
    constexpr int K_PAGE_SIZE = 4096; // vpages
    constexpr std::size_t STACK_PREFAULT_BYTES = 64 * 1024; // TODO - profile
    constexpr float STATE_SCALE = lqr::StateAbi::export_scale;
    constexpr std::size_t kReadFailReport = 8; // consec read fails -> probe EXPORT_STATUS
    constexpr double kCacheableRatioMax = 4.0; // window/heap read latency; >this => suspect

    std::thread bridge_thread; // bridge worker
    std::atomic<bool> bridge_stop{false}; // cooperative stop flag

    // Startup cacheability probe result: window/heap read-latency ratio * 1000
    // (-1 until probed). Read by lqr_bridge_cache_ratio_milli() for a status PV.
    std::atomic<std::int32_t> cache_ratio_milli{-1};

    // Configure the calling thread for RT. 
    // Exit immediately if we can't get the deterministic setup
    [[nodiscard]] bool configure_rt(void) {
        bool ok = true;

        // Pin to the isolated core
        cpu_set_t set; // CPU core bitmask
        CPU_ZERO(&set);
        CPU_SET(BRIDGE_CPU, &set);
        if (
            int rc = pthread_setaffinity_np(
                pthread_self(),
                sizeof(set),
                &set
            );
            rc != 0
        ) {
            log_message("LQR bridge: affinity CPU%d failed: %s",
                BRIDGE_CPU, std::strerror(rc));
            ok = false;
        } else {
            log_message("LQR bridge: pinned to CPU%d", BRIDGE_CPU);
        }

        // Real-time scheduling
        struct sched_param sp{};
        sp.sched_priority = BRIDGE_PRIO;
        if (
            int rc = pthread_setschedparam(
                pthread_self(),
                SCHED_FIFO,
                &sp
            );
            rc != 0
        ) {
            log_message("LQR bridge: SCHED_FIFO(%d) failed: %s",
                BRIDGE_PRIO, std::strerror(rc));
            ok = false;
        } else {
            log_message("LQR bridge: SCHED_FIFO prio %d", BRIDGE_PRIO);
        }

        // Lock all pages against demand paging
        if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) {
            log_message("LQR bridge: mlockall failed: %s", std::strerror(errno));
            ok = false;
        } else {
            log_message("LQR bridge: memory locked");
        }

        // Pre-fault stack pages so the hot loop never faults them in. Touching
        // this from configure_rt() backs the pages the loop + optimiser reuse;
        // mlockall(MCL_FUTURE) keeps them resident.
        unsigned char probe[STACK_PREFAULT_BYTES];
        for (std::size_t i = 0; i < sizeof(probe); i += K_PAGE_SIZE) {
            probe[i] = 0; // one byte per page
        }
        asm volatile("" :: "r"(probe) : "memory"); // defeat dead-store elision

        return ok;
    }

    // One batch: window/heap read-latency ratio.
    // Test if the mapped window is cacheable (~1) or if it silently went
    // non-cacheable (>>1) into DDR.
    // This would increase the cost of the computation by ~10x!
    [[nodiscard]] double window_ratio_once(
        const lqr::MappedRegion& region, std::size_t stamp_i) {
        constexpr int kIters = 4096;
        volatile std::uint32_t sink = 0;

        std::uint32_t heap = 0;
        volatile std::uint32_t* hp = &heap;
        sink = *hp; // warm
        const auto t0 = std::chrono::steady_clock::now();
        for (int i = 0; i < kIters; ++i) { sink = *hp; }
        const auto t1 = std::chrono::steady_clock::now();

        sink = region.word(stamp_i); // warm
        const auto t2 = std::chrono::steady_clock::now();
        for (int i = 0; i < kIters; ++i) { sink = region.word(stamp_i); }
        const auto t3 = std::chrono::steady_clock::now();
        (void) sink;

        const double heap_ns = std::chrono::duration<double, std::nano>(t1 - t0).count();
        const double win_ns = std::chrono::duration<double, std::nano>(t3 - t2).count();
        return heap_ns > 0.0 ? win_ns / heap_ns : 0.0;
    }

    // The fastest of N batches = least-perturbed = truest cache latency
    [[nodiscard]] double window_ratio(
        const lqr::MappedRegion& region, std::size_t stamp_i) {
        double best = std::numeric_limits<double>::infinity();
        for (int rep = 0; rep < 5; ++rep) {
            best = std::min(best, window_ratio_once(region, stamp_i));
        }
        return best;
    }

    // Pass coords by val => no lifetime dependancy on start's stack.
    void bridge_loop(lqr_coords c) {
        if (!configure_rt()) {
            log_message("LQR bridge: refusing to run without RT guarantees");
            return;
        }

        // Build out the transport chain from the resolved coords
        lqr::HwBus bus(c.block_base, c.block_number);
        lqr::Reg reg{.start = c.start, .data = c.data,
            .commit = c.commit, .gen = c.gen };
        lqr::MmioTransport<lqr::HwBus> transport(bus, reg);
        lqr::Publisher<lqr::MmioTransport<lqr::HwBus>, N> pub(transport);

        // tmp
        lqr::ConstantOptimiser<N> optimiser({1.0, -1.0, 0.5, -0.25});

        // Map the coherent state window
        auto region = lqr::MappedRegion::map(
            static_cast<std::uintptr_t>(c.state_phys),
            static_cast<std::size_t>(c.state_bytes)
        );
        if (!region) {
            log_message("LQR bridge: state map failed: %s",
                region.error().message().c_str());
            return; // no state => don't run the controller
        }

        // Mark the window "no valid export yet".
        // Write an odd seq + zero stamp so a stale/garbage boot value
        // can't be read as a consistent snapshot before the FPGA's first
        // export closes the bracket.
        constexpr std::size_t W = lqr::StateAbi::word_bytes;
        region->write_word(lqr::StateAbi::seq_off / W, 1u);
        region->write_word(lqr::StateAbi::stamp_off / W, 0u);

        // On boot check if the window is actually cacheable.
        // Warn loudly + publish a status if not.
        const double ratio = window_ratio(
            *region, lqr::StateAbi::stamp_off / W
        );
        cache_ratio_milli.store(
            static_cast<std::int32_t>(ratio * 1000.0), std::memory_order_relaxed
        );
        log_message("LQR bridge: state-window read latency %.1fx heap", ratio);
        if (ratio > kCacheableRatioMax) {
            log_message(
                "LQR bridge: WARNING state window looks NON-CACHEABLE "
                "(%.1fx): check no-map / O_SYNC / STRICT_DEVMEM, "
                "determinism + L2 lockdown are void", ratio
            );
        }

        lqr::SeqlockStateSource<lqr::MappedRegion, N_AX> source {
            std::move(*region), STATE_SCALE
        };

        // Busy-poll the servo tick on this dedicated core
        // Only re-optimise on a fresh snapshot (stamp advances).
        // Keep I/O off the hot path and report the tallies once, on stop.
        std::uint32_t last_stamp = 0;
        std::size_t published = 0;
        std::size_t dropped = 0;
        std::size_t read_fail = 0;
        while (!bridge_stop.load(std::memory_order_relaxed)) {
            if (source.peek_stamp() == last_stamp) {
                lqr::cpu_relax(); // no new export yet -> hold last K
                continue;
            }

            const auto op = source.read();
            if (!op) { // torn / stuck export
                lqr::cpu_relax();
                if (++read_fail >= kReadFailReport) {
                    const std::uint32_t st = bus.read32(c.export_status);
                    log_message(
                        "LQR bridge: seqlock read failed x%zu EXPORT_STATUS=%#x "
                        "(busy=%u err=%u overrun=%u)",
                        read_fail, st, st & 1u, (st >> 1) & 1u, (st >> 2) & 1u
                    );
                    read_fail = 0; // rate-limit the report
                }
                continue; // failure - do not advance last_stamp, do not publish
            }
            read_fail = 0;

            last_stamp = op->stamp;
            const auto k = optimiser.solve(*op); // update gains
            if (pub.publish(k, lqr::gain_q, lqr::Rounding::HalfAway)) {
                ++published;
            } else {
                ++dropped; // back-pressure: newest gains dropped
            }
        }
        log_message("LQR bridge: stopped, published=%zu dropped=%zu",
            published, dropped
        );
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
        c.block_base, c.block_number, c.start, c.data, c.commit, c.gen
    );

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

// Health seam for a status PV / supervisor.
extern "C" std::int32_t lqr_bridge_cache_ratio_milli(void) {
    return cache_ratio_milli.load(std::memory_order_relaxed);
}
