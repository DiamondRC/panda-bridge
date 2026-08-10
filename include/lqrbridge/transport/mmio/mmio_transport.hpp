#pragma once
#include "lqrbridge/transport/mmio/barrier.hpp"
#include "lqrbridge/transport/mmio/bus.hpp"
#include "lqrbridge/transport/transport.hpp"
#include "lqrbridge/types.hpp"

#include <cstddef>
  
namespace lqr {

    // Byte offsets into the block's register window.
    struct Reg {
        // GAINS_START
        std::size_t start; // Pulse resets fill pointer
        // GAINS_DATA
        std::size_t data; // Each write streams one word
        // COMMIT
        std::size_t commit; // Pulse arms the deferred swap
        // GEN (read)
        std::size_t gen; // Generation tag
        
        // GAINS_LENGTH - informational
        // skipped for latency
    }; 

    template <Bus B>
    class MmioTransport {
        B bus_;
        Reg reg_;
        Generation expected_ = 0;
    public:
        MmioTransport(B bus, Reg reg) noexcept : bus_(bus), reg_(reg) {}

        void stage(Frame f) noexcept {
            // Start pulse low
            bus_.write32(reg_.start,  0);

            // Write data
            for (Word w : f) {
                bus_.write32(reg_.data,  w);
            }
        }

        [[nodiscard]] Generation commit() noexcept {
            // Arm swap on posted write.
            bus_.write32(reg_.commit, 0);

            // Commit and other writes ordered before
            // callers poll.
            mmio_barrier();
            
            // Expected gen after commit lands.
            // Count this locally instead of stalling the
            // core with reads of GEN.
            return ++expected_;
        }

        [[nodiscard]] Generation generation() const noexcept {
            // Single read, polled by the caller.
            return bus_.read32(reg_.gen);
        }

        B& bus() noexcept { return bus_; } // Test hook

    };
}