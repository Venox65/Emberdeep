#include "ed/crafting.hpp"
#include "ed/postgame.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace ed {

const char* tierName(int tier) {
  static constexpr const char* names[] = {
      "", "Ashen", "Copper", "Iron", "Silver", "Aurelite",
      "Runic", "Starmetal", "Wyrm", "Void", "Aether"};
  return tier >= 1 && tier <= 10 ? names[tier] : "Unknown";
}

const char* fantasyGearName(int tier, const std::string& slot) {
  static constexpr const char* names[10][6] = {
      // Pickaxe, Helm, Chestplate, Gauntlets, Boots, Amulet
      {"Stone Pick", "Ash Cap", "Ash Jerkin", "Ash Wraps", "Ash Sandals", "Pebble Charm"},
      {"Copper Pick", "Copper Coif", "Copper Cuirass", "Copper Grips", "Copper Greaves", "Copper Locket"},
      {"Iron Pick", "Iron Helm", "Iron Hauberk", "Iron Gauntlets", "Iron Sabatons", "Iron Sigil"},
      {"Silver Pick", "Silver Circlet", "Silver Mail", "Silver Gloves", "Silver Treads", "Silver Pendant"},
      {"Gilded Pick", "Gilded Helm", "Gilded Plate", "Gilded Fists", "Gilded Boots", "Gilded Amulet"},
      {"Runepick", "Rune Hood", "Rune Robe", "Rune Mitts", "Rune Slippers", "Rune Talisman"},
      {"Starbreaker", "Star Helm", "Star Plate", "Star Gauntlets", "Star Greaves", "Star Choker"},
      {"Wyrmfang Pick", "Wyrm Helm", "Wyrm Hide", "Wyrm Claws", "Wyrm Boots", "Wyrm Talon"},
      {"Voidpick", "Void Cowl", "Void Shroud", "Void Grasp", "Void Steps", "Void Eye"},
      {"Aetherpick", "Aether Crown", "Aether Plate", "Aether Hands", "Aether Stride", "Aether Heart"}};

  const int t = std::clamp(tier, 1, 10) - 1;
  int s = 0;
  if (slot == "pickaxe") s = 0;
  else if (slot == "helm") s = 1;
  else if (slot == "chestplate") s = 2;
  else if (slot == "gauntlets") s = 3;
  else if (slot == "boots") s = 4;
  else if (slot == "amulet") s = 5;
  return names[t][s];
}

namespace {

const char* slotName(const std::string& slot) {
  if (slot == "pickaxe") return "Pickaxe";
  if (slot == "helm") return "Helm";
  if (slot == "chestplate") return "Chestplate";
  if (slot == "gauntlets") return "Gauntlets";
  if (slot == "boots") return "Boots";
  return "Amulet";
}

int tierFromOreId(const std::string& oreId) {
  static const char* ids[] = {"ashen-stone", "copper", "iron", "silvervein", "aurelite",
                              "runic-slate", "starmetal", "wyrmite", "voidglass", "aetherium"};
  for (int i = 0; i < 10; ++i) {
    if (oreId == ids[i]) return i + 1;
  }
  return 0;
}

} // namespace

std::vector<CraftRecipe> defaultCraftRecipes(int maxTier) {
  const int safeMaxTier = std::clamp(maxTier, 1, 10);
  const std::vector<std::pair<std::string, int>> costs = {
      {"pickaxe", 8}, {"helm", 6}, {"chestplate", 10},
      {"gauntlets", 5}, {"boots", 5}, {"amulet", 3}};
  const std::vector<int> trophies = {2, 3, 4, 2, 2, 2};
  const std::vector<int> gold = {60, 40, 80, 40, 40, 50};
  std::vector<CraftRecipe> recipes;
  for (int tier = 1; tier <= safeMaxTier; ++tier) {
    for (std::size_t i = 0; i < costs.size(); ++i) {
      CraftRecipe recipe;
      recipe.slot = costs[i].first;
      recipe.tier = tier;
      recipe.ingots = costs[i].second;
      recipe.trophies = tier == 1 ? 0 : trophies[i];
      recipe.gold = gold[i] * static_cast<int>(std::pow(3.0, tier - 1));
      recipe.requiresGem = recipe.slot == "amulet";
      recipe.itemId = std::string(tierName(tier)) + "-" + recipe.slot;
      recipe.itemName = fantasyGearName(tier, recipe.slot);
      recipes.push_back(recipe);
    }
  }
  return recipes;
}

bool smeltOre(InventoryState& inventory, const std::string& oreId, int batches) {
  if (batches <= 0 || countItem(inventory, "ore", oreId) < batches * 4) {
    return false;
  }
  removeItemFromInventory(inventory, "ore", oreId, batches * 4);
  addItemToInventory(inventory, "ingots", oreId, batches);
  return true;
}

bool canCraftGear(const GameState& game, const InventoryState& inventory, const CraftRecipe& recipe) {
  if (game.player.gold < recipe.gold) {
    return false;
  }
  static const char* materialIds[] = {"ashen-stone", "copper", "iron", "silvervein", "aurelite",
                                      "runic-slate", "starmetal", "wyrmite", "voidglass",
                                      "aetherium"};
  const std::string materialId = materialIds[std::clamp(recipe.tier, 1, 10) - 1];
  if (countItem(inventory, "ingots", materialId) < recipe.ingots ||
      (recipe.trophies > 0 && countItem(inventory, "trophies", "zone-" + std::to_string(recipe.tier)) < recipe.trophies)) {
    return false;
  }
  if (recipe.requiresGem) {
    bool hasAnyGem = false;
    for (const auto& [id, cnt] : inventory.gems) {
      if (cnt > 0) {
        hasAnyGem = true;
        break;
      }
    }
    if (!hasAnyGem) return false;
  }
  return true;
}

