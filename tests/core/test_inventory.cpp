#include "ed/inventory.hpp"
#include "ed/game_state.hpp"

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

TEST_CASE("gem cuts and combining behave according to section 6.1") {
  REQUIRE(ed::gemCutFromId("rough-amber") == ed::GemCut::Rough);
  REQUIRE(ed::gemCutFromId("cut-amber") == ed::GemCut::Cut);
  REQUIRE(ed::gemCutFromId("flawless-amber") == ed::GemCut::Flawless);

  REQUIRE(ed::gemCutMultiplier("rough-amber") == 1);
  REQUIRE(ed::gemCutMultiplier("cut-amber") == 2);
  REQUIRE(ed::gemCutMultiplier("flawless-amber") == 4);

  REQUIRE(ed::nextGemCutId("rough-amber") == "cut-amber");
  REQUIRE(ed::nextGemCutId("cut-amber") == "flawless-amber");
  REQUIRE(ed::nextGemCutId("flawless-amber").empty());

  REQUIRE(ed::gemCombineDustCost(ed::GemCut::Rough) == 10);
  REQUIRE(ed::gemCombineGoldCost(ed::GemCut::Rough) == 50);
  REQUIRE(ed::gemCombineDustCost(ed::GemCut::Cut) == 30);
  REQUIRE(ed::gemCombineGoldCost(ed::GemCut::Cut) == 200);

  ed::InventoryState inv;
  ed::GameState game;
  game.player.gold = 300;
  game.player.arcaneDust = 50;

  ed::addItemToInventory(inv, "gems", "rough-amber", 3);
  REQUIRE(ed::canCombineGems(inv, game, "rough-amber"));
  REQUIRE(ed::combineGems(inv, game, "rough-amber"));

  REQUIRE(ed::countItem(inv, "gems", "rough-amber") == 0);
  REQUIRE(ed::countItem(inv, "gems", "cut-amber") == 1);
  REQUIRE(game.player.gold == 250);
  REQUIRE(game.player.arcaneDust == 40);

  // Flawless cannot be upgraded further
  REQUIRE_FALSE(ed::canCombineGems(inv, game, "flawless-amber"));

  // Combining unprefixed gems (e.g. "amber" from drop) works identically
  ed::addItemToInventory(inv, "gems", "amber", 3);
  REQUIRE(ed::countRoughGems(inv, "amber") == 3);
  REQUIRE(ed::canCombineGems(inv, game, "amber"));
  REQUIRE(ed::combineGems(inv, game, "amber"));
  REQUIRE(ed::countItem(inv, "gems", "cut-amber") == 2);
}

TEST_CASE("boss legendary items exist and possess specified attributes") {
  const auto allIds = ed::allLegendaryIds();
  REQUIRE(allIds.size() == 10);

  for (const auto& id : allIds) {
    REQUIRE(ed::isLegendaryId(id));
    auto piece = ed::createLegendaryGear(id);
    REQUIRE(piece.legendary == true);
    REQUIRE_FALSE(piece.slot.empty());
    REQUIRE(piece.tier >= 1);
    REQUIRE(piece.tier <= 10);
    // Legendaries always have +1 extra socket
    REQUIRE(piece.sockets == ed::gearSocketCount(piece.tier, true));
    REQUIRE_FALSE(ed::legendaryEffectDescription(id).empty());
  }

  auto crown = ed::createLegendaryGear("burrowkings-crown");
  REQUIRE(crown.slot == "helm");
  REQUIRE(crown.tier == 1);
  REQUIRE(crown.sockets == 1); // 0 + 1

  auto pick = ed::createLegendaryGear("aetherbrand");
  REQUIRE(pick.slot == "pickaxe");
  REQUIRE(pick.tier == 10);
  REQUIRE(pick.sockets == 4); // 3 + 1
}
