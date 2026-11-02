#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "ed/data_model.hpp"
#include "ed/game_state.hpp"
#include "ed/inventory.hpp"

namespace ed {

struct DeepStrataBlock {
  int depth = 0;
  std::int64_t hp = 0;
  std::int64_t xp = 0;
  std::int64_t oreQuantity = 0;
  int aetherShardChanceBasisPoints = 0;
};

double echoDifficultyMultiplier(int echoRank);
DeepStrataBlock deepStrataBlock(int depth, std::int64_t baseHp = 108700, std::int64_t baseXp = 13200);
bool isDeepStrataUnlocked(const GameState& game);
int calculateAscensionEmberMarks(std::int64_t goldEarnedThisRun, int echoRank);

// Ascension prestige logic (resets run progression, keeps legendaries & gems, grants starter tier-3 gear if armory perk active)
int applyAscension(GameState& game, InventoryState* inventory = nullptr);

// Ember Perk bonus calculations
int perkMiningPowerBonusBasisPoints(const EmberPerks& perks);
int perkGoldBonusBasisPoints(const EmberPerks& perks);
int perkXpBonusBasisPoints(const EmberPerks& perks);
int perkUpgradeCost(const std::string& perkId, int currentLevel);
bool upgradeEmberPerk(GameState& game, const std::string& perkId);

// Echo Campaign progression helpers
bool isEchoCampaignUnlocked(const GameState& game);
bool isEchoBossCleared(const GameState& game, int bossIndex);
void recordEchoBossClear(GameState& game, int bossIndex);
int echoBossRewardGold(int baseGold, int echoRank);
int echoBossRewardSigils(int echoRank);

// Chrono Cores & Auto-Clicker progression
int calculateAscensionChronoCores(int ascensionsAlreadyDone, int earnedEmberMarks);
int autoClickerUpgradeCost(int currentLevel);
double autoClickerSpeed(int level);
bool upgradeAutoClicker(GameState& game);

// Miner's Vows (Perk / Skill Tree - Section 7.2)
int maxVowRank(const std::string& vowId);
int currentVowRank(const GameState& game, const std::string& vowId);
bool isVowCapstone(const std::string& vowId);
bool canUpgradeVow(const GameState& game, const std::string& vowId);
bool upgradeVow(GameState& game, const std::string& vowId);
int respecVowsCost(const GameState& game);
bool respecVows(GameState& game);
int totalVowPointsSpent(const GameState& game);
std::string vowDescription(const std::string& vowId);

// Offline Delving progression
struct OfflineProgressResult {
  std::int64_t secondsOffline = 0;
  std::int64_t clicks = 0;
  std::int64_t blocksBroken = 0;
  std::int64_t goldEarned = 0;
  std::int64_t xpEarned = 0;
  int aetherShardsEarned = 0;
  std::map<std::string, int> materialsEarned;
};

OfflineProgressResult calculateOfflineProgress(
    const GameState& game,
    const InventoryState& inventory,
    const std::vector<LayerDef>& layers,
    const std::vector<MaterialDef>& materials,
    std::int64_t secondsOffline);

void applyOfflineProgress(
    GameState& game,
    InventoryState& inventory,
    const OfflineProgressResult& result);

} // namespace ed
