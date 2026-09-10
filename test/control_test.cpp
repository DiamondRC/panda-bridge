#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "lqrbridge/control/operating_point.hpp"
#include "lqrbridge/control/optimiser.hpp"
#include "lqrbridge/control/seqlock_state_source.hpp"
#include "lqrbridge/control/state_source.hpp"
#include "lqrbridge/state_abi.hpp"
#include "lqrbridge/transport/word_window.hpp"
#include "lqrbridge/fixed_point/format.hpp"
#include "lqrbridge/fixed_point/quantise.hpp"
#include "lqrbridge/publisher.hpp"
#include "lqrbridge/transport/mock_transport.hpp"
#include "lqrbridge/types.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>

using lqr::ConstantOptimiser, lqr::ConstantStateSource, lqr::OperatingPoint;
using lqr::Optimiser, lqr::StateSource;
using lqr::Publisher, lqr::MockTransport, lqr::Word;
using lqr::gain_q, lqr::Rounding, lqr::dequantise;
using lqr::SeqlockStateSource, lqr::StateAbi, lqr::WordWindow;

// Array-backed word window.
template <std::size_t Words>
struct ArrayWindow {
    std::array<std::uint32_t, Words> buf{};
    std::uint32_t& operator[](std::size_t i) noexcept { return buf[i]; }
    [[nodiscard]] std::uint32_t word(std::size_t i) const noexcept { return buf[i]; }
};
static_assert(WordWindow<ArrayWindow<4>>);

// Shows an ODD seq on the first peek, committed EVEN after -> forces one retry.
template <std::size_t Words>
struct OneRetryWindow {
    std::array<std::uint32_t, Words> buf{};
    mutable bool first_seq = true;
    std::uint32_t& operator[](std::size_t i) noexcept { return buf[i]; }

    [[nodiscard]] std::uint32_t word(std::size_t i) const noexcept {
        if (i == StateAbi::seq_off / 4 && first_seq) {
            first_seq = false;
            return buf[i] | 1u; // writer mid-export
        }
        return buf[i];
    }
};

// Each stub must actually model the seam it stands in for.
static_assert(StateSource<ConstantStateSource<3>>);
static_assert(StateSource<SeqlockStateSource<ArrayWindow<24>, 3>>);
static_assert(Optimiser<ConstantOptimiser<4>, 4>);

TEST_CASE("ConstantStateSource hands back seeded payload as views") {
    const std::array<float, 3> pos{1.0f, 2.0f, 3.0f};
    const std::array<float, 3> vel{4.0f, 5.0f, 6.0f};
    const std::array<float, 3> setp{7.0f, 8.0f, 9.0f};
    const std::array<float, 3> setv{10.0f, 11.0f, 12.0f};
    ConstantStateSource<3> src(pos, vel, setp, setv);

    const OperatingPoint op = src.read();

    REQUIRE(op.pos.size() == 3);
    REQUIRE(op.set_v.size() == 3);
    CHECK(op.pos[0] == 1.0f);
    CHECK(op.pos[2] == 3.0f);
    CHECK(op.vel[1] == 5.0f);
    CHECK(op.set_p[0] == 7.0f);
    CHECK(op.set_v[2] == 12.0f);
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
        {9.0f, 9.0f, 9.0f},
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
        {0.0f, 0.0f, 0.0f},
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

TEST_CASE("SeqlockStateSource decodes a committed snapshot") {
    constexpr std::size_t N = 3;
    constexpr std::size_t Words = StateAbi::buffer_bytes(N) / 4;
    ArrayWindow<Words> win;

    // One committed state snapshot
    const auto put = [&](std::size_t byte_off, std::int32_t v) {
        win[byte_off / 4] = std::bit_cast<std::uint32_t>(v);
    };
    win[StateAbi::seq_off / 4] = 2u; // even => committed
    win[StateAbi::stamp_off / 4] = 1u; // stamp = seq / 2

    for (std::size_t k = 0; k < N; ++k) {
        const std::size_t a = StateAbi::axis_off(k);
        const int ki = static_cast<int>(k);

        put(a + StateAbi::pos_off, 100 + ki);
        put(a + StateAbi::vel_off, 200 + ki);
        put(a + StateAbi::setp_off, 300 + ki);
        put(a + StateAbi::setv_off, -(400 + ki)); // -ve
    }

    SeqlockStateSource<ArrayWindow<Words>, N> src { // unit scale
        std::move(win),
        1.0f
    };
    const OperatingPoint op = src.read();

    CHECK(op.stamp == 1u);
    for (std::size_t k = 0; k < N; ++k) {
        const int ki = static_cast<int>(k);

        CHECK(op.pos[k] == static_cast<float>(100 + ki));
        CHECK(op.vel[k] == static_cast<float>(200 + ki));
        CHECK(op.set_p[k] == static_cast<float>(300 + ki));
        CHECK(op.set_v[k] == static_cast<float>(-(400 + ki)));
    }
}

TEST_CASE("SeqlockStateSource applies the fixed-point scale") {
    constexpr std::size_t N = 1;
    constexpr std::size_t Words = StateAbi::buffer_bytes(N) / 4;
    ArrayWindow<Words> win;

    win[StateAbi::seq_off / 4] = 2u;
    win[StateAbi::stamp_off / 4] = 1u;
    const std::size_t a = StateAbi::axis_off(0);
    win[(a + StateAbi::pos_off) / 4] = std::bit_cast<std::uint32_t>(
        std::int32_t{128}
    );

    // Q6
    SeqlockStateSource<ArrayWindow<Words>, N> src {
        std::move(win),
        StateAbi::export_scale
    };
    CHECK(src.read().pos[0] == doctest::Approx(2.0)); // 128 * 2^-6
}

TEST_CASE("SeqlockStateSource retries past an in-flight generation") {
    constexpr std::size_t N = 1;
    constexpr std::size_t Words = StateAbi::buffer_bytes(N) / 4;
    OneRetryWindow<Words> win;

    win[StateAbi::seq_off / 4] = 2u; // committed under the odd first peek
    win[StateAbi::stamp_off / 4] = 7u;
    const std::size_t a = StateAbi::axis_off(0);
    win[(a + StateAbi::pos_off) / 4] = std::bit_cast<std::uint32_t>(
        std::int32_t{42}
    );

    SeqlockStateSource<OneRetryWindow<Words>, N> src{std::move(win), 1.0f};
    const OperatingPoint op = src.read(); // odd first => spins once => committed

    CHECK(op.stamp == 7u); // returns the committed generation, not in-flight
    CHECK(op.pos[0] == 42.0f);
}

TEST_CASE("SeqlockStateSource peek_stamp reads the stamp without a full read") {
    constexpr std::size_t N = 1;
    constexpr std::size_t Words = StateAbi::buffer_bytes(N) / 4;
    ArrayWindow<Words> win;
    win[StateAbi::seq_off / 4] = 2u;
    win[StateAbi::stamp_off / 4] = 5u;

    SeqlockStateSource<ArrayWindow<Words>, N> src{std::move(win), 1.0f};
    CHECK(src.peek_stamp() == 5u); // cheap probe, no seqlock retry
}
