#include "cycles.hpp"

#include <cstdint>
#include <print>

int main() {
    const std::uint64_t a = lqr::bench::now_cycles();
    const std::uint64_t b = lqr::bench::now_cycles();

    // These reads will have the RDTSC-P/lframe lag
    std::print("cycles: {} -> {}  (delta {})\n", a, b, b - a);
}