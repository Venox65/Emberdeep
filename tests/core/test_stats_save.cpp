#include "ed/save.hpp"
#include "ed/stats.hpp"
#include "ed/cloud.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>

TEST_CASE("equipped pickaxe increases mining power and equipped armor affects other stats") {
  ed::GameState game;
  ed::InventoryState inventory;
  const auto base = ed::calculatePlayerStats(game, inventory);

  inventory.gear.push_back({"pickaxe", "iron-pickaxe", "Iron Pickaxe", 3, 0, 0, false});
  inventory.gear.push_back({"chestplate", "iron-chestplate", "Iron Chestplate", 3, 0, 0, false});
  inventory.gear.push_back({"boots", "iron-boots", "Iron Boots", 3, 0, 0, false});
  ed::equipGear(inventory, inventory.gear[0]);
  ed::equipGear(inventory, inventory.gear[1]);
  ed::equipGear(inventory, inventory.gear[2]);

  const auto equipped = ed::calculatePlayerStats(game, inventory);
  REQUIRE(equipped.miningPower > base.miningPower);
  REQUIRE(equipped.maxHp > base.maxHp);
  REQUIRE(equipped.defense > base.defense);
  REQUIRE(equipped.speed > base.speed);
}

TEST_CASE("socketed gems contribute their defined equipment bonuses") {
  ed::GameState game;
  ed::InventoryState inventory;
  inventory.gear.push_back({"amulet", "gem-amulet", "Gem Amulet", 4, 0, 1, false,
                            {"dragon-eye"}});
  ed::equipGear(inventory, inventory.gear.front());

  const auto stats = ed::calculatePlayerStats(game, inventory);
  REQUIRE(stats.pierce >= 2);
  REQUIRE(stats.critDamageBasisPoints >= 15500);
}

TEST_CASE("percentage gem bonuses are applied after equipment totals") {
  ed::GameState game;
  ed::InventoryState inventory;
  inventory.gear.push_back({"gauntlets", "garnet-gauntlets", "Garnet Gauntlets", 4, 0, 1,
                            false, {"garnet"}});
  ed::equipGear(inventory, inventory.gear.front());

  const auto stats = ed::calculatePlayerStats(game, inventory);
  REQUIRE(stats.attackBonusBasisPoints == 200);
  REQUIRE(stats.attack >= 12);
}

TEST_CASE("save snapshots round-trip game, inventory, gear, and campaign") {
  ed::SaveSnapshot original;
  original.game.player.level = 8;
  original.game.player.gold = 1234;
  original.game.player.pickaxeTier = 3;
  ed::addItemToInventory(original.inventory, "ore", "iron", 12);
  original.inventory.gear.push_back({"pickaxe", "iron-pickaxe", "Iron Pickaxe", 3, 2, 1,
                                     false, {"amber"}});
  ed::addItemToInventory(original.inventory, "gems", "amber", 1);
  ed::equipGear(original.inventory, original.inventory.gear.front());
  original.campaign.cleared = {true, false};
  original.campaign.bestStars = {3, 0};

  const auto text = ed::serializeSaveSnapshot(original);
  ed::SaveSnapshot restored;
  REQUIRE(ed::deserializeSaveSnapshot(text, restored));
  REQUIRE(restored.game.player.level == 8);
  REQUIRE(restored.game.player.gold == 1234);
  REQUIRE(ed::countItem(restored.inventory, "ore", "iron") == 12);
  REQUIRE(restored.inventory.gear.size() == 1);
  REQUIRE(restored.inventory.gear.front().socketedGems ==
          std::vector<std::string>{"amber"});
  REQUIRE(restored.inventory.equipped.pickaxeId == "iron-pickaxe");
  REQUIRE(restored.campaign.cleared.size() == 2);
  REQUIRE(restored.campaign.cleared[0]);
  REQUIRE(restored.campaign.bestStars[0] == 3);
}

TEST_CASE("save backend writes and reads atomically") {
  const auto path = std::filesystem::temp_directory_path() / "emberdeep-save-test.sav";
  ed::SaveSnapshot original;
  original.game.player.gold = 77;
  REQUIRE(ed::writeSaveAtomic(path, original));

  ed::SaveSnapshot restored;
  REQUIRE(ed::readSave(path, restored));
  REQUIRE(restored.game.player.gold == 77);
  std::filesystem::remove(path);
}

TEST_CASE("relative account profiles can be written") {
  const auto path = std::filesystem::path("emberdeep-profile-test.tmp");
  ed::AccountProfile original{"account", "player", "token"};
  REQUIRE(ed::writeAccountProfile(path, original));

  ed::AccountProfile restored;
  REQUIRE(ed::readAccountProfile(path, restored));
  REQUIRE(restored.mode == original.mode);
  REQUIRE(restored.username == original.username);
  REQUIRE(restored.token == original.token);
  std::filesystem::remove(path);
}
