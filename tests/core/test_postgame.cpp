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

  ed::GameState testGame;
  REQUIRE_FALSE(ed::isDeepStrataUnlocked(testGame));
  testGame.player.bossesCleared = 10;
  REQUIRE(ed::isDeepStrataUnlocked(testGame));
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

TEST_CASE("level cap scales with potential perk up to 100") {
  ed::PlayerState player;
  REQUIRE(ed::playerLevelCap(player) == 60);

  player.emberPerks.potential = 3;
  REQUIRE(ed::playerLevelCap(player) == 65);

  player.emberPerks.potential = 24;
  REQUIRE(ed::playerLevelCap(player) == 100);

  player.emberPerks.potential = 30; // capped at 100
  REQUIRE(ed::playerLevelCap(player) == 100);
}

TEST_CASE("ember perks can be upgraded with marks and cap at maximum ranks") {
  ed::GameState game;
  game.player.emberMarks = 20;

  REQUIRE(ed::upgradeEmberPerk(game, "power"));
  REQUIRE(game.player.emberPerks.power == 1);
  REQUIRE(game.player.emberMarks == 19); // cost 1

  REQUIRE(ed::upgradeEmberPerk(game, "power"));
  REQUIRE(game.player.emberPerks.power == 2);
  REQUIRE(game.player.emberMarks == 17); // cost 2

  // Armory costs 5
  REQUIRE(ed::upgradeEmberPerk(game, "armory"));
  REQUIRE(game.player.emberPerks.armory == 1);
  REQUIRE(game.player.emberMarks == 12);

  // Armory max rank is 1
  REQUIRE_FALSE(ed::upgradeEmberPerk(game, "armory"));

  // Check stat bonuses
  REQUIRE(ed::perkMiningPowerBonusBasisPoints(game.player.emberPerks) == 1000); // 2 ranks * 500
}

TEST_CASE("ascension with inventory clears raw materials, keeps legendaries and gems, and grants armory gear") {
  ed::GameState game;
  ed::InventoryState inventory;

  game.player.goldEarnedThisRun = 100000;
  game.player.emberPerks.armory = 1;

  ed::addItemToInventory(inventory, "ore", "copper", 20);
  ed::addItemToInventory(inventory, "ingots", "iron", 10);
  ed::addItemToInventory(inventory, "trophies", "gnawed-pelt", 5);
  ed::addItemToInventory(inventory, "gems", "amber", 3);

  // Non-legendary gear
  inventory.gear.push_back({"pickaxe", "stone-pick", "Stone Pick", 1, 0, 0, false});
  // Legendary gear
  inventory.gear.push_back({"amulet", "aetherbrand", "The Aetherbrand", 10, 5, 4, true});
  ed::equipGear(inventory, inventory.gear[0]);
  ed::equipGear(inventory, inventory.gear[1]);

  ed::applyAscension(game, &inventory);

  // Ores, ingots, trophies are wiped
  REQUIRE(inventory.ore.empty());
  REQUIRE(inventory.ingots.empty());
  REQUIRE(inventory.trophies.empty());

  // Gems preserved
  REQUIRE(ed::countItem(inventory, "gems", "amber") == 3);

  // Non-legendary stone pick wiped, legendary preserved, iron set granted
  bool hasLegendary = false;
  bool hasIronPick = false;
  for (const auto& piece : inventory.gear) {
    if (piece.itemId == "aetherbrand") hasLegendary = true;
    if (piece.itemId == "iron-pickaxe") hasIronPick = true;
    REQUIRE_FALSE(piece.itemId == "stone-pick");
  }
  REQUIRE(hasLegendary);
  REQUIRE(hasIronPick);
  REQUIRE(game.player.pickaxeTier == 3);
  REQUIRE(inventory.equipped.amuletId == "aetherbrand");
  REQUIRE(inventory.equipped.pickaxeId == "iron-pickaxe");
}

TEST_CASE("echo campaign tracks clears, scales rewards, and advances rank after all 10 cleared") {
  ed::GameState game;
  game.player.bossesCleared = 9;
  REQUIRE_FALSE(ed::isEchoCampaignUnlocked(game));

  game.player.bossesCleared = 10;
  REQUIRE(ed::isEchoCampaignUnlocked(game));

  REQUIRE(ed::echoBossRewardGold(1000, 1) == 1000);
  REQUIRE(ed::echoBossRewardGold(1000, 2) == 1250); // 1 + 0.25 * (2-1)
  REQUIRE(ed::echoBossRewardSigils(2) == 2);

  for (int i = 0; i < 9; ++i) {
    ed::recordEchoBossClear(game, i);
    REQUIRE(ed::isEchoBossCleared(game, i));
  }
  REQUIRE(game.player.echoRank == 0);

  // Clearing the 10th boss advances the Echo Rank and resets clear flags for the next rank
  ed::recordEchoBossClear(game, 9);
  REQUIRE(game.player.echoRank == 1);
  REQUIRE_FALSE(ed::isEchoBossCleared(game, 0));
}
