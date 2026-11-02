#include "ed/postgame.hpp"

#include <algorithm>
#include <cmath>

#include "ed/formulas.hpp"
#include "ed/stats.hpp"

namespace ed {

double echoDifficultyMultiplier(int echoRank) {
  return std::min(2.0, 1.0 + 0.08 * std::max(0, echoRank - 1));
}

DeepStrataBlock deepStrataBlock(int depth, std::int64_t baseHp, std::int64_t baseXp) {
  const int safeDepth = std::max(0, depth);
  const double depthValue = static_cast<double>(safeDepth);
  DeepStrataBlock block;
  block.depth = safeDepth;
  block.hp = static_cast<std::int64_t>(std::llround(baseHp * std::pow(1.28, depthValue)));
  block.xp = baseXp;
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
  const int earnedCores = calculateAscensionChronoCores(game.player.ascensions, earnedMarks);
  ++game.player.ascensions;
  game.player.emberMarks += earnedMarks;
  game.player.chronoCores += earnedCores;
  game.player.level = 1;
  game.player.xp = 0;
  game.player.gold = 0;
  game.player.arcaneDust = 0;
  game.player.vowPoints = 0;
  game.player.vows = MinersVows{};
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

int calculateAscensionChronoCores(int ascensionsAlreadyDone, int earnedEmberMarks) {
  if (ascensionsAlreadyDone <= 0) {
    return 1;
  }
  return 1 + std::max(0, earnedEmberMarks / 4);
}

int autoClickerUpgradeCost(int currentLevel) {
  if (currentLevel >= 10) {
    return 0;
  }
  if (currentLevel <= 0) {
    return 1;
  }
  return currentLevel + 1;
}

double autoClickerSpeed(int level) {
  if (level <= 0) {
    return 0.0;
  }
  return static_cast<double>(level);
}

bool upgradeAutoClicker(GameState& game) {
  if (game.player.autoClickerLevel >= 10) {
    return false;
  }
  const int cost = autoClickerUpgradeCost(game.player.autoClickerLevel);
  if (game.player.chronoCores < cost) {
    return false;
  }
  game.player.chronoCores -= cost;
  ++game.player.autoClickerLevel;
  game.player.autoClickerEnabled = true;
  game.dirty = true;
  return true;
}

OfflineProgressResult calculateOfflineProgress(
    const GameState& game,
    const InventoryState& inventory,
    const std::vector<LayerDef>& layers,
    const std::vector<MaterialDef>& materials,
    std::int64_t secondsOffline) {
  OfflineProgressResult result;
  if (secondsOffline < 5 || game.player.autoClickerLevel <= 0 || !game.player.autoClickerEnabled) {
    return result;
  }

  constexpr std::int64_t kMaxOfflineSeconds = 24 * 3600;
  const std::int64_t effectiveSeconds = std::min(secondsOffline, kMaxOfflineSeconds);
  result.secondsOffline = effectiveSeconds;

  const double speed = autoClickerSpeed(game.player.autoClickerLevel);
  const std::int64_t totalClicks = static_cast<std::int64_t>(std::llround(static_cast<double>(effectiveSeconds) * speed));
  result.clicks = totalClicks;
  if (totalClicks <= 0) {
    return result;
  }

  const auto stats = calculatePlayerStats(game, inventory);

  std::string targetLayerId = game.player.currentLayer;
  if (targetLayerId.empty()) {
    targetLayerId = "ashen-warrens";
  }

  const MaterialDef* mat = nullptr;
  for (const auto& m : materials) {
    if (m.layer == targetLayerId) {
      mat = &m;
      break;
    }
  }
  if (!mat && !materials.empty()) {
    mat = &materials[0];
  }
  if (!mat) {
    return result;
  }

  const double critChance = std::clamp(static_cast<double>(stats.critChanceBasisPoints) / 10000.0, 0.0, 1.0);
  const double critMult = std::max(1.0, static_cast<double>(stats.critDamageBasisPoints) / 10000.0);
  const double normDmg = miningDamage(stats.miningPower, stats.miningPowerBonusPercent, false, 1.0, mat->hardness, stats.pierce);
  const double critDmg = miningDamage(stats.miningPower, stats.miningPowerBonusPercent, true, critMult, mat->hardness, stats.pierce);
  const double avgDmg = normDmg * (1.0 - critChance) + critDmg * critChance;

  const double totalDmg = avgDmg * static_cast<double>(totalClicks);
  const double avgBlockHp = std::max(1.0, static_cast<double>(mat->hp) * 1.35);
  const std::int64_t blocks = std::max<std::int64_t>(1, static_cast<std::int64_t>(totalDmg / avgBlockHp));
  result.blocksBroken = blocks;

  const std::int64_t totalOre = std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(static_cast<double>(blocks) * 1.25)));
  result.materialsEarned[mat->id] = static_cast<int>(std::min<std::int64_t>(1000000000, totalOre));

  result.goldEarned = static_cast<std::int64_t>(std::llround(static_cast<double>(blocks * mat->sell) * 1.25));
  result.xpEarned = std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(static_cast<double>(blocks * mat->xp) * 0.001)));

  if (targetLayerId == "aether-vault") {
    result.aetherShardsEarned = static_cast<int>(blocks / 50);
  }

  return result;
}

