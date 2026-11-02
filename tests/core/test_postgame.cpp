#include "ed/postgame.hpp"
#include "ed/dungeon.hpp"
#include "ed/stats.hpp"
#include "ed/formulas.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("post-campaign scaling follows the specified formulas") {
  REQUIRE(ed::echoDifficultyMultiplier(1) > 0.999);
  REQUIRE(ed::echoDifficultyMultiplier(1) < 1.001);
  REQUIRE(ed::echoDifficultyMultiplier(5) > 1.319);
  REQUIRE(ed::echoDifficultyMultiplier(5) < 1.321);

  const auto block = ed::deepStrataBlock(2);
  REQUIRE(block.depth == 2);
  REQUIRE(block.hp > 108700);
  REQUIRE(block.xp == 13200);
  REQUIRE(ed::deepStrataBlock(0).xp == 13200);
  REQUIRE(ed::deepStrataBlock(5).xp == 13200);
  REQUIRE(ed::deepStrataBlock(10).xp == 13200);
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

TEST_CASE("the abyss simulation scales floors, rewards milestone loot, and tracks progression") {
  ed::DungeonPlayerStats endgamePlayer;
  endgamePlayer.maxHp = 600000;
  endgamePlayer.attack = 75000;
  endgamePlayer.defense = 40000;
  endgamePlayer.speed = 45;
  endgamePlayer.critChanceBasisPoints = 2500;
  endgamePlayer.critDamageBasisPoints = 18000;

  // Run a 15-floor Abyss gauntlet
  const auto result = ed::simulateAbyssRun(endgamePlayer, 0x12345ULL, 15);
  REQUIRE(result.isAbyss);
  REQUIRE(result.floorsCleared >= 10); // Successfully beat at least the 10th floor mini-boss
  REQUIRE(result.gold > 0);
  REQUIRE(result.xp > 0);
  // Defeating Floor 10 mini-boss guarantees Echo Sigils and Aether Shards
  REQUIRE(result.echoSigils >= 1);
  REQUIRE(result.aetherShards >= 2);
  REQUIRE(result.arcaneDust >= 5);
  REQUIRE_FALSE(result.log.empty());

  // A novice player with low stats cannot survive deep floors
  ed::DungeonPlayerStats weakPlayer;
  weakPlayer.maxHp = 200;
  weakPlayer.attack = 20;
  weakPlayer.defense = 5;
  weakPlayer.speed = 10;

  const auto weakResult = ed::simulateAbyssRun(weakPlayer, 0x12345ULL, 15);
  REQUIRE(weakResult.isAbyss);
  REQUIRE_FALSE(weakResult.cleared);
  REQUIRE(weakResult.floorsCleared == 0);
}

TEST_CASE("chrono cores and auto clicker scaling and upgrades") {
  // First ascension awards 1 Chrono Core regardless of ember marks
  REQUIRE(ed::calculateAscensionChronoCores(0, 0) == 1);
  REQUIRE(ed::calculateAscensionChronoCores(0, 50) == 1);

  // Subsequent ascensions award 1 + floor(earnedMarks / 4)
  REQUIRE(ed::calculateAscensionChronoCores(1, 0) == 1);
  REQUIRE(ed::calculateAscensionChronoCores(1, 4) == 2);
  REQUIRE(ed::calculateAscensionChronoCores(1, 8) == 3);
  REQUIRE(ed::calculateAscensionChronoCores(2, 15) == 4);

  // Auto clicker speed & cost progression
  REQUIRE(ed::autoClickerSpeed(0) == 0.0);
  REQUIRE(ed::autoClickerSpeed(1) == 1.0);
  REQUIRE(ed::autoClickerSpeed(5) == 5.0);
  REQUIRE(ed::autoClickerSpeed(10) == 10.0);

  REQUIRE(ed::autoClickerUpgradeCost(0) == 1);
  REQUIRE(ed::autoClickerUpgradeCost(1) == 2);
  REQUIRE(ed::autoClickerUpgradeCost(9) == 10);
  REQUIRE(ed::autoClickerUpgradeCost(10) == 0); // max level

  // Upgrading with GameState
  ed::GameState game;
  game.player.chronoCores = 0;
  game.player.autoClickerLevel = 0;
  game.player.autoClickerEnabled = false;

  REQUIRE_FALSE(ed::upgradeAutoClicker(game)); // Cannot afford

  game.player.chronoCores = 3;
  REQUIRE(ed::upgradeAutoClicker(game)); // Level 0 -> 1 (costs 1)
  REQUIRE(game.player.autoClickerLevel == 1);
  REQUIRE(game.player.autoClickerEnabled == true);
  REQUIRE(game.player.chronoCores == 2);

  REQUIRE(ed::upgradeAutoClicker(game)); // Level 1 -> 2 (costs 2)
  REQUIRE(game.player.autoClickerLevel == 2);
  REQUIRE(game.player.chronoCores == 0);

  REQUIRE_FALSE(ed::upgradeAutoClicker(game)); // Cannot afford next level
}

