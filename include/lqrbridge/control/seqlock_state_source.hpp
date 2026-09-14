#pragma once
#include "lqrbridge/control/operating_point.hpp"
#include "lqrbridge/state_abi.hpp"
#include "lqrbridge/transport/word_window.hpp"
#include "lqrbridge/util/cpu_relax.hpp"

#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <optional>

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

        // Bounded: a failed export burst that leaves seq odd should
        // not wedge this RT thread!
        // Return nullopt after kMaxReadAttempts and let the caller
        // hold last-K / consult EXPORT_STATUS.
        static constexpr std::size_t kMaxReadAttempts = 8;

        [[nodiscard]] std::optional<OperatingPoint> read() noexcept {
            for (std::size_t attempt = 0; attempt < kMaxReadAttempts; ++attempt) {
                const std::uint32_t s1 = win_.word(seq_i); // seq state before
                
                // odd -> mid-export
                if (s1 & 1u) {
                    cpu_relax();
                    continue;
                }

                std::atomic_thread_fence(std::memory_order_acquire); // DMB on ARM
                const std::uint32_t stamp = win_.word(stamp_i);
                decode();
                std::atomic_thread_fence(std::memory_order_acquire);

                if (win_.word(seq_i) == s1) { // unchanged => consistent
                    return OperatingPoint {
                        .stamp = stamp,
                        .pos = pos_,
                        .vel = vel_,
                        .set_p = setp_,
                        .set_v = setv_
                    };
                }
                cpu_relax(); // writer collided mid-read, retry
            }

            // no consistent snapshot within the bound
            return std::nullopt;
        }

        // Cheap freshness check:
        // one aligned load of the stamp word, no seqlock.
        [[nodiscard]] std::uint32_t peek_stamp() const noexcept {
            return win_.word(stamp_i);
        }
    };
}