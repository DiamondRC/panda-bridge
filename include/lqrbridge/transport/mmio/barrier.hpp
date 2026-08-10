#pragma once
  
namespace lqr {

    // System-domain barrier for MMIO ordering.
    // ARM: Data Memory Barrier (DMB) in the system domain (SY)
    // x86: Compiler barrier only 
    inline void mmio_barrier() noexcept {
        #if defined(__aarch64__) || defined(__arm__)
            __asm__ __volatile__("dmb sy" ::: "memory");
        #else 
            // Compiler barrier, TSO does the rest.
            __asm__ __volatile__("" ::: "memory");
        #endif
    }

}