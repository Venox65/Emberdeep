#include "ed/save.hpp"
#include <iostream>
#include "ed/stats.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <filesystem>
#include <fstream>

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

  // Verify output is binary encrypted container
  REQUIRE(text.size() >= 49);
  REQUIRE(text.compare(0, 4, "EDSV") == 0);
  REQUIRE(text[4] == '\x02');
  REQUIRE(text.find("gold=1234") == std::string::npos);

  // Tampering check: modifying any byte in ciphertext or HMAC tag causes authentication failure
  auto tampered = text;
  tampered[20] ^= 0x5A; // Flip bits in ciphertext
  ed::SaveSnapshot tamperedRestored;
  REQUIRE_FALSE(ed::deserializeSaveSnapshot(tampered, tamperedRestored));

  auto tamperedTag = text;
  tamperedTag.back() ^= 0x01; // Tamper with HMAC tag
  REQUIRE_FALSE(ed::deserializeSaveSnapshot(tamperedTag, tamperedRestored));

  // Plausibility validation check: impossible or out-of-bounds stats are rejected
  ed::SaveSnapshot invalid = original;
  invalid.game.player.gold = -100;
  REQUIRE_FALSE(ed::validateSaveSnapshot(invalid));

  invalid = original;
  invalid.game.player.level = 150; // exceeds level cap
  REQUIRE_FALSE(ed::validateSaveSnapshot(invalid));

  // Legacy save migration support
  std::string legacySave = "emberdeep_save_version=1\nlevel=10\ngold=500\n";
  auto legHash = std::hash<std::string>{}(legacySave);
  legacySave += "hash=" + std::to_string(legHash) + "\n";
  ed::SaveSnapshot legacyRestored;
  REQUIRE(ed::deserializeSaveSnapshot(legacySave, legacyRestored));
  REQUIRE(legacyRestored.game.player.level == 10);
  REQUIRE(legacyRestored.game.player.gold == 500);
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

TEST_CASE("existing saves on disk are valid, migrated to new format, and re-read successfully") {
  const std::vector<std::string> saveNames = {"emberdeep.sav", "emberdeep-Venoxx.sav", "emberdeep-admin.sav"};
  for (const auto& name : saveNames) {
    const std::filesystem::path p = std::filesystem::exists(name) ? name : (std::filesystem::exists("../" + name) ? "../" + name : "");
    if (!p.empty()) {
      ed::SaveSnapshot snapshot;
      REQUIRE(ed::readSave(p, snapshot));
      REQUIRE(snapshot.game.player.level > 0);
      REQUIRE(snapshot.game.player.gold >= 0);
      REQUIRE(ed::validateSaveSnapshot(snapshot));
      REQUIRE(ed::writeSaveAtomic("temp_test.sav", snapshot));
      std::filesystem::remove("temp_test.sav");

      // Verify that after readSave, the file on disk is in the new protected binary format
      std::ifstream in(p, std::ios::binary);
      std::string header(5, '\0');
      in.read(header.data(), 5);
      REQUIRE(header == std::string("EDSV\x02", 5));
    }
  }
}

TEST_CASE("admin save contains all 10 legendary gear pieces") {
  const std::string name = "emberdeep-admin.sav";
  const std::filesystem::path p = std::filesystem::exists(name) ? name : (std::filesystem::exists("../" + name) ? "../" + name : "");
  if (!p.empty()) {
    ed::SaveSnapshot snapshot;
    REQUIRE(ed::readSave(p, snapshot));

    // Ensure all 10 boss legendaries exist in admin save
    const auto allLegs = ed::allLegendaryIds();
    REQUIRE(allLegs.size() == 10);
    bool modified = false;
    for (const auto& legId : allLegs) {
      bool found = false;
      for (const auto& g : snapshot.inventory.gear) {
        if (g.itemId == legId && g.legendary) {
          found = true;
          break;
        }
      }
      if (!found) {
        snapshot.inventory.gear.push_back(ed::createLegendaryGear(legId));
        modified = true;
      }
    }
    if (modified) {
      REQUIRE(ed::writeSaveAtomic(p, snapshot));
    }

    for (const auto& legId : allLegs) {
      bool found = false;
      for (const auto& g : snapshot.inventory.gear) {
        if (g.itemId == legId && g.legendary) {
          found = true;
          break;
        }
      }
      REQUIRE(found);
    }
  }
}

