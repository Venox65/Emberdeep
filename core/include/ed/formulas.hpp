#pragma once

#include <cmath>
#include <cstdint>
#include <string>

namespace ed {

constexpr int kMaxTiers = 10;
constexpr double kCritChanceBase = 0.05;
constexpr double kCritDamageBase = 1.5;

int xpToNextLevel(int level);
int materialHpForTier(int tier);
int materialXpForTier(int tier);
int materialSellForTier(int tier);
int materialHardnessForTier(int tier);

inline double clamp01(double value) {
  if (value < 0.0) {
    return 0.0;
  }
  if (value > 1.0) {
    return 1.0;
  }
  return value;
}

inline std::int64_t roundToInt64(double value) {
  return static_cast<std::int64_t>(std::llround(value));
}

double miningDamage(double miningPower,
                    double powerBonusPercent,
                    bool isCrit,
                    double critDamage,
                    int hardness,
                    int pierce);

std::uint64_t progressScore(int ascensions,
                           int echoRank,
                           int bossesCleared,
                           int level,
                           std::uint64_t lifetimeGold);

} // namespace ed
