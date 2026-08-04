#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "lqrbridge/fixed_point/format.hpp"
#include "lqrbridge/fixed_point/quantise.hpp"
#include "lqrbridge/publish.hpp"
#include "lqrbridge/transport/mock_transport.hpp"
#include "lqrbridge/types.hpp"

#include <array>
#include <bit>
#include <cstdint>

using lqr::publish, lqr::dequantise;
using lqr::MockTransport, lqr::gain_q, lqr::Rounding, lqr::Word;

TEST_CASE("Publish encodes and delivers a gain matrix") {
    MockTransport<9> mt;
    const std::array<double, 3> k{1.0, 0.5, -1.0};
    std::array<Word, 3> scratch{};

    const auto gen = publish(
        mt,
        k,
        scratch,
        gain_q,
        Rounding::HalfAway
    );

    CHECK(gen == 1);
    CHECK(mt.generation() == 1);

    const auto f = mt.active();
    REQUIRE(f.size() == 3);
    CHECK(f[0] == 0x02000000u); // 2^25
    CHECK(f[1] == 0x01000000u); // 2^24
    CHECK(
        dequantise(std::bit_cast<std::int32_t>(f[2]), gain_q) == 
        doctest::Approx(-1.0)
    );
}

TEST_CASE("Successive publishes advance generation") {
    MockTransport<9> mt;
    std::array<Word, 3> scratch{};

    const std::array<double, 3> k1{1.0, 1.0, 1.0};
    const std::array<double, 3> k2{2.0, 2.0, 2.0};

    // First matrix
    CHECK(
        publish(mt, k1, scratch, gain_q, Rounding::HalfAway) == 1
    );
    CHECK(mt.active()[0] == 0x02000000u); // 2^25
    
    // Second matrix
    CHECK(
        publish(mt, k2, scratch, gain_q, Rounding::HalfAway) == 2
    );
    CHECK(mt.generation() == 2);
    CHECK(mt.active()[0] == 0x04000000u); // 2^26, previous frame fully replaced
}