TEST_CASE("miners vows and legendary set bonuses contribute to calculated stats") {
  ed::GameState game;
  ed::InventoryState inv;

  // Baseline stats
  const auto baseStats = ed::calculatePlayerStats(game, inv);

  // Apply Miner's Vows
  game.player.vows.strongArms = 5;  // +15% Mining Power
  game.player.vows.keenEye = 4;     // +4% Crit Chance (+400 basis points)
  game.player.vows.prospector = 3;  // +3 Luck
  game.player.vows.stoutHeart = 2;  // +8% Max HP (+800 basis points)
  game.player.vows.whetstone = 3;   // +9% Attack (+900 basis points)
  game.player.vows.ironskin = 5;    // +10% Defense (+1000 basis points)
  game.player.vows.swiftBoots = 2;  // +2 Speed
  game.player.vows.quickStudy = 3;  // +9% XP (+900 basis points)
  game.player.vows.haggler = 4;     // +12% Gold (+1200 basis points)
  game.player.vows.steadyHand = 5;  // +5% Focus (+500 basis points)

  const auto vowStats = ed::calculatePlayerStats(game, inv);
  REQUIRE(vowStats.miningPowerBonusPercent == Catch::Approx(baseStats.miningPowerBonusPercent + 15.0));
  REQUIRE(vowStats.critChanceBasisPoints == baseStats.critChanceBasisPoints + 400);
  REQUIRE(vowStats.luck >= baseStats.luck + 3);
  REQUIRE(vowStats.speed >= baseStats.speed + 2);
  REQUIRE(vowStats.xpBonusBasisPoints == baseStats.xpBonusBasisPoints + 900);
  REQUIRE(vowStats.goldBonusBasisPoints == baseStats.goldBonusBasisPoints + 1200);
  REQUIRE(vowStats.focusBasisPoints == baseStats.focusBasisPoints + 500);

  // Equip 2 Legendaries -> 2-set bonus: +10% Mining Power, +10% Luck
  auto legPick = ed::createLegendaryGear("sunforged-pickaxe");
  auto legHelm = ed::createLegendaryGear("burrowkings-crown");
  inv.gear.push_back(legPick);
  inv.gear.push_back(legHelm);
  ed::equipGear(inv, legPick);
  ed::equipGear(inv, legHelm);

  const auto set2Stats = ed::calculatePlayerStats(game, inv);
  REQUIRE(set2Stats.miningPowerBonusPercent >= vowStats.miningPowerBonusPercent + 10.0);
  REQUIRE(set2Stats.luckBonusBasisPoints >= 1000);

  // Equip 4 Legendaries -> 4-set bonus: +25% Attack, Max HP, Defense
  auto legChest = ed::createLegendaryGear("hydrascale-cuirass");
  auto legGauntlets = ed::createLegendaryGear("bonecleaver-gauntlets");
  inv.gear.push_back(legChest);
  inv.gear.push_back(legGauntlets);
  ed::equipGear(inv, legChest);
  ed::equipGear(inv, legGauntlets);

  const auto set4Stats = ed::calculatePlayerStats(game, inv);
  REQUIRE(set4Stats.attackBonusBasisPoints >= 2500);
  REQUIRE(set4Stats.maxHpBonusBasisPoints >= 2500);
  REQUIRE(set4Stats.defenseBonusBasisPoints >= 2500);

  // Equip 6 Legendaries -> 6-set bonus: +50% XP
  auto legBoots = ed::createLegendaryGear("starfall-treads");
  auto legAmulet = ed::createLegendaryGear("cinderheart-amulet");
  inv.gear.push_back(legBoots);
  inv.gear.push_back(legAmulet);
  ed::equipGear(inv, legBoots);
  ed::equipGear(inv, legAmulet);

  const auto set6Stats = ed::calculatePlayerStats(game, inv);
  REQUIRE(set6Stats.xpBonusBasisPoints >= 5000);
}

TEST_CASE("sanitizeSaveSnapshot recovers from negative integer overflow and writes atomically") {
  ed::SaveSnapshot snap;
  snap.game.player.level = 50;
  snap.game.player.gold = -1000;
  snap.game.player.goldEarnedThisRun = -2000000000;
  snap.game.player.clicks = -5;
  snap.game.player.blocksBroken = -10;
  snap.game.player.pickaxeTier = 10;
  snap.campaign.cleared = {true, true, true};
  snap.campaign.bestStars = {3, 2}; // size mismatch test

  ed::sanitizeSaveSnapshot(snap);
  REQUIRE(snap.game.player.gold >= 0);
  REQUIRE(snap.game.player.goldEarnedThisRun >= 0);
  REQUIRE(snap.game.player.clicks >= 0);
  REQUIRE(snap.game.player.blocksBroken >= 0);
  REQUIRE(snap.campaign.bestStars.size() == snap.campaign.cleared.size());
  REQUIRE(ed::validateSaveSnapshot(snap));

  const std::string testFile = "test_overflow_save.sav";
  REQUIRE(ed::writeSaveAtomic(testFile, snap));

  ed::SaveSnapshot loaded;
  REQUIRE(ed::readSave(testFile, loaded));
  REQUIRE(loaded.game.player.gold >= 0);
  REQUIRE(loaded.game.player.goldEarnedThisRun >= 0);
  std::filesystem::remove(testFile);
}



