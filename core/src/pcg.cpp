#include "ed/pcg.hpp"

#include <algorithm>
#include <limits>

namespace ed {

Pcg32::Pcg32(std::uint64_t seed)
    : state_(seed == 0 ? 0x4d595df4d0f33173ULL : seed),
      increment_((state_ << 1U) | 1U) {
  state_ += 0x9e3779b97f4a7c15ULL;
}

std::uint32_t Pcg32::next() {
  const std::uint64_t oldState = state_;
  state_ = oldState * 6364136223846793005ULL + increment_;
  const std::uint32_t xorShifted = static_cast<std::uint32_t>(((oldState >> 18U) ^ oldState) >> 27U);
  const std::uint32_t rot = static_cast<std::uint32_t>(oldState >> 61U);
  return (xorShifted >> rot) | (xorShifted << ((32U - rot) & 31U));
}

std::uint32_t Pcg32::nextRange(std::uint32_t inclusiveMin, std::uint32_t inclusiveMax) {
  if (inclusiveMin > inclusiveMax) {
    std::swap(inclusiveMin, inclusiveMax);
  }
  const std::uint64_t range = static_cast<std::uint64_t>(inclusiveMax - inclusiveMin) + 1ULL;
  const std::uint32_t random = next();
  const std::uint64_t scaled = (static_cast<std::uint64_t>(random) * range) >> 32U;
  return static_cast<std::uint32_t>(inclusiveMin + scaled);
}

} // namespace ed
