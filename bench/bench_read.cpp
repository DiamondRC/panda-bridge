//! Measure the compute floor on x86

#include "cycles.hpp"
#include "do_not_optimise.hpp"
#include "measure.hpp"
#include "lqrbridge/control/seqlock_state_source.hpp"
#include "lqrbridge/state_abi.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <print>
#include <utility>

using lqr::SeqlockStateSource, lqr::StateAbi, lqr::OperatingPoint;
using lqr::bench::measure, lqr::bench::do_not_optimise, lqr::bench::clobber_memory;

// In-memory word window
template <std::size_t Words>
struct BenchWindow {
    std::array<std::uint32_t, Words> buf{};
    std::uint32_t& operator[](std::size_t i) noexcept { return buf[i]; }
    [[nodiscard]] std::uint32_t word(std::size_t i) const noexcept { return buf[i]; }
};

// A source over one committed snapshot.
template <std::size_t N_AX>
auto make_source() {
    constexpr std::size_t Words = StateAbi::buffer_bytes(N_AX) / 4;
    BenchWindow<Words> win;
    win[StateAbi::seq_off / 4] = 2u; // even => committed
    win[StateAbi::stamp_off / 4] = 1u;

    for (std::size_t k = 0; k < N_AX; ++k) {
        const std::size_t a = StateAbi::axis_off(k);
        win[(a + StateAbi::pos_off) / 4] = static_cast<std::uint32_t>(100 + k);
        win[(a + StateAbi::vel_off) / 4] = static_cast<std::uint32_t>(200 + k);
        win[(a + StateAbi::setp_off) / 4] = static_cast<std::uint32_t>(300 + k);
        win[(a + StateAbi::setv_off) / 4] = static_cast<std::uint32_t>(400 + k);
    }
    return SeqlockStateSource<BenchWindow<Words>, N_AX>{
        std::move(win), StateAbi::export_scale};
}

template <std::size_t N_AX>
void run_read(const char* tag) {
    auto source = make_source<N_AX>();
    const auto s = measure([&] {
        const OperatingPoint op = source.read();
        do_not_optimise(op);
        clobber_memory(); // force the decode stores + a re-read each iter
    });
    std::print("read {} axes={:2}  p50={}  p99={}  p99.9={}  max={}\n",
        tag, N_AX, s.p50, s.p99, s.p999, s.max);
}

template <std::size_t N_AX>
void run_peek(const char* tag) {
    auto source = make_source<N_AX>();
    const auto s = measure([&] {
        const std::uint32_t st = source.peek_stamp();
        do_not_optimise(st);
        clobber_memory();
    });
    std::print("peek {} axes={:2}  p50={}  p99={}  p99.9={}  max={}\n",
        tag, N_AX, s.p50, s.p99, s.p999, s.max);
}

int main() {
    // A/B: the busy-poll idle gate.
    std::print("A/B: idle-spin gate\n");
    run_peek<3>("gate");
    run_read<3>("full");

    // Decode compute floor vs axis count.
    std::print("read() decode floor vs axes\n");
    run_read<3>("    ");
    run_read<6>("    ");
    run_read<12>("    ");
}
