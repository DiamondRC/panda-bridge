#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "lqrbridge/transport/mmio/mmio_transport.hpp"
#include "lqrbridge/transport/transport.hpp"
#include "lqrbridge/types.hpp"
#include "mmio/fake_bus.hpp"

#include <array>
#include <cstddef>

using lqr::Reg, lqr::MmioTransport, lqr::MockBus, lqr::Word, lqr::Generation;

// MmioTransport driving the MockBus must satisfy the Transport contract.
static_assert(lqr::Transport<MmioTransport<MockBus<9>>>);

namespace {
    // Distinct, non-overlapping register offsets.
    constexpr Reg kReg{.start = 0x00, .data = 0x04, .commit = 0x08, .gen = 0x0C};

    template <std::size_t Cap>
    MmioTransport<MockBus<Cap>> make() {
        return MmioTransport<MockBus<Cap>>(MockBus<Cap>(kReg), kReg);
    }
}

TEST_CASE("Stage streams words in order and resets the fill pointer") {
    auto mt = make<9>();
    const std::array<Word, 3> frame{0x11u, 0x22u, 0x33u};

    mt.stage(frame);

    CHECK(mt.bus().count() == frame.size());
    CHECK(mt.bus().at(0) == frame[0]);
    CHECK(mt.bus().at(1) == frame[1]);
    CHECK(mt.bus().at(2) == frame[2]);
}

TEST_CASE("Commit returns correct gen and swap defers") {
    auto mt = make<9>();
    const std::array<Word, 3> frame{0x11u, 0x22u, 0x33u};

    mt.stage(frame);
    const Generation expected = mt.commit();

    CHECK(expected == 1u);
    CHECK(mt.bus().armed()); // armed, not yet swapped
    CHECK(mt.generation() == 0u); // GEN tag has not advanced on commit

    mt.bus().tick(); // servo pass boundary

    CHECK_FALSE(mt.bus().armed());
    CHECK(mt.generation() == expected); // now the eagle (swap) has landed!
}

TEST_CASE("Generation tracks across multiple commit/tick cycles") {
    auto mt = make<9>();
    const std::array<Word, 2> frame{0xAAu, 0xBBu};

    const Generation e1 = mt.commit(); // no stage needed to bump gen
    mt.bus().tick();
    CHECK(e1 == 1u);
    CHECK(mt.generation() == 1u);

    mt.stage(frame);
    const Generation e2 = mt.commit();
    mt.bus().tick();
    CHECK(e2 == 2u);
    CHECK(mt.generation() == 2u);
}

TEST_CASE("Re-stage without a commit overwrites from index 0") {
    auto mt = make<9>();
    const std::array<Word, 3> a{0x01u, 0x02u, 0x03u};
    const std::array<Word, 2> b{0x77u, 0x88u};

    mt.stage(a);
    mt.stage(b); // Resets cnt

    CHECK(mt.bus().count() == b.size());
    CHECK(mt.bus().at(0) == b[0]);
    CHECK(mt.bus().at(1) == b[1]);
}

TEST_CASE("Empty frame still pulses START and leaves nothing staged") {
    auto mt = make<9>();

    mt.stage(lqr::Frame{}); // START, no data

    CHECK(mt.bus().count() == 0u);
    const Generation expected = mt.commit();
    mt.bus().tick();
    CHECK(mt.generation() == expected);
}

TEST_CASE("Tick without an armed commit does not advance gen") {
    auto mt = make<9>();

    mt.bus().tick(); // nothing armed
    CHECK(mt.generation() == 0u);

    const Generation e = mt.commit();
    mt.bus().tick(); // lands the swap
    mt.bus().tick(); // second tick no-op
    CHECK(e == 1u);
    CHECK(mt.generation() == 1u);
}

TEST_CASE("Staging a full-capacity frame fills every slot") {
    auto mt = make<4>();
    const std::array<Word, 4> frame{0x10u, 0x20u, 0x30u, 0x40u};

    mt.stage(frame);

    CHECK(mt.bus().count() == frame.size());
    CHECK(mt.bus().at(0) == frame[0]);
    CHECK(mt.bus().at(3) == frame[3]);
}
