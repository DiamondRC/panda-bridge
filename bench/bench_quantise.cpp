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

template <std::size_t N>
void run(const char* label) {
    // Pre-generate test gains
    std::array<double, N> gains{};

    // Same linear-ramp spread between [-8, 8)
    // across all runs.
    for (std::size_t i = 0; i < N; ++i) {
        gains[i] = -8.0 + 16.0 * (double(i) / double(N)); 
    }
    std::array<Word, N> scratch{};

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
    std::print("{}: x{}  p50={}  p99={}  p99.9={}  max={}  ({} cyc/elem)\n",
        label, N, s.p50, s.p99, s.p999, s.max,
        double(s.p50) / double(N));
}



int main() {
    // 9 doubles = trivial case, close to floor
    // 900 dubs ~7.2kb => within cache
    // 901 to check tail at demand
    run<9>("small "); 
    run<900>("bulk  ");
    run<901>("bulk + tail  ");
}