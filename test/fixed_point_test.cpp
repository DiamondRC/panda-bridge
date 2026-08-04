#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "lqrbridge/fixed_point/format.hpp"
#include "lqrbridge/fixed_point/quantise.hpp"
#include "lqrbridge/transport/mock_transport.hpp"
#include "lqrbridge/types.hpp"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <span>
#include <limits>

using lqr::quantise, lqr::dequantise, lqr::to_word, lqr::quantise_into;
using lqr::gain_q, lqr::Rounding, lqr::Word, lqr::MockTransport;

TEST_CASE("Q7.25 known codes") {
    CHECK(quantise(1.0, gain_q, Rounding::HalfAway) == 33554432); // 2^25
    CHECK(quantise(0.5, gain_q, Rounding::HalfAway) == 16777216); // 2^24
    CHECK(quantise(-1.0, gain_q, Rounding::HalfAway) == -33554432);
    CHECK(quantise(0.0, gain_q, Rounding::HalfAway) == 0);
}

TEST_CASE("Saturation clamps to the field range") {
    constexpr double inf = std::numeric_limits<double>::infinity();
    constexpr double qnan = std::numeric_limits<double>::quiet_NaN();

    // Range Q7.25 is ~[-64, 64)
    // (-)1e9 far outside -> should clamp to lims
    CHECK(
        quantise(1e9, gain_q, Rounding::HalfAway) == 
            static_cast<std::int32_t>(gain_q.max_code())
    );
    CHECK(
        quantise(-1e9, gain_q, Rounding::HalfAway) == 
            static_cast<std::int32_t>(gain_q.min_code())
    );

    // +/-inf -> limits, NaN -> 0
    CHECK(
        quantise(inf, gain_q, Rounding::HalfAway) == 
            static_cast<std::int32_t>(gain_q.max_code())
    );
    CHECK(
        quantise(-inf, gain_q, Rounding::HalfAway) == 
            static_cast<std::int32_t>(gain_q.min_code())
    );
    CHECK(
        quantise(qnan, gain_q, Rounding::HalfAway) == 0 // Nan -> 0
    );
}

TEST_CASE("Rounding modes diverge on half-LSB tie") {
    const double half_lsb = std::ldexp(1.0, -26); // scaled == 0.5
    const double two_and_half_lsb = std::ldexp(5.0, -26); // scaled == 2.5

    CHECK(quantise(half_lsb, gain_q, Rounding::HalfAway) == 1); // away
    CHECK(quantise(half_lsb, gain_q, Rounding::HalfEven) == 0); // even

    CHECK(quantise(two_and_half_lsb, gain_q, Rounding::HalfAway) == 3); // away
    CHECK(quantise(two_and_half_lsb, gain_q, Rounding::HalfEven) == 2); // even
}

TEST_CASE("Dequantise inverts quantise within one LSB") {
    const double res = std::ldexp(1.0, -gain_q.frac); // 2^-25

    for (const double g : {0.0, 1.0, -1.0, 0.5, -12.375, 63.9}) {
        const auto code = quantise(g, gain_q, Rounding::HalfAway);

        CHECK(std::abs(dequantise(code, gain_q) - g) <= res);
    }
}

TEST_CASE("Gains enter transport and round-trip") {
    const std::array<double, 3> k{1.0, 0.5, -1.0};
    std::array<Word, 9> buf{}; // a 3x3 store, uses the first 3 slots
    quantise_into(k, gain_q, Rounding::HalfAway, std::span{buf}.first(3));

    MockTransport<9> mt;
    mt.stage(std::span<const Word>{buf.data(), 3});
    mt.commit();

    const auto f = mt.active();
    REQUIRE(f.size() == 3);

    CHECK(f[0] == 0x02000000u); // 2^25 as a device word
    CHECK(f[1] == 0x01000000u); // 2^24
    CHECK(dequantise(std::bit_cast<std::int32_t>(f[0]), gain_q) == 
        doctest::Approx(1.0));
    CHECK(dequantise(std::bit_cast<std::int32_t>(f[2]), gain_q) == 
        doctest::Approx(-1.0));
}
