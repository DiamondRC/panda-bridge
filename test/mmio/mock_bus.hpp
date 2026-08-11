#pragma once

#include "lqrbridge/transport/mmio/bus.hpp"
#include "lqrbridge/transport/mmio/mmio_transport.hpp"
#include "lqrbridge/transport/transport.hpp" 
#include "lqrbridge/types.hpp"

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace lqr {

    template <std::size_t Capacity>
    class MockBus {
        Reg reg_;
        std::array<Word, Capacity> bank_{};
        std::size_t cnt_ = 0;
        Generation gen_ = 0;
        bool armed_ = false;
    public:
        explicit MockBus(Reg reg) noexcept : reg_(reg) {}

        void write32(std::size_t off, std::uint32_t v) noexcept {
            if (off == reg_.start) {
                cnt_ = 0; // fill starts, reset pointer
            } else if (off == reg_.data) {
                assert(cnt_ < Capacity);
                bank_[cnt_++] = v; // valid, write then increment
            } else if (off == reg_.commit) {
                armed_ = true; // arm and await swap
            }
        }

        [[nodiscard]] std::uint32_t read32(std::size_t off) const noexcept {
            return (off == reg_.gen) ? gen_ : 0u;
        }

        // Armed deferred swap at servo-pass boundary
        void tick() noexcept {
            if (armed_) {
                ++gen_;
                armed_ = false;
            }
        }

        // For inspection
        [[nodiscard]] Word at(std::size_t i) const noexcept { return bank_[i]; }
        [[nodiscard]] std::size_t count() const noexcept { return cnt_; }
        [[nodiscard]] bool armed() const noexcept { return armed_; }
    };

    static_assert(Bus<MockBus<9>>);
}
