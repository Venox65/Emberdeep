#pragma once

#include <cstdint>

namespace ed {

class Pcg32 {
public:
  explicit Pcg32(std::uint64_t seed = 0x853c49e6748fea9bULL);

  std::uint32_t next();
  std::uint32_t nextRange(std::uint32_t inclusiveMin, std::uint32_t inclusiveMax);

private:
  std::uint64_t state_;
  std::uint64_t increment_;
};

} // namespace ed
