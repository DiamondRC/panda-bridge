#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "lqrbridge/transport/mock_transport.hpp"
#include "lqrbridge/transport/transport.hpp"
  
#include <doctest/doctest.h>
#include <array>

using lqr::MockTransport, lqr::Transport, lqr::Word;


// Force full instantiation
static_assert(Transport<MockTransport<9>>);

TEST_CASE("Mock Transport has no visibilty until commit") {
    MockTransport<9> mt;
    std::array<Word, 3> k1{1, 2, 3};

    mt.stage(k1);
    // Staged but not visible
    CHECK(mt.generation() == 0);
    CHECK(mt.active().empty());

    const auto gen = mt.commit();
    CHECK(gen == 1);
    CHECK(mt.generation() == 1);
    REQUIRE(mt.active().size() == 3);
    CHECK(mt.active()[2] == 3);

    // Stage second frame
    std::array<Word, 3> k2{7, 8, 9};
    mt.stage(k2);
    CHECK(mt.active()[0] == 1); // k1, untorn
    mt.commit();
    CHECK(mt.active()[0] == 7); // k2
}