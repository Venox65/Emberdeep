#include "ed/inventory.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("inventory grants and removes counts for item types") {
  ed::InventoryState inventory;

  REQUIRE(ed::addItemToInventory(inventory, "ore", "iron", 3));
  REQUIRE(ed::addItemToInventory(inventory, "gems", "amber", 2));
  REQUIRE(inventory.ore.at("iron") == 3);
  REQUIRE(inventory.gems.at("amber") == 2);

  REQUIRE(ed::removeItemFromInventory(inventory, "ore", "iron", 2));
  REQUIRE(inventory.ore.at("iron") == 1);

  REQUIRE_FALSE(ed::removeItemFromInventory(inventory, "ore", "iron", 99));
}

TEST_CASE("gear equip writes into the correct slot") {
  ed::InventoryState inventory;
  ed::GearPiece piece{"pickaxe", "iron-pick", "Iron Pick", 3, 2, 0, false};

  REQUIRE(ed::equipGear(inventory, piece));
  REQUIRE(inventory.equipped.pickaxeId == "iron-pick");
}

TEST_CASE("socket count follows tier rules") {
  REQUIRE(ed::gearSocketCount(3) == 0);
  REQUIRE(ed::gearSocketCount(4) == 1);
  REQUIRE(ed::gearSocketCount(7) == 2);
  REQUIRE(ed::gearSocketCount(10) == 3);
  REQUIRE(ed::gearSocketCount(10, true) == 4);
}

TEST_CASE("gems can be inserted into and removed from gear sockets") {
  ed::InventoryState inventory;
  ed::GearPiece piece{"amulet", "amber-amulet", "Amber Amulet", 4, 0, 1, false};
  REQUIRE(ed::addItemToInventory(inventory, "gems", "amber", 1));

  REQUIRE(ed::insertGem(inventory, piece, "amber"));
  REQUIRE(piece.socketedGems == std::vector<std::string>{"amber"});
  REQUIRE(ed::countItem(inventory, "gems", "amber") == 0);
  REQUIRE_FALSE(ed::insertGem(inventory, piece, "amber"));

  REQUIRE(ed::removeGem(inventory, piece, 0));
  REQUIRE(piece.socketedGems.empty());
  REQUIRE(ed::countItem(inventory, "gems", "amber") == 1);
}
