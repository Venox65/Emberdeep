#include "ed/formulas.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace ed {

int xpToNextLevel(int level) {
  if (level < 1) {
    return 40;
  }
  if (level < 60) {
    return static_cast<int>(std::llround(40.0 * std::pow(static_cast<double>(level), 1.8)));
  }
  const double base60 = 40.0 * std::pow(60.0, 1.8);
  const double delta = static_cast<double>(level - 60);
  return static_cast<int>(std::llround(base60 * std::pow(1.15, delta)));
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

std::string formatCount(std::int64_t count) {
  if (count < 0) {
    if (count == INT64_MIN) return "-9.223b";
    return "-" + formatCount(-count);
  }
  if (count < 1000) {
    return std::to_string(count);
  }

  auto formatWithPrecision = [](double value, int decimals, const char* suffix) -> std::string {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.*f", decimals, value);
    std::string s(buf);
    if (s.find('.') != std::string::npos) {
      while (!s.empty() && s.back() == '0') {
        s.pop_back();
      }
      if (!s.empty() && s.back() == '.') {
        s.pop_back();
      }
    }
    return s + suffix;
  };

  if (count < 1'000'000) {
    if (count >= 999'995) {
      return "1m";
    }
    return formatWithPrecision(static_cast<double>(count) / 1'000.0, 2, "k");
  }
  if (count < 1'000'000'000) {
    if (count >= 999'999'500) {
      return "1b";
    }
    return formatWithPrecision(static_cast<double>(count) / 1'000'000.0, 3, "m");
  }
  if (count < 1'000'000'000'000LL) {
    if (count >= 999'999'500'000LL) {
      return "1t";
    }
    return formatWithPrecision(static_cast<double>(count) / 1'000'000'000.0, 3, "b");
  }
  return formatWithPrecision(static_cast<double>(count) / 1'000'000'000'000.0, 3, "t");
}

} // namespace ed
