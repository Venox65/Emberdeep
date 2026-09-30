#include "ed/game_state.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("level progression gives ranks and vow points") {
  ed::GameState state;
  state.player.level = 1;
  state.player.xp = 0;

  ed::applyXp(state, 1000);

  REQUIRE(state.player.level >= 2);
  REQUIRE(state.player.vowPoints >= 1);
  REQUIRE(state.player.xp < ed::xpToNextLevel(state.player.level));
}

TEST_CASE("layer unlock checks min level and pickaxe tier") {
  ed::GameState state;
  state.player.level = 5;
  state.player.pickaxeTier = 1;

  const ed::LayerDef copper = {"copper-crypt", "Copper Crypt", 2, 5, 1};
  const ed::LayerDef iron = {"iron-hollows", "Iron Hollows", 3, 10, 2};

  REQUIRE(ed::canUnlockLayer(state, copper));
  REQUIRE_FALSE(ed::canUnlockLayer(state, iron));
}

TEST_CASE("save state round-trips with plain-text serialization") {
  ed::GameState original;
  original.schemaVersion = 2;
  original.saveId = "slot-1";
  original.dirty = true;
  original.player.level = 12;
  original.player.xp = 420;
  original.player.gold = 2500;
  original.player.arcaneDust = 99;
  original.player.vowPoints = 5;
  original.player.blocksBroken = 550;
  original.player.clicks = 1024;
  original.player.bossesCleared = 2;
  original.player.ascensions = 1;
  original.player.echoRank = 3;
  original.player.pickaxeTier = 4;
  original.player.currentLayer = "silvermire";

  const std::string text = ed::serializeGameState(original);
  ed::GameState restored;
  REQUIRE(ed::deserializeGameState(text, restored));

  REQUIRE(restored.schemaVersion == original.schemaVersion);
  REQUIRE(restored.saveId == original.saveId);
  REQUIRE(restored.dirty == original.dirty);
  REQUIRE(restored.player.level == original.player.level);
  REQUIRE(restored.player.xp == original.player.xp);
  REQUIRE(restored.player.gold == original.player.gold);
  REQUIRE(restored.player.arcaneDust == original.player.arcaneDust);
  REQUIRE(restored.player.vowPoints == original.player.vowPoints);
  REQUIRE(restored.player.bossesCleared == original.player.bossesCleared);
  REQUIRE(restored.player.pickaxeTier == original.player.pickaxeTier);
  REQUIRE(restored.player.currentLayer == original.player.currentLayer);
}
