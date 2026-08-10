#pragma once

#include "lqrbridge/transport/mmio/bus.hpp"
#include "lqrbridge/transport/mmio/mmio_transport.hpp"
#include "lqrbridge/types.hpp"
#include "lqrbridge/transport/transport.hpp"


#include <cassert>
#include <cstddef>
#include <cstdint>
  
namespace lqr {

    template <std::size_t Capacity>
    class MockBus {
        Reg reg_;
        std::array<Word, Capacity> bank_{};
        std::size_t cnt_ = 0;
        Generation gen_{};
        bool armed_ = false;
    public:
        explicit MockBus(Reg reg) noexcept : reg_(reg) {}

        void write32(std::size_t off, std::uint32_t v) noexcept {
            if (off == reg_.start) {
                cnt_ = 0; // fill starts
            } else if (off == reg_.data) {
                bank_[cnt_++] = v; // fill valid: write + increment
            } else if (off == reg_.commit) {
                armed_ = true; // arm, get ready for swap
            }
        }

        [[nodiscard]] std::uint32_t read32(std::size_t off) const noexcept {
            return (off == reg_.gen) ? gen_ : 0u;
        }

        // Test hook. Handle defered swap here
        void tick() noexcept {
            if (armed_) {
                ++gen_;
                armed_ = false;
            }
        }

        // Test inspection
        [[nodiscard]] Word at(std::size_t i) const noexcept {
            return bank_[i];
        }
        [[nodiscard]] std::size_t count() const noexcept {
            return cnt_;
        }
    };

    static_assert(Bus<MockBus<9>>);

}
