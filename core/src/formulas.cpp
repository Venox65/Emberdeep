#include "ed/formulas.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ed {

int xpToNextLevel(int level) {
  if (level < 1) {
    return 40;
  }
  return static_cast<int>(std::llround(40.0 * std::pow(static_cast<double>(level), 1.8)));
}

int materialHpForTier(int tier) {
  if (tier < 1) {
    throw std::invalid_argument("tier must be >= 1");
  }
  return static_cast<int>(std::llround(20.0 * std::pow(2.6, tier - 1)));
}

int materialXpForTier(int tier) {
  if (tier < 1) {
    throw std::invalid_argument("tier must be >= 1");
  }
  return static_cast<int>(std::llround(5.0 * std::pow(2.4, tier - 1)));
}

int materialSellForTier(int tier) {
  if (tier < 1) {
    throw std::invalid_argument("tier must be >= 1");
  }
  return static_cast<int>(std::llround(1.0 * std::pow(2.5, tier - 1)));
}

int materialHardnessForTier(int tier) {
  if (tier < 1) {
    throw std::invalid_argument("tier must be >= 1");
  }
  return std::max(0, static_cast<int>(std::llround(2.5 * (tier - 1) + 0.5 * std::pow(tier, 2))));
}

double miningDamage(double miningPower,
                    double powerBonusPercent,
                    bool isCrit,
                    double critDamage,
                    int hardness,
                    int pierce) {
  const double bonusMultiplier = 1.0 + (powerBonusPercent / 100.0);
  const double rawDamage = miningPower * bonusMultiplier * (isCrit ? critDamage : 1.0);
  const double effectiveHardness = std::max(0.0, static_cast<double>(hardness - pierce));
  const double baseDamage = rawDamage * 100.0 / (100.0 + effectiveHardness);
  return std::max(1.0, baseDamage);
}

std::uint64_t progressScore(int ascensions,
                           int echoRank,
                           int bossesCleared,
                           int level,
                           std::uint64_t lifetimeGold) {
  const double goldLog = std::log2(static_cast<double>(lifetimeGold + 1ULL));
  return static_cast<std::uint64_t>(
      static_cast<std::uint64_t>(ascensions) * 1'000'000'000ULL +
      static_cast<std::uint64_t>(echoRank) * 10'000'000ULL +
      static_cast<std::uint64_t>(bossesCleared) * 1'000'000ULL +
      static_cast<std::uint64_t>(level) * 1'000ULL +
      static_cast<std::uint64_t>(std::floor(goldLog)));
}

} // namespace ed
