// Check C++26 works

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <cstdint>
#include <numeric>  // std::add_sat — C++26 saturating arithmetic (our RTL saturate, now in std)

// C++26 pack indexing — proves `-std=c++2c` is genuinely active, not merely accepted.
template <typename... Ts>
constexpr auto first(Ts... xs) {
    return xs...[0];
}
static_assert(first(7, 8, 9) == 7, "pack indexing should select the first element");

TEST_CASE("C++26 library: saturating arithmetic clamps instead of wrapping") {
    constexpr std::int8_t hi{100};
    constexpr std::int8_t lo{-100};
    CHECK(std::add_sat(hi, hi) == std::int8_t{127});   // clamps at max, no overflow
    CHECK(std::add_sat(lo, lo) == std::int8_t{-128});  // clamps at min
}
