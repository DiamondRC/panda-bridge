#pragma once
#include "cycles.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace lqr::bench {
    
    // Hold percentiles etc for histogram
    struct Stats {
        std::uint64_t min, p50, p99, p999, max;
        std::size_t n;
    };


    // Warm up process then time work once per sample.
    template <typename F>
    Stats measure(
        F&& body,
        std::size_t samples = 100'000,
        std::size_t warmup = 2'000
    ) {
        for (std::size_t i = 0; i < warmup; ++i) {
            body(); // Prime cache / PLT / branch predictor
        }

        // Allocate outwith the hotpath
        std::vector<std::uint64_t> d(samples);


        for (std::size_t i = 0; i < samples; ++i) {
            const auto t0 = now_cycles();
            body();
            const auto t1 = now_cycles();
            d[i] = t1 - t0;
        }

        std::ranges::sort(d);
        const auto at = [&](double p) {
            return d[
                static_cast<std::size_t>(p * static_cast<double>(d.size() - 1))
            ];
        };
        return { d.front(), at(0.50), at(0.99), at(0.999), d.back(), d.size()};
    }


}