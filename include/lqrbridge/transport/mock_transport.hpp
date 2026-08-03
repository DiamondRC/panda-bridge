#pragma once

#include "lqrbridge/transport/transport.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>

namespace lqr {

    template<std::size_t Capacity>
    class MockTransport {
    private:
        std::array<Word, Capacity> staged_{0}; // inactive bank
        std::array<Word, Capacity> active_{0}; // last commited
        std::size_t staged_len_ = 0;
        std::size_t active_len_ = 0;
        Generation generation_ = 0;
        std::size_t stage_count_ = 0;
    public:
        // Write frame into inactive bank
        void stage(Frame f) noexcept {
            // Don't accidently overwrite!
            assert(f.size() <= Capacity);
            // Copy whole frame
            std::copy(f.begin(), f.end(), staged_.begin());
            staged_len_ = f.size();
            ++stage_count_;
        }

        // Publish staged frame
        Generation commit() noexcept {
            // Only want to copy staged
            std::copy(staged_.begin(), staged_.begin() + staged_len_, active_.begin());
            active_len_ = staged_len_;
            return ++generation_;
        }

        Generation generation() const noexcept {
            return generation_;
        }

        // Test inspection
        Frame active() const noexcept { return {active_.data(), active_len_};} // reader view
        Frame staged() const noexcept { return {staged_.data(), staged_len_};}
        std::size_t stage_count() const noexcept { return stage_count_;}
    };
}
