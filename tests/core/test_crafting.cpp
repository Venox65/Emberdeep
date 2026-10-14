#include "ed/crafting.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("smelting converts four ore into one ingot") {
  ed::InventoryState inventory;
  REQUIRE(ed::addItemToInventory(inventory, "ore", "iron", 8));
  REQUIRE(ed::smeltOre(inventory, "iron", 2));
  REQUIRE(ed::countItem(inventory, "ore", "iron") == 0);
  REQUIRE(ed::countItem(inventory, "ingots", "iron") == 2);
}

TEST_CASE("crafting consumes resources and creates tiered gear") {
  ed::GameState game;
  game.player.gold = 1000;
  ed::InventoryState inventory;
  REQUIRE(ed::addItemToInventory(inventory, "ingots", "ashen-stone", 8));
  const auto recipes = ed::defaultCraftRecipes(1);
  REQUIRE(ed::craftGear(game, inventory, recipes.front()));
  REQUIRE(inventory.gear.size() == 1);
  REQUIRE(inventory.gear.front().slot == "pickaxe");
  REQUIRE(game.player.gold == 940);
}

TEST_CASE("gear upgrades consume gold and dust") {
  ed::GameState game;
  game.player.gold = 1000;
  game.player.arcaneDust = 5;
  ed::GearPiece piece{"pickaxe", "iron-pickaxe", "Iron Pickaxe", 3, 0, 0, false};
  REQUIRE(ed::upgradeGear(game, piece));
  REQUIRE(piece.upgradeLevel == 1);
  REQUIRE(game.player.arcaneDust == 4);
  REQUIRE(game.player.gold < 1000);
}

TEST_CASE("mythic gear upgrades require endgame unlock and postgame materials") {
  ed::GameState game;
  game.player.gold = 25'000'000;
  game.player.arcaneDust = 100;
  game.player.bossesCleared = 9; // Not unlocked yet

  ed::GearPiece t10Piece{"pickaxe", "aether-pickaxe", "Aetherpick", 10, 10, 3, false};

  // Cannot upgrade beyond +10 before clearing Hollow King
  REQUIRE_FALSE(ed::isMythicUpgradeUnlocked(game));
  REQUIRE(ed::maxGearUpgradeLevel(game) == 10);
  REQUIRE_FALSE(ed::canUpgradeGear(game, t10Piece));
  REQUIRE_FALSE(ed::upgradeGear(game, t10Piece));

  // Beat Hollow King -> Mythic unlocked
  game.player.bossesCleared = 10;
  REQUIRE(ed::isMythicUpgradeUnlocked(game));
  REQUIRE(ed::maxGearUpgradeLevel(game) == 20);

  // Missing Aether Shards & Echo Sigils -> cannot upgrade
  REQUIRE(ed::upgradeAetherShardCost(t10Piece) == 1);
  REQUIRE(ed::upgradeEchoSigilCost(t10Piece) == 1);
  REQUIRE_FALSE(ed::canUpgradeGear(game, t10Piece));

  // Grant shards and sigils
  game.player.aetherShards = 5;
  game.player.echoSigils = 5;
  REQUIRE(ed::canUpgradeGear(game, t10Piece));

  // Upgrade to +11
  REQUIRE(ed::upgradeGear(game, t10Piece));
  REQUIRE(t10Piece.upgradeLevel == 11);
  REQUIRE(game.player.aetherShards == 4);
  REQUIRE(game.player.echoSigils == 4);
  // Tier 10 gear gains 4th socket at Mythic level
  REQUIRE(t10Piece.sockets == 4);

  // Test cap at +20
  t10Piece.upgradeLevel = 20;
  REQUIRE_FALSE(ed::canUpgradeGear(game, t10Piece));
  REQUIRE(ed::upgradeGoldCost(t10Piece) == 0);
}
