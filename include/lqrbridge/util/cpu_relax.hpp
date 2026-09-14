#pragma once

namespace lqr {
    // Spin hint for a busy-wait on a dedicated RT core:
    // yields the pipeline (and SMT sibling) without sleeping.
    // A compiler barrier too, so the surrounding polled loads
    // are not hoisted out of the spin.
    inline void cpu_relax() noexcept {
        #if defined(__arm__) || defined(__aarch64__)
            asm volatile("yield" ::: "memory");
        #elif defined(__x86_64__) || defined(__i386__)
            asm volatile("pause" ::: "memory");
        #else
            asm volatile("" ::: "memory");
        #endif
    }
}
