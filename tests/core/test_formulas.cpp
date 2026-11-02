#include "ed/formulas.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("xp progression follows the designed formula") {
  REQUIRE(ed::xpToNextLevel(1) == 40);
  REQUIRE(ed::xpToNextLevel(2) == 139);
  REQUIRE(ed::xpToNextLevel(10) == 2524);
  REQUIRE(ed::xpToNextLevel(60) == 63494);
  REQUIRE(ed::xpToNextLevel(61) == 73018);
  REQUIRE(ed::xpToNextLevel(70) > 250000);
  REQUIRE(ed::xpToNextLevel(80) > 1000000);
  REQUIRE(ed::xpToNextLevel(100) > 10000000);
}

TEST_CASE("material formulas match the tier ladder") {
  REQUIRE(ed::materialHpForTier(1) == 20);
  REQUIRE(ed::materialHpForTier(2) == 52);
  REQUIRE(ed::materialHpForTier(10) == 108590);

  REQUIRE(ed::materialXpForTier(1) == 5);
  REQUIRE(ed::materialXpForTier(5) == 166);

  REQUIRE(ed::materialSellForTier(1) == 1);
  REQUIRE(ed::materialSellForTier(10) == 3815);
}

TEST_CASE("mining damage applies hardness and crit damage") {
  const double normal = ed::miningDamage(10.0, 0.0, false, 1.5, 20, 5);
  const double crit = ed::miningDamage(10.0, 0.0, true, 1.5, 20, 5);

  REQUIRE(normal > 0.0);
  REQUIRE(crit > normal);
  REQUIRE(crit > 13.0);
  REQUIRE(crit < 14.0);
}

TEST_CASE("progress score follows the server-defined formula") {
  REQUIRE(ed::progressScore(1, 2, 3, 4, 5) == 1'000'000'000ULL + 20'000'000ULL + 3'000'000ULL + 4'000ULL + 2ULL);
}

TEST_CASE("formatCount abbreviates numbers with k, m, b suffixes and fractional precision") {
  REQUIRE(ed::formatCount(0) == "0");
  REQUIRE(ed::formatCount(12) == "12");
  REQUIRE(ed::formatCount(999) == "999");
  REQUIRE(ed::formatCount(1000) == "1k");
  REQUIRE(ed::formatCount(1050) == "1.05k");
  REQUIRE(ed::formatCount(1200) == "1.2k");
  REQUIRE(ed::formatCount(1250) == "1.25k");
  REQUIRE(ed::formatCount(1234) == "1.23k");
  REQUIRE(ed::formatCount(1500) == "1.5k");
  REQUIRE(ed::formatCount(10000) == "10k");
  REQUIRE(ed::formatCount(10500) == "10.5k");
  REQUIRE(ed::formatCount(999990) == "999.99k");
  REQUIRE(ed::formatCount(1000000) == "1m");
  REQUIRE(ed::formatCount(1200000) == "1.2m");
  REQUIRE(ed::formatCount(1230000) == "1.23m");
  REQUIRE(ed::formatCount(1234000) == "1.234m");
  REQUIRE(ed::formatCount(1234567) == "1.235m");
  REQUIRE(ed::formatCount(1000000000) == "1b");
  REQUIRE(ed::formatCount(1234000000) == "1.234b");
  REQUIRE(ed::formatCount(-1500) == "-1.5k");
}
