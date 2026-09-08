#pragma once
#include "lqrbridge/control/operating_point.hpp"
#include "lqrbridge/state_abi.hpp"
#include "lqrbridge/transport/word_window.hpp"

#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace lqr {
    // Seqlock reader
    template<WordWindow Win, std::size_t N_AX>
    class SeqlockStateSource {
        Win win_;
        float scale_; // FPGA sends in fixed point

        // Scratch the returned spans view into
        std::array<float, N_AX> pos_{};
        std::array<float, N_AX> vel_{};
        std::array<float, N_AX> setp_{};
        std::array<float, N_AX> setv_{};

        static constexpr std::size_t W = StateAbi::word_bytes;
        static constexpr std::size_t seq_i = StateAbi::seq_off / W;
        static constexpr std::size_t stamp_i = StateAbi::stamp_off / W;

        [[nodiscard]] float to_float(std::size_t word) const noexcept {
            return static_cast<float>(
                std::bit_cast<std::int32_t>(win_.word(word))
            ) * scale_;
        }

        void decode() noexcept {
            for (std::size_t k = 0; k < N_AX; ++k) {
                const std::size_t a = StateAbi::axis_off(k) / W;
                pos_[k] = to_float(a + StateAbi::pos_off / W);
                vel_[k] = to_float(a + StateAbi::vel_off / W);
                setp_[k] = to_float(a + StateAbi::setp_off / W);
                setv_[k] = to_float(a + StateAbi::setv_off / W);
            }
        }
    public:
        SeqlockStateSource(Win win, float scale) noexcept :
        win_(std::move(win)), scale_(scale) {}

        [[nodiscard]] OperatingPoint read() noexcept {
            std::uint32_t stamp = 0;
            while (true) {
                const std::uint32_t s1 = win_.word(seq_i); // seq state before
                if (s1 & 1u) { continue; } // odd -> mid-export

                std::atomic_thread_fence(std::memory_order_acquire); // DMB on ARM
                stamp = win_.word(stamp_i);
                decode();
                std::atomic_thread_fence(std::memory_order_acquire);

                if (win_.word(seq_i) == s1) { break; } // unchanged => consistent
            }

            return OperatingPoint {
                .stamp = stamp,
                .pos = pos_,
                .vel = vel_,
                .set_p = setp_,
                .set_v = setv_
            };
        }

        // Cheap freshness probe: one aligned load of the stamp word, no seqlock.
        // A busy-poll uses this to skip the full read() until state advances;
        // consistency still comes from read().
        [[nodiscard]] std::uint32_t peek_stamp() const noexcept {
            return win_.word(stamp_i);
        }
    };
}