#include "ed/save.hpp"
#include "ed/stats.hpp"

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

  original.game.player.abyssBestFloor = 15;
  original.game.player.emberMarks = 40;
  original.game.player.emberPerks.wealth = 2;
  original.game.player.echoRank = 1;
  original.game.player.deepestDepth = 7;
  original.game.player.aetherShards = 12;
  original.game.player.echoCleared = {true, false, true, false, false, false, false, false, false, false};

  const auto text = ed::serializeSaveSnapshot(original);
  ed::SaveSnapshot restored;
  REQUIRE(ed::deserializeSaveSnapshot(text, restored));
  REQUIRE(restored.game.player.level == 8);
  REQUIRE(restored.game.player.gold == 1234);
  REQUIRE(restored.game.player.abyssBestFloor == 15);
  REQUIRE(restored.game.player.emberMarks == 40);
  REQUIRE(restored.game.player.emberPerks.wealth == 2);
  REQUIRE(restored.game.player.echoRank == 1);
  REQUIRE(restored.game.player.deepestDepth == 7);
  REQUIRE(restored.game.player.aetherShards == 12);
  REQUIRE(restored.game.player.echoCleared.size() == 10);
  REQUIRE(restored.game.player.echoCleared[0]);
  REQUIRE_FALSE(restored.game.player.echoCleared[1]);
  REQUIRE(ed::countItem(restored.inventory, "ore", "iron") == 12);
  REQUIRE(restored.inventory.gear.size() == 1);
  REQUIRE(restored.inventory.gear.front().socketedGems ==
          std::vector<std::string>{"amber"});
  REQUIRE(restored.inventory.equipped.pickaxeId == "iron-pickaxe");
  REQUIRE(restored.campaign.cleared.size() == 2);
  REQUIRE(restored.campaign.cleared[0]);
  REQUIRE(restored.campaign.bestStars[0] == 3);

  // Tampering check: modifying any value without updating hash fails deserialization
  auto tampered = text;
  auto goldPos = tampered.find("gold=1234");
  REQUIRE(goldPos != std::string::npos);
  tampered.replace(goldPos, 9, "gold=9999");
  ed::SaveSnapshot tamperedRestored;
  REQUIRE_FALSE(ed::deserializeSaveSnapshot(tampered, tamperedRestored));
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

TEST_CASE("emberdeep.sav on disk is valid and loads successfully") {
  if (std::filesystem::exists("emberdeep.sav")) {
    ed::SaveSnapshot snapshot;
    REQUIRE(ed::readSave("emberdeep.sav", snapshot));
    REQUIRE(snapshot.game.player.level == 76);
    REQUIRE(snapshot.game.player.aetherShards == 1000000);
  }
}


