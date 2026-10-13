#include "ed/postgame.hpp"

#include <algorithm>
#include <cmath>

namespace ed {

double echoDifficultyMultiplier(int echoRank) {
  return std::min(2.0, 1.0 + 0.08 * std::max(0, echoRank - 1));
}

DeepStrataBlock deepStrataBlock(int depth, std::int64_t baseHp, std::int64_t baseXp) {
  const int safeDepth = std::max(0, depth);
  const double depthValue = static_cast<double>(safeDepth);
  DeepStrataBlock block;
  block.depth = safeDepth;
  block.hp = static_cast<std::int64_t>(std::llround(baseHp * std::pow(2.6, depthValue)));
  block.xp = static_cast<std::int64_t>(std::llround(baseXp * std::pow(2.4, depthValue)));
  block.oreQuantity = std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(1.0 + 0.5 * depthValue)));
  block.aetherShardChanceBasisPoints = std::min(10000, safeDepth * 250);
  return block;
}

int calculateAscensionEmberMarks(std::int64_t goldEarnedThisRun, int echoRank) {
  const auto safeGold = std::max<std::int64_t>(0, goldEarnedThisRun);
  const int goldMarks = static_cast<int>(std::floor(std::sqrt(static_cast<double>(safeGold) / 50000.0)));
  return goldMarks + 2 * std::max(0, echoRank);
}

int applyAscension(GameState& game, InventoryState* inventory) {
  const int earnedMarks = calculateAscensionEmberMarks(game.player.goldEarnedThisRun, game.player.echoRank);
  ++game.player.ascensions;
  game.player.emberMarks += earnedMarks;
  game.player.level = 1;
  game.player.xp = 0;
  game.player.gold = 0;
  game.player.arcaneDust = 0;
  game.player.vowPoints = 0;
  game.player.blocksBroken = 0;
  game.player.clicks = 0;
  game.player.pickaxeTier = 1;
  game.player.currentLayer = "ashen-warrens";
  game.player.goldEarnedThisRun = 0;

  if (inventory != nullptr) {
    inventory->ore.clear();
    inventory->ingots.clear();
    inventory->trophies.clear();

    // Retain only legendary gear
    inventory->gear.erase(
        std::remove_if(inventory->gear.begin(), inventory->gear.end(),
                       [](const GearPiece& p) { return !p.legendary; }),
        inventory->gear.end());

    // If armory perk is active, grant starter tier-3 (Iron) set
    if (game.player.emberPerks.armory >= 1) {
      game.player.pickaxeTier = 3;
      const std::vector<GearPiece> starterSet = {
          {"pickaxe", "iron-pickaxe", "Iron Pickaxe", 3, 0, 0, false},
          {"helm", "iron-helm", "Iron Helm", 3, 0, 0, false},
          {"chestplate", "iron-hauberk", "Iron Hauberk", 3, 0, 0, false},
          {"gauntlets", "iron-gauntlets", "Iron Gauntlets", 3, 0, 0, false},
          {"boots", "iron-sabatons", "Iron Sabatons", 3, 0, 0, false},
          {"amulet", "iron-sigil", "Iron Sigil", 3, 0, 0, false},
      };
      for (const auto& item : starterSet) {
        inventory->gear.push_back(item);
      }
    }

    // Ensure equipped IDs are valid
    auto ensureEquipped = [&](std::string& equippedId, const std::string& slot) {
      bool stillExists = false;
      for (const auto& piece : inventory->gear) {
        if (piece.itemId == equippedId) {
          stillExists = true;
          break;
        }
      }
      if (!stillExists) {
        equippedId.clear();
        for (const auto& piece : inventory->gear) {
          if (piece.slot == slot) {
            equippedId = piece.itemId;
            break;
          }
        }
      }
    };

    ensureEquipped(inventory->equipped.pickaxeId, "pickaxe");
    ensureEquipped(inventory->equipped.helmId, "helm");
    ensureEquipped(inventory->equipped.chestplateId, "chestplate");
    ensureEquipped(inventory->equipped.gauntletsId, "gauntlets");
    ensureEquipped(inventory->equipped.bootsId, "boots");
    ensureEquipped(inventory->equipped.amuletId, "amulet");
  }

  game.dirty = true;
  return earnedMarks;
}

