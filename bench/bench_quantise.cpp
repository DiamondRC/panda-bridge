#include "cycles.hpp"
#include "do_not_optimise.hpp"
#include "measure.hpp"
#include "lqrbridge/fixed_point/format.hpp"
#include "lqrbridge/fixed_point/quantise.hpp"
#include "lqrbridge/types.hpp"

#include <array>
#include <cstdint>
#include <print>
#include <span>

using lqr::Word, lqr::quantise_into, lqr::Rounding, lqr::gain_q, lqr::bench::measure,
lqr::bench::do_not_optimise;

int main() {
    // Initialise test gains
    const std::array<double, 9> gains{
        1.0, 0.5, -1.0,
        3.0, 0.4, -1.2,
        4.2, 3.1, -2.3
    };
    std::array<Word, 9> scratch{};

    // Warm up the hot-path
    const auto s = measure(
        [&] {
            quantise_into(
                gains,
                gain_q,
                Rounding::HalfAway,
                scratch
            );
            do_not_optimise(scratch);
        }
    );

    // Inspect the warming run-distribution
    std::print("Work was: x9  p50={}  p99={}  p99.9={}  max={}  (n={})\n",
         s.p50, s.p99, s.p999, s.max, s.n);


    // First measurement
    const std::uint64_t a = lqr::bench::now_cycles();

    // Do some work
    quantise_into(
        gains, 
        gain_q,
        Rounding::HalfAway,
        std::span{scratch}.first(9)
    );

    // Complete measurement
    const std::uint64_t b = lqr::bench::now_cycles();

    // These reads will have the RDTSC-P/lframe lag
    std::print("cycles: {} -> {}  (delta {})\n", a, b, b - a);
}