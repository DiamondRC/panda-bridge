#pragma once
#include "lqrbridge/fixed_point/format.hpp"
#include "lqrbridge/fixed_point/quantise.hpp"
#include "lqrbridge/transport/transport.hpp"
#include "lqrbridge/types.hpp"
 
#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <optional>
#include <span>

namespace lqr {

    // Stage and commit calculated LQR gains into a Transport, tracking the
    // deferred swap. Owns a reused scratch bank; back-pressures on the
    // previous swap so it never overwrites a bank the FPGA hasn't consumed.
    //
    // TODO - quantise directly into the inactive bank (DMA transport only)
    // and eliminate the working scratch area.
    template <Transport T, std::size_t N>
    class Publisher {
        T transport_;
        std::array<Word, N> scratch_{};
        Generation last_ = 0;
        bool pending_ = false;
        std::atomic<std::size_t> dropped_ = 0;
    public:
        explicit Publisher(T transport) noexcept : transport_(transport) {}

        [[nodiscard]] std::optional<Generation> publish(
            std::span<const double> gains,
            QFormat q,
            Rounding mode
        ) noexcept {
            assert(gains.size() == N);

            // Don't overwrite banks the PandA FPGA hasn't consumed
            if (
                pending_ && confirm(transport_, last_, 0) == 
                Swap::TimedOut
            ) {
                // publish skipped by backpressure
                dropped_.fetch_add(1, std::memory_order_relaxed);
                return std::nullopt;
            }

            // Quantise into the owned scratch bank (gains -> scratch)
            quantise_into(gains, q, mode, scratch_);
            // Stream the words to the inactive bank (scratch -> bank)
            transport_.stage(scratch_);
            last_ = transport_.commit();
            pending_ = true;
            return last_;
        }

        T& transport() noexcept {
            return transport_;
        }

        [[nodiscard]] std::size_t dropped() const noexcept {
            return dropped_.load(std::memory_order_relaxed);
        }
    };
}