int perkMiningPowerBonusBasisPoints(const EmberPerks& perks) {
  return std::clamp(perks.power, 0, 20) * 500;
}

int perkGoldBonusBasisPoints(const EmberPerks& perks) {
  return std::clamp(perks.wealth, 0, 20) * 500;
}

int perkXpBonusBasisPoints(const EmberPerks& perks) {
  return std::clamp(perks.wisdom, 0, 20) * 500;
}

int perkUpgradeCost(const std::string& perkId, int currentLevel) {
  if (perkId == "power" || perkId == "wealth" || perkId == "wisdom" || perkId == "potential") {
    return currentLevel + 1;
  }
  if (perkId == "armory") {
    return 5;
  }
  if (perkId == "smelting") {
    return (currentLevel + 1) * 2;
  }
  return 1;
}

bool upgradeEmberPerk(GameState& game, const std::string& perkId) {
  int* target = nullptr;
  int maxRank = 20;
  if (perkId == "power") {
    target = &game.player.emberPerks.power;
  } else if (perkId == "wealth") {
    target = &game.player.emberPerks.wealth;
  } else if (perkId == "wisdom") {
    target = &game.player.emberPerks.wisdom;
  } else if (perkId == "potential") {
    target = &game.player.emberPerks.potential;
    maxRank = 24;
  } else if (perkId == "armory") {
    target = &game.player.emberPerks.armory;
    maxRank = 1;
  } else if (perkId == "smelting") {
    target = &game.player.emberPerks.smelting;
    maxRank = 5;
  }

  if (target == nullptr || *target >= maxRank) {
    return false;
  }

  const int cost = perkUpgradeCost(perkId, *target);
  if (game.player.emberMarks < cost) {
    return false;
  }

  game.player.emberMarks -= cost;
  ++(*target);
  game.dirty = true;
  return true;
}

bool isDeepStrataUnlocked(const GameState& game) {
  return game.player.bossesCleared >= 10 || game.player.ascensions > 0 || game.player.deepestDepth > 0;
}

bool isEchoCampaignUnlocked(const GameState& game) {
  return game.player.bossesCleared >= 10 || game.player.ascensions > 0;
}

bool isEchoBossCleared(const GameState& game, int bossIndex) {
  if (bossIndex < 0 || bossIndex >= static_cast<int>(game.player.echoCleared.size())) {
    return false;
  }
  return game.player.echoCleared[static_cast<std::size_t>(bossIndex)];
}

void recordEchoBossClear(GameState& game, int bossIndex) {
  if (bossIndex < 0 || bossIndex >= 10) {
    return;
  }
  if (game.player.echoCleared.size() < 10) {
    game.player.echoCleared.resize(10, false);
  }
  game.player.echoCleared[static_cast<std::size_t>(bossIndex)] = true;

  bool allCleared = true;
  for (int i = 0; i < 10; ++i) {
    if (!game.player.echoCleared[static_cast<std::size_t>(i)]) {
      allCleared = false;
      break;
    }
  }

  if (allCleared) {
    game.player.echoRank = std::max(1, game.player.echoRank + 1);
    std::fill(game.player.echoCleared.begin(), game.player.echoCleared.end(), false);
  }
  game.dirty = true;
}

int echoBossRewardGold(int baseGold, int echoRank) {
  const double factor = 1.0 + 0.25 * std::max(0, echoRank - 1);
  return std::max(baseGold, static_cast<int>(std::llround(baseGold * factor)));
}

int echoBossRewardSigils(int echoRank) {
  return std::max(1, echoRank);
}

} // namespace ed