TEST_CASE("offline progress calculates clicks and applies rewards") {
  ed::GameState game;
  ed::InventoryState inventory;
  game.player.autoClickerLevel = 1;
  game.player.autoClickerEnabled = true;
  game.player.currentLayer = "ashen-warrens";

  std::vector<ed::LayerDef> layers = {
      {"ashen-warrens", "Ashen Warrens", 1, 1, 1}
  };
  std::vector<ed::MaterialDef> materials = {
      {"stone", "Stone", "ashen-warrens", "grey", 10, 10, 10, 1, 1},
      {"copper", "Copper Ore", "ashen-warrens", "orange", 20, 20, 25, 2, 1}
  };

  // Disabled auto clicker should result in 0 clicks
  game.player.autoClickerEnabled = false;
  auto disabledResult = ed::calculateOfflineProgress(game, inventory, layers, materials, 60);
  REQUIRE(disabledResult.clicks == 0);
  REQUIRE(disabledResult.blocksBroken == 0);

  // Enabled auto clicker (1 click/sec, 60 sec offline = 60 clicks)
  game.player.autoClickerEnabled = true;
  auto result = ed::calculateOfflineProgress(game, inventory, layers, materials, 60);
  REQUIRE(result.secondsOffline == 60);
  REQUIRE(result.clicks == 60);
  REQUIRE(result.blocksBroken > 0);
  REQUIRE(result.goldEarned > 0);
  REQUIRE(result.xpEarned > 0);

  const std::int64_t initialGold = game.player.gold;
  const int initialLevel = game.player.level;
  const std::int64_t initialBlocks = game.player.blocksBroken;

  ed::applyOfflineProgress(game, inventory, result);
  REQUIRE(game.player.gold == initialGold + result.goldEarned);
  REQUIRE(game.player.level >= initialLevel);
  REQUIRE(game.player.blocksBroken == initialBlocks + result.blocksBroken);
  REQUIRE(game.dirty);
}

TEST_CASE("miners vows can be upgraded, enforce caps, and refund on respec") {
  ed::GameState game;
  game.player.level = 20;
  game.player.vowPoints = 5;
  game.player.gold = 50000;

  // Normal vow upgrades
  REQUIRE(ed::canUpgradeVow(game, "strong_arms"));
  REQUIRE(ed::upgradeVow(game, "strong_arms"));
  REQUIRE(ed::currentVowRank(game, "strong_arms") == 1);
  REQUIRE(game.player.vowPoints == 4);

  // Capstone cannot be unlocked before level 30
  REQUIRE_FALSE(ed::canUpgradeVow(game, "shatterer"));
  REQUIRE_FALSE(ed::upgradeVow(game, "shatterer"));

  // Level up to 30
  game.player.level = 30;
  REQUIRE(ed::canUpgradeVow(game, "shatterer"));
  REQUIRE(ed::upgradeVow(game, "shatterer"));
  REQUIRE(ed::currentVowRank(game, "shatterer") == 1);
  REQUIRE(game.player.vowPoints == 3);

  // Capstone cannot exceed rank 1
  REQUIRE_FALSE(ed::canUpgradeVow(game, "shatterer"));

  // Check total spent
  REQUIRE(ed::totalVowPointsSpent(game) == 2);

  // Respec
  const int cost = ed::respecVowsCost(game);
  REQUIRE(cost == 30 * 100); // level * 100
  REQUIRE(ed::respecVows(game));
  REQUIRE(game.player.vowPoints == 5); // 3 + 2 refunded
  REQUIRE(game.player.vows.strongArms == 0);
  REQUIRE(game.player.vows.shatterer == 0);
  REQUIRE(game.player.gold == 50000 - cost);
}

TEST_CASE("deep strata blocks scale smoothly past depth 5 allowing maxed pickaxe progression") {
  const auto d5 = ed::deepStrataBlock(5);
  const auto d10 = ed::deepStrataBlock(10);
  const auto d15 = ed::deepStrataBlock(15);
  const auto d20 = ed::deepStrataBlock(20);

  // HP scales smoothly, not through billions at depth 10
  REQUIRE(d5.hp < 400000);
  REQUIRE(d10.hp < 1500000);
  REQUIRE(d15.hp < 5000000);
  REQUIRE(d20.hp < 20000000);

  // Build a maxed out +20 legendary pickaxe state with perks
  ed::GameState game;
  game.player.level = 80;
  game.player.emberPerks.power = 20; // +100% mining power
  game.player.vows.strongArms = 10;  // +30% mining power

  ed::InventoryState inv;
  ed::GearPiece maxPickaxe;
  maxPickaxe.slot = "pickaxe";
  maxPickaxe.itemId = "legendary-pick";
  maxPickaxe.name = "Aetherium Pickaxe";
  maxPickaxe.tier = 10;
  maxPickaxe.upgradeLevel = 20;
  maxPickaxe.legendary = true;
  inv.gear.push_back(maxPickaxe);
  inv.equipped.pickaxeId = "legendary-pick";

  const auto stats = ed::calculatePlayerStats(game, inv);

  // Mythic upgrades grant high mining power and bonus pierce
  REQUIRE(stats.miningPower > 50000.0);
  REQUIRE(stats.pierce >= 150);

  // Calculate damage against Aetherium (hardness 300)
  const double damagePerHit = ed::miningDamage(stats.miningPower, stats.miningPowerBonusPercent, false,
                                               1.5, 300, stats.pierce);
  REQUIRE(damagePerHit > 100000.0);

  // At Depth 5 (~373k HP), the block can be shattered in just ~3-4 normal hits (or 1-2 crits)
  const int hitsForDepth5 = static_cast<int>(std::ceil(static_cast<double>(d5.hp) / damagePerHit));
  REQUIRE(hitsForDepth5 <= 4);

  // At Depth 10 (~1.28M HP), it takes fewer than 15 hits
  const int hitsForDepth10 = static_cast<int>(std::ceil(static_cast<double>(d10.hp) / damagePerHit));
  REQUIRE(hitsForDepth10 <= 15);
}
