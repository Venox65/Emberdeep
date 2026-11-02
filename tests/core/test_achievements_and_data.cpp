#include <catch2/catch_test_macros.hpp>

#include "ed/achievements.hpp"
#include "ed/data_loader.hpp"
#include "ed/game_state.hpp"
#include "ed/inventory.hpp"
#include "ed/campaign.hpp"

TEST_CASE("Dynamic data loader loads json files and handles fallbacks", "[data_loader]") {
  SECTION("Loads valid game data from data directory") {
    auto data = ed::loadGameData("data");
    REQUIRE(data.loadedFromDisk);
    REQUIRE(data.materials.size() == 10);
    REQUIRE(data.layers.size() == 10);
    REQUIRE(data.bosses.size() == 10);
    REQUIRE(data.zones.size() == 10);
    REQUIRE(data.gems.size() == 10);

    CHECK(data.materials[0].id == "ashen-stone");
    CHECK(data.layers[0].id == "ashen-warrens");
    CHECK(data.bosses[0].id == "grubnok");
    CHECK(data.bosses[9].id == "hollow-king");
  }

  SECTION("Fallback to compiled defaults on nonexistent directory") {
    auto fallback = ed::loadGameData("non_existent_directory_12345");
    CHECK_FALSE(fallback.loadedFromDisk);
    REQUIRE(fallback.materials.size() == 10);
    REQUIRE(fallback.layers.size() == 10);
    REQUIRE(fallback.bosses.size() == 10);
    REQUIRE(fallback.zones.size() == 10);
    REQUIRE(fallback.gems.size() == 10);
  }
}

TEST_CASE("Achievements definition and evaluation logic", "[achievements]") {
  const auto& all = ed::allAchievements();
  REQUIRE(all.size() >= 30);

  for (const auto& a : all) {
    CHECK_FALSE(a.id.empty());
    CHECK_FALSE(a.title.empty());
    CHECK_FALSE(a.description.empty());
    CHECK_FALSE(a.category.empty());
  }

  ed::GameState game;
  ed::InventoryState inv;
  auto campaign = ed::createCampaignState(10);

  SECTION("Fresh state has no unlocked achievements") {
    auto initial = ed::evaluateNewAchievements(game, inv, campaign);
    CHECK(initial.empty());
  }

  SECTION("Mining and weak spot achievements trigger accurately") {
    game.player.blocksBroken = 1;
    game.player.weakSpotsHit = 1;

    auto newly = ed::evaluateNewAchievements(game, inv, campaign);
    REQUIRE(newly.size() == 2);
    CHECK(newly[0] == "first_strike");
    CHECK(newly[1] == "precision_striker");

    // Add them to player achievements
    for (const auto& id : newly) {
      game.player.achievements.push_back(id);
    }
    CHECK(ed::isAchievementUnlocked(game.player, "first_strike"));
    CHECK(ed::isAchievementUnlocked(game.player, "precision_striker"));

    // Next evaluation should not return them again
    auto nextEval = ed::evaluateNewAchievements(game, inv, campaign);
    CHECK(nextEval.empty());
  }

  SECTION("Progression and Combat achievements trigger on criteria") {
    game.player.level = 35;
    game.player.gold = 150000;
    game.player.deepestDepth = 12;
    campaign.cleared[0] = true;
    campaign.bestStars[0] = 3;

    auto newly = ed::evaluateNewAchievements(game, inv, campaign);
    CHECK(std::find(newly.begin(), newly.end(), "novice_adventurer") != newly.end());
    CHECK(std::find(newly.begin(), newly.end(), "seasoned_delver") != newly.end());
    CHECK(std::find(newly.begin(), newly.end(), "pocket_change") != newly.end());
    CHECK(std::find(newly.begin(), newly.end(), "gold_hoarder") != newly.end());
    CHECK(std::find(newly.begin(), newly.end(), "depth_explorer") != newly.end());
    CHECK(std::find(newly.begin(), newly.end(), "abyss_diver") != newly.end());
    CHECK(std::find(newly.begin(), newly.end(), "first_boss_slain") != newly.end());
    CHECK(std::find(newly.begin(), newly.end(), "perfect_strike") != newly.end());
  }

  SECTION("Crafting achievements trigger on gear & socket counts") {
    ed::GearPiece piece1;
    piece1.itemId = "pick_1";
    piece1.sockets = 2;
    piece1.socketedGems = {"ruby", "sapphire"};

    ed::GearPiece piece2;
    piece2.itemId = "helm_1";
    piece2.sockets = 1;
    piece2.socketedGems = {"emerald"};

    ed::GearPiece piece3;
    piece3.itemId = "chest_1";
    piece3.tier = 5;

    inv.gear.push_back(piece1);
    inv.gear.push_back(piece2);
    inv.gear.push_back(piece3);

    auto newly = ed::evaluateNewAchievements(game, inv, campaign);
    CHECK(std::find(newly.begin(), newly.end(), "blacksmith_hand") != newly.end());
    CHECK(std::find(newly.begin(), newly.end(), "gemologist") != newly.end());
    CHECK(std::find(newly.begin(), newly.end(), "jeweled_knight") != newly.end());
    CHECK(std::find(newly.begin(), newly.end(), "mythic_ascendant") != newly.end());
  }

  SECTION("Hollow Descent achievements trigger on run, chamber, victory, and vault milestones") {
    game.player.descentRuns = 1;
    game.player.descentBestChamber = 10;
    game.player.descentVictories = 1;
    game.player.descentVaultsCracked = 1;
    game.player.descentMaxRunLevel = 15;

    auto newly = ed::evaluateNewAchievements(game, inv, campaign);
    CHECK(std::find(newly.begin(), newly.end(), "descent_initiate") != newly.end());
    CHECK(std::find(newly.begin(), newly.end(), "descent_depth_5") != newly.end());
    CHECK(std::find(newly.begin(), newly.end(), "descent_depth_10") != newly.end());
    CHECK(std::find(newly.begin(), newly.end(), "descent_conqueror") != newly.end());
    CHECK(std::find(newly.begin(), newly.end(), "descent_vault_cracker") != newly.end());
    CHECK(std::find(newly.begin(), newly.end(), "descent_level_15") != newly.end());
  }
}
