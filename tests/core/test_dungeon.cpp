#include "ed/dungeon.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("dungeon simulation is deterministic for the same seed") {
  const auto zones = ed::defaultDungeonZones();
  ed::DungeonPlayerStats player;
  player.maxHp = 100000;
  player.attack = 10000;
  player.defense = 1000;
  player.speed = 100;

  const auto first = ed::simulateDungeonRun(player, zones[0], 1234);
  const auto second = ed::simulateDungeonRun(player, zones[0], 1234);

  REQUIRE(first.cleared == second.cleared);
  REQUIRE(first.floorsCleared == second.floorsCleared);
  REQUIRE(first.gold == second.gold);
  REQUIRE(first.xp == second.xp);
  REQUIRE(first.trophies == second.trophies);
  REQUIRE(first.gemFound == second.gemFound);
}

TEST_CASE("dungeon clear grants all five floors and a chest multiplier") {
  const auto zones = ed::defaultDungeonZones();
  ed::DungeonPlayerStats player;
  player.maxHp = 100000;
  player.attack = 10000;
  player.defense = 1000;
  player.speed = 100;

  const auto result = ed::simulateDungeonRun(player, zones[0], 99);

  REQUIRE(result.cleared);
  REQUIRE(result.floorsCleared == 5);
  REQUIRE(result.gold > 0);
  REQUIRE(result.xp > 0);
  REQUIRE(result.trophies >= 5);
}
