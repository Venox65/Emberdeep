#pragma once

#include <cstdint>

#include <string>
#include <vector>

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

} // namespace ed
