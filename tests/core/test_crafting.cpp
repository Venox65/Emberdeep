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
