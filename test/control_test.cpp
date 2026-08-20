#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "lqrbridge/control/operating_point.hpp"
#include "lqrbridge/control/optimiser.hpp"
#include "lqrbridge/control/state_source.hpp"
#include "lqrbridge/fixed_point/format.hpp"
#include "lqrbridge/fixed_point/quantise.hpp"
#include "lqrbridge/publisher.hpp"
#include "lqrbridge/transport/mock_transport.hpp"
#include "lqrbridge/types.hpp"

#include <array>
#include <bit>
#include <cstdint>

using lqr::ConstantOptimiser, lqr::ConstantStateSource, lqr::OperatingPoint;
using lqr::Optimiser, lqr::StateSource;
using lqr::Publisher, lqr::MockTransport, lqr::Word;
using lqr::gain_q, lqr::Rounding, lqr::dequantise;

// Each stub must actually model the seam it stands in for.
static_assert(StateSource<ConstantStateSource<3>>);
static_assert(Optimiser<ConstantOptimiser<4>, 4>);

TEST_CASE("ConstantStateSource hands back seeded payload as views") {
    const std::array<float, 3> pv{1.0f, 2.0f, 3.0f};
    const std::array<float, 3> sp{4.0f, 5.0f, 6.0f};
    ConstantStateSource<3> src(pv, sp);

    const OperatingPoint op = src.read();

    REQUIRE(op.pv.size() == 3);
    REQUIRE(op.sp.size() == 3);
    CHECK(op.pv[0] == 1.0f);
    CHECK(op.pv[2] == 3.0f);
    CHECK(op.sp[0] == 4.0f);
    CHECK(op.sp[2] == 6.0f);
}

TEST_CASE("ConstantStateSource stamp advances every read") {
    ConstantStateSource<3> src;

    CHECK(src.read().stamp == 1u);
    CHECK(src.read().stamp == 2u);
    CHECK(src.read().stamp == 3u);
}

TEST_CASE("ConstantOptimiser returns fixed gains and ignores the operating point") {
    ConstantOptimiser<4> opt({1.0, -1.0, 0.5, -0.25});
    ConstantStateSource<3> src(
        {9.0f, 9.0f, 9.0f},
        {9.0f, 9.0f, 9.0f}
    );

    const auto k = opt.solve(src.read());

    REQUIRE(k.size() == 4);
    CHECK(k[0] == 1.0);
    CHECK(k[1] == -1.0);
    CHECK(k[2] == 0.5);
    CHECK(k[3] == -0.25);

    // A different operating point yields identical K.
    ConstantStateSource<3> other(
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    );
    const auto k2 = opt.solve(other.read());
    CHECK(k2[0] == 1.0);
    CHECK(k2[3] == -0.25);
}

TEST_CASE("source -> optimiser -> publisher streams the quantised K") {
    ConstantOptimiser<4> opt({1.0, -1.0, 0.5, -0.25});
    ConstantStateSource<3> src;
    Publisher<MockTransport<4>, 4> pub{MockTransport<4>{}};

    const OperatingPoint op = src.read();
    const auto k = opt.solve(op);
    const auto gen = pub.publish(k, gain_q, Rounding::HalfAway);

    REQUIRE(gen.has_value());
    CHECK(*gen == 1u); // first publish always lands

    const auto frame = pub.transport().active();
    REQUIRE(frame.size() == 4);
    CHECK(frame[0] == 0x02000000u); // 1.0 in Q7.25 == 2^25
    CHECK(dequantise(std::bit_cast<std::int32_t>(frame[3]), gain_q)
          == doctest::Approx(-0.25));
}