bool craftGear(GameState& game, InventoryState& inventory, const CraftRecipe& recipe) {
  if (!canCraftGear(game, inventory, recipe)) {
    return false;
  }
  static const char* materialIds[] = {"ashen-stone", "copper", "iron", "silvervein", "aurelite",
                                      "runic-slate", "starmetal", "wyrmite", "voidglass",
                                      "aetherium"};
  const std::string materialId = materialIds[std::clamp(recipe.tier, 1, 10) - 1];
  removeItemFromInventory(inventory, "ingots", materialId, recipe.ingots);
  if (recipe.trophies > 0) {
    removeItemFromInventory(inventory, "trophies", "zone-" + std::to_string(recipe.tier),
                            recipe.trophies);
  }
  if (recipe.requiresGem) {
    for (auto it = inventory.gems.begin(); it != inventory.gems.end(); ++it) {
      if (it->second > 0) {
        it->second -= 1;
        if (it->second == 0) {
          inventory.gems.erase(it);
        }
        break;
      }
    }
  }
  game.player.gold -= recipe.gold;
  int sockets = gearSocketCount(recipe.tier);
  if (recipe.slot == "pickaxe") {
    sockets = std::max(1, sockets);
  }
  GearPiece piece{recipe.slot, recipe.itemId, recipe.itemName, recipe.tier, 0,
                  sockets, false};
  inventory.gear.push_back(piece);
  game.dirty = true;
  return true;
}

bool isMythicUpgradeUnlocked(const GameState& game) {
  return isEchoCampaignUnlocked(game);
}

int maxGearUpgradeLevel(const GameState& game) {
  return isMythicUpgradeUnlocked(game) ? 20 : 10;
}

int upgradeGoldCost(const GearPiece& piece) {
  if (piece.upgradeLevel >= 20) return 0;
  return static_cast<int>(std::ceil(20.0 * std::pow(3.0, piece.tier - 1) *
                                    std::pow(piece.upgradeLevel + 1, 1.5)));
}

int upgradeDustCost(const GearPiece& piece) {
  if (piece.upgradeLevel >= 20) return 0;
  return piece.upgradeLevel + 1;
}

int upgradeAetherShardCost(const GearPiece& piece) {
  if (piece.upgradeLevel < 10 || piece.upgradeLevel >= 20) return 0;
  return piece.upgradeLevel + 1 - 10;
}

int upgradeEchoSigilCost(const GearPiece& piece) {
  if (piece.upgradeLevel < 10 || piece.upgradeLevel >= 20) return 0;
  return piece.upgradeLevel + 1 - 10;
}

bool canUpgradeGear(const GameState& game, const GearPiece& piece) {
  if (piece.upgradeLevel >= maxGearUpgradeLevel(game)) {
    return false;
  }
  if (piece.upgradeLevel >= 10 && !isMythicUpgradeUnlocked(game)) {
    return false;
  }
  const int gold = upgradeGoldCost(piece);
  const int dust = upgradeDustCost(piece);
  const int shards = upgradeAetherShardCost(piece);
  const int sigils = upgradeEchoSigilCost(piece);

  return game.player.gold >= gold &&
         game.player.arcaneDust >= dust &&
         game.player.aetherShards >= shards &&
         game.player.echoSigils >= sigils;
}

bool upgradeGear(GameState& game, GearPiece& piece) {
  if (!canUpgradeGear(game, piece)) {
    return false;
  }
  const int gold = upgradeGoldCost(piece);
  const int dust = upgradeDustCost(piece);
  const int shards = upgradeAetherShardCost(piece);
  const int sigils = upgradeEchoSigilCost(piece);

  game.player.gold -= gold;
  game.player.arcaneDust -= dust;
  game.player.aetherShards -= shards;
  game.player.echoSigils -= sigils;
  ++piece.upgradeLevel;

  if (piece.tier >= 10 && piece.upgradeLevel >= 11) {
    piece.sockets = gearSocketCount(piece.tier, piece.legendary, piece.upgradeLevel);
  }

  game.dirty = true;
  return true;
}


int enchantDustCost(const GearPiece& piece) {
  return 5 * piece.tier + piece.enchantRerolls * 5;
}

int enchantGoldCost(const GearPiece& piece) {
  return 50 * piece.tier + piece.enchantRerolls * 50;
}

bool enchantGearPiece(GameState& game, InventoryState& inventory, const std::string& itemId, Pcg32& rng) {
  GearPiece* target = nullptr;
  for (auto& piece : inventory.gear) {
    if (piece.itemId == itemId) {
      target = &piece;
      break;
    }
  }
  if (!target) return false;

  int dust = enchantDustCost(*target);
  int gold = enchantGoldCost(*target);

  if (game.player.arcaneDust < dust || game.player.gold < gold) {
    return false;
  }

  game.player.arcaneDust -= dust;
  game.player.gold -= gold;

  const char* enchants[] = {
    "mining-power", "crit-chance", "luck", "max-hp",
    "attack", "speed", "pierce"
  };
  int choice = rng.nextRange(0, 6);
  target->enchantment = enchants[choice];
  target->enchantRerolls++;
  game.dirty = true;
  return true;
}
} // namespace ed
