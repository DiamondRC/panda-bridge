#pragma once

namespace lqr::bench {

    // Forces compiler to hold register/memory value so
    // deadcode analysis doesn't spoil my benchmarking accuracy.
    template<typename T>
    
    inline void do_not_optimise(const T& value) noexcept {
        // Empty ASM but the input can live in a reg/memory
        // as we 'might need it' => kept alive.
        __asm__ __volatile__("" : : "r,m"(value) : "memory");
    }

    inline void clobber_memory() noexcept {
        // Mark all of the memory as having changed here ->
        // compiler will not reorder across this point
        __asm__ __volatile__("" : : : "memory");
    }

}