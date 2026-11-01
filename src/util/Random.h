#pragma once

#include <cstdint>
#include <random>

namespace util {

// One engine per thread so the clock and CLI threads never share RNG state.
inline std::mt19937_64& rng() {
    thread_local std::mt19937_64 engine{std::random_device{}()};
    return engine;
}

// Uniform integer in [lo, hi].
inline uint64_t randRange(uint64_t lo, uint64_t hi) {
    return std::uniform_int_distribution<uint64_t>(lo, hi)(rng());
}

}  // namespace util
