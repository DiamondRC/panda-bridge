#pragma once

#include <cstdint>
#include <span>

namespace lqr {

// Device word for PandA BRAM/reg compat
using Word = std::uint32_t;
// Gain frame - run of words in row * N + col form.
using Frame = std::span<const Word>;

}