#pragma once

#include <string>
#include <vector>

#include "ed/data_model.hpp"
#include "ed/formulas.hpp"

namespace ed {

struct EmberPerks {
  int power = 0;      // +5% Mining Power per rank (max 20)
  int wealth = 0;     // +5% Gold earned per rank (max 20)
  int wisdom = 0;     // +5% XP gained per rank (max 20)
  int potential = 0;  // +5 Level cap per 3 ranks (max 24)
  int armory = 0;     // 1: Start each run with tier-3 gear set (max 1)
  int smelting = 0;   // Smelting efficiency / bonus ingots (max 5)
};

struct MinersVows {
  // Delver branch (mining)
  int strongArms = 0;    // +3% Mining Power per rank (max 10)
  int keenEye = 0;       // +1% Crit Chance per rank (max 10)
  int prospector = 0;    // +1 Luck per rank (max 10)
  int deepBreath = 0;    // Weak Spot appears 5% more often per rank (max 5)
  int shatterer = 0;     // Every 25th hit is a guaranteed crit (max 1, Lv 30 capstone)

  // Warrior branch (combat)
  int stoutHeart = 0;    // +4% Max HP per rank (max 10)
  int whetstone = 0;     // +3% Attack per rank (max 10)
  int ironskin = 0;      // +2% Defense per rank (max 10)
  int swiftBoots = 0;    // +1 Speed per rank (max 5)
  int secondWind = 0;    // Once per dungeon run, revive at 30% HP (max 1, Lv 30 capstone)

  // Scholar branch (economy)
  int quickStudy = 0;    // +3% XP per rank (max 10)
  int haggler = 0;       // +3% gold from sales per rank (max 10)
  int steadyHand = 0;    // +1% Focus per rank (max 10)
  int dustCollector = 0; // +2% Arcane Dust chance per rank (max 5)
  int gemWhisperer = 0;  // +3 Gem Hollow swings (max 1, Lv 30 capstone)
};

struct PlayerState {
  int level = 1;
  int xp = 0;
  int gold = 0;
  int arcaneDust = 0;
  int bossEssence = 0;
  int echoSigils = 0;
  int aetherShards = 0;
  int emberMarks = 0;
  int vowPoints = 0;
  int blocksBroken = 0;
  int clicks = 0;
  int bossesCleared = 0;
  int ascensions = 0;
  int echoRank = 0;
  int deepestDepth = 0;
  int goldEarnedThisRun = 0;
  int pickaxeTier = 1;
  std::string currentLayer = "ashen-warrens";
  int abyssBestFloor = 0;
  EmberPerks emberPerks;
  MinersVows vows;
  std::vector<bool> echoCleared;
  std::string lastGemHollowDate = "";
  int gemHollowStreak = 0;
  int gemHollowSwingsRemaining = 0;
  int gemHollowTodayGems = 0;
  int gemHollowTodayDust = 0;
  int gemHollowTodayGold = 0;
  bool gemHollowActive = false;
  std::vector<int> gemHollowVeins;
  int weakSpotsHit = 0;
  std::vector<std::string> achievements;
  int chronoCores = 0;
  int autoClickerLevel = 0;
  bool autoClickerEnabled = true;
  std::int64_t lastSaveTime = 0;

  // Hollow Descent
  int descentRuns = 0;
  int descentVictories = 0;
  int descentBestChamber = 0;
  int descentVaultsCracked = 0;
  int descentMaxRunLevel = 0;
  int descentEndlessBestChamber = 0;
};

struct GameState {
  int schemaVersion = 1;
  PlayerState player;
  bool dirty = false;
  std::string saveId = "emberdeep-save";
};

bool canUnlockLayer(const GameState& game, const LayerDef& layer);
int playerLevelCap(const PlayerState& player);
void applyXp(GameState& game, std::int64_t earnedXp);
std::string serializeGameState(const GameState& game);
bool deserializeGameState(const std::string& text, GameState& game);

} // namespace ed
