#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "lqrbridge/fixed_point/format.hpp"
#include "lqrbridge/fixed_point/quantise.hpp"
#include "lqrbridge/publisher.hpp"
#include "lqrbridge/transport/mmio/mmio_transport.hpp"
#include "lqrbridge/types.hpp"
#include "mmio/mock_bus.hpp"

#include <array>
#include <bit>
#include <cstdint>

using lqr::Reg, lqr::MockBus, lqr::MmioTransport, lqr::Publisher;
using lqr::gain_q, lqr::Rounding, lqr::Word, lqr::dequantise;

namespace {
    constexpr Reg kReg{.start = 0x00, .data = 0x04, .commit = 0x08, .gen = 0x0C};

    using Bus3 = MockBus<3>;
    using Tx3  = MmioTransport<Bus3>;
    using Pub3 = Publisher<Tx3, 3>;

    Pub3 make() { return Pub3(Tx3(Bus3(kReg), kReg)); }
}

TEST_CASE("First publish encodes and streams the gain matrix") {
    auto pub = make();
    const std::array<double, 3> k{1.0, 0.5, -1.0};

    const auto g = pub.publish(k, gain_q, Rounding::HalfAway);

    REQUIRE(g.has_value());
    CHECK(*g == 1u); // expected post-swap gen

    CHECK(pub.transport().bus().at(0) == 0x02000000u); // 2^25
    CHECK(pub.transport().bus().at(1) == 0x01000000u); // 2^24
    CHECK(dequantise(
        std::bit_cast<std::int32_t>(
            pub.transport().bus().at(2)
        ), gain_q) == 
        doctest::Approx(-1.0)
    );

    CHECK(pub.transport().generation() == 0u); // deferred, not swapped yet
}

TEST_CASE("Confirmed swap lets the next publish through and advances gen") {
    auto pub = make();
    const std::array<double, 3> k1{1.0, 1.0, 1.0};
    const std::array<double, 3> k2{2.0, 2.0, 2.0};

    const auto g1 = pub.publish(
        k1, gain_q, Rounding::HalfAway
    );
    REQUIRE(g1.has_value());
    CHECK(*g1 == 1u);

    pub.transport().bus().tick(); // servo pass boundary: swap lands

    const auto g2 = pub.publish(
        k2, gain_q, Rounding::HalfAway
    );
    REQUIRE(g2.has_value()); // back-pressure check passed
    CHECK(*g2 == 2u);
    CHECK(pub.transport().bus().at(0) == 0x04000000u); // 2^26, frame replaced

    pub.transport().bus().tick();
    CHECK(pub.transport().generation() == 2u);
}

TEST_CASE("Back-pressure blocks a publish over an unconsumed bank") {
    auto pub = make();
    const std::array<double, 3> k1{1.0, 1.0, 1.0};
    const std::array<double, 3> k2{2.0, 2.0, 2.0};

    const auto g1 = pub.publish(
        k1, gain_q, Rounding::HalfAway
    );
    REQUIRE(g1.has_value()); // first publish always stages

    // Previous swap did not land!
    const auto g2 = pub.publish(
        k2, gain_q, Rounding::HalfAway);

    CHECK_FALSE(g2.has_value()); // back-pressured, did not stage
    CHECK(pub.transport().bus().at(0) == 0x02000000u); // bank still holds k1, not k2
}
