#include "ed/postgame.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("post-campaign scaling follows the specified formulas") {
  REQUIRE(ed::echoDifficultyMultiplier(1) > 0.999);
  REQUIRE(ed::echoDifficultyMultiplier(1) < 1.001);
  REQUIRE(ed::echoDifficultyMultiplier(5) > 1.319);
  REQUIRE(ed::echoDifficultyMultiplier(5) < 1.321);

  const auto block = ed::deepStrataBlock(2);
  REQUIRE(block.depth == 2);
  REQUIRE(block.hp > 108700);
  REQUIRE(block.xp > 13200);
  REQUIRE(block.oreQuantity == 2);
  REQUIRE(block.aetherShardChanceBasisPoints == 500);
}

TEST_CASE("ascension awards marks and resets run-scoped progression") {
  ed::GameState game;
  game.player.level = 42;
  game.player.gold = 5000;
  game.player.goldEarnedThisRun = 200000;
  game.player.echoRank = 2;
  game.player.arcaneDust = 20;
  game.player.pickaxeTier = 7;

  const int marks = ed::applyAscension(game);

  REQUIRE(marks == 6);
  REQUIRE(game.player.ascensions == 1);
  REQUIRE(game.player.emberMarks == 6);
  REQUIRE(game.player.level == 1);
  REQUIRE(game.player.gold == 0);
  REQUIRE(game.player.arcaneDust == 0);
  REQUIRE(game.player.pickaxeTier == 1);
  REQUIRE(game.dirty);
}
