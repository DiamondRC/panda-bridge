#pragma once
#include <atomic>
#include <cstdint>
#include <chrono>

namespace lqr::bench {
    // Monotonic CPU cycle count w/ ReaD Time Stamp Counter Processor id (RDTSC-P)
    // and lfence guards to isolate work for accurate measurement.

    [[nodiscard]] inline std::uint64_t now_cycles() noexcept {
        #if defined (__x86_64__)
            std::uint32_t lo, hi, aux;

            // RDTSC-P: serialises on retire of previous instructions,
            // returns the Time Stamp Counter + the CPU core ID
            __asm__ __volatile__("rdtscp" : "=a"(lo), "=d"(hi), "=c"(aux));

            // lfence: prevents later instructions from being pulled
            // above the work under analysis.
            __asm__ __volatile__("lfence" ::: "memory");

            return (static_cast<std::uint64_t>(hi) << 32) | lo;
        #else
            // TODO - wire ARM PMU
            // No unprivaliaged cycle coutner for ARM by deafult,
            // need to pathc kernal.

            // Portable fallback
            return static_cast<std::uint64_t>(
                std::chrono::steady_clock::now().time_since_epoch().count()
            );
        #endif
    }
}