#include "ed/formulas.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("xp progression follows the designed formula") {
  REQUIRE(ed::xpToNextLevel(1) == 40);
  REQUIRE(ed::xpToNextLevel(2) == 139);
  REQUIRE(ed::xpToNextLevel(10) == 2524);
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