void applyOfflineProgress(
    GameState& game,
    InventoryState& inventory,
    const OfflineProgressResult& result) {
  if (result.clicks <= 0) return;

  game.player.clicks += static_cast<int>(std::min<std::int64_t>(result.clicks, 1000000000));
  game.player.blocksBroken += static_cast<int>(std::min<std::int64_t>(result.blocksBroken, 1000000000));
  game.player.gold += static_cast<int>(std::min<std::int64_t>(result.goldEarned, 2000000000));
  game.player.goldEarnedThisRun += static_cast<int>(std::min<std::int64_t>(result.goldEarned, 2000000000));
  applyXp(game, result.xpEarned);
  game.player.aetherShards += result.aetherShardsEarned;

  for (const auto& [matId, qty] : result.materialsEarned) {
    addItemToInventory(inventory, "ore", matId, qty);
  }
  game.dirty = true;
}

static int* vowPointer(MinersVows& vows, const std::string& vowId) {
  if (vowId == "strong_arms") return &vows.strongArms;
  if (vowId == "keen_eye") return &vows.keenEye;
  if (vowId == "prospector") return &vows.prospector;
  if (vowId == "deep_breath") return &vows.deepBreath;
  if (vowId == "shatterer") return &vows.shatterer;

  if (vowId == "stout_heart") return &vows.stoutHeart;
  if (vowId == "whetstone") return &vows.whetstone;
  if (vowId == "ironskin") return &vows.ironskin;
  if (vowId == "swift_boots") return &vows.swiftBoots;
  if (vowId == "second_wind") return &vows.secondWind;

  if (vowId == "quick_study") return &vows.quickStudy;
  if (vowId == "haggler") return &vows.haggler;
  if (vowId == "steady_hand") return &vows.steadyHand;
  if (vowId == "dust_collector") return &vows.dustCollector;
  if (vowId == "gem_whisperer") return &vows.gemWhisperer;
  return nullptr;
}

static const int* vowPointer(const MinersVows& vows, const std::string& vowId) {
  return vowPointer(const_cast<MinersVows&>(vows), vowId);
}

int maxVowRank(const std::string& vowId) {
  if (vowId == "shatterer" || vowId == "second_wind" || vowId == "gem_whisperer") return 1;
  if (vowId == "deep_breath" || vowId == "swift_boots" || vowId == "dust_collector") return 5;
  if (vowId == "strong_arms" || vowId == "keen_eye" || vowId == "prospector" ||
      vowId == "stout_heart" || vowId == "whetstone" || vowId == "ironskin" ||
      vowId == "quick_study" || vowId == "haggler" || vowId == "steady_hand") return 10;
  return 0;
}

int currentVowRank(const GameState& game, const std::string& vowId) {
  const int* p = vowPointer(game.player.vows, vowId);
  return p ? *p : 0;
}

bool isVowCapstone(const std::string& vowId) {
  return (vowId == "shatterer" || vowId == "second_wind" || vowId == "gem_whisperer");
}

bool canUpgradeVow(const GameState& game, const std::string& vowId) {
  if (game.player.vowPoints <= 0) return false;
  const int cur = currentVowRank(game, vowId);
  const int maxR = maxVowRank(vowId);
  if (maxR <= 0 || cur >= maxR) return false;
  if (isVowCapstone(vowId) && game.player.level < 30) return false;
  return true;
}

bool upgradeVow(GameState& game, const std::string& vowId) {
  if (!canUpgradeVow(game, vowId)) return false;
  int* p = vowPointer(game.player.vows, vowId);
  if (!p) return false;
  --game.player.vowPoints;
  ++(*p);
  game.dirty = true;
  return true;
}

int respecVowsCost(const GameState& game) {
  return std::max(100, game.player.level * 100);
}

int totalVowPointsSpent(const GameState& game) {
  const auto& v = game.player.vows;
  return v.strongArms + v.keenEye + v.prospector + v.deepBreath + v.shatterer +
         v.stoutHeart + v.whetstone + v.ironskin + v.swiftBoots + v.secondWind +
         v.quickStudy + v.haggler + v.steadyHand + v.dustCollector + v.gemWhisperer;
}

bool respecVows(GameState& game) {
  const int cost = respecVowsCost(game);
  if (game.player.gold < cost) return false;
  const int spent = totalVowPointsSpent(game);
  if (spent <= 0) return false;
  game.player.gold -= cost;
  game.player.vowPoints += spent;
  game.player.vows = MinersVows{};
  game.dirty = true;
  return true;
}

std::string vowDescription(const std::string& vowId) {
  if (vowId == "strong_arms") return "+3% Mining Power per rank";
  if (vowId == "keen_eye") return "+1% Crit Chance per rank";
  if (vowId == "prospector") return "+1 Luck per rank";
  if (vowId == "deep_breath") return "Weak Spot appears 5% more often per rank";
  if (vowId == "shatterer") return "Every 25th hit is a guaranteed crit (Lv 30 Capstone)";

  if (vowId == "stout_heart") return "+4% Max HP per rank";
  if (vowId == "whetstone") return "+3% Attack per rank";
  if (vowId == "ironskin") return "+2% Defense per rank";
  if (vowId == "swift_boots") return "+1 Speed per rank";
  if (vowId == "second_wind") return "Once per dungeon run, revive at 30% HP (Lv 30 Capstone)";

  if (vowId == "quick_study") return "+3% XP per rank";
  if (vowId == "haggler") return "+3% gold from sales per rank";
  if (vowId == "steady_hand") return "+1% Focus per rank";
  if (vowId == "dust_collector") return "+2% Arcane Dust drop chance per rank";
  if (vowId == "gem_whisperer") return "+3 Gem Hollow daily swings (Lv 30 Capstone)";
  return "";
}

} // namespace ed
