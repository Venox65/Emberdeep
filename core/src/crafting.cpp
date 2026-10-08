#include "ed/crafting.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace ed {

namespace {

const char* tierName(int tier) {
  static constexpr const char* names[] = {
      "", "Ashen", "Copper", "Iron", "Silver", "Aurelite",
      "Runic", "Starmetal", "Wyrm", "Void", "Aether"};
  return tier >= 1 && tier <= 10 ? names[tier] : "Unknown";
}

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
      recipe.itemName = std::string(tierName(tier)) + " " + slotName(recipe.slot);
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

bool craftGear(GameState& game, InventoryState& inventory, const CraftRecipe& recipe) {
  if (game.player.gold < recipe.gold) {
    return false;
  }
  static const char* materialIds[] = {"ashen-stone", "copper", "iron", "silvervein", "aurelite",
                                      "runic-slate", "starmetal", "wyrmite", "voidglass",
                                      "aetherium"};
  const std::string materialId = materialIds[std::clamp(recipe.tier, 1, 10) - 1];
  if (countItem(inventory, "ingots", materialId) < recipe.ingots ||
      countItem(inventory, "trophies", "zone-" + std::to_string(recipe.tier)) < recipe.trophies ||
      (recipe.requiresGem && inventory.gems.empty())) {
    return false;
  }
  removeItemFromInventory(inventory, "ingots", materialId, recipe.ingots);
  if (recipe.trophies > 0) {
    removeItemFromInventory(inventory, "trophies", "zone-" + std::to_string(recipe.tier),
                            recipe.trophies);
  }
  if (recipe.requiresGem) {
    inventory.gems.begin()->second -= 1;
    if (inventory.gems.begin()->second == 0) inventory.gems.erase(inventory.gems.begin());
  }
  game.player.gold -= recipe.gold;
  GearPiece piece{recipe.slot, recipe.itemId, recipe.itemName, recipe.tier, 0,
                  gearSocketCount(recipe.tier), false};
  inventory.gear.push_back(piece);
  game.dirty = true;
  return true;
}

int upgradeGoldCost(const GearPiece& piece) {
  if (piece.upgradeLevel >= 10) return 0;
  return static_cast<int>(std::ceil(20.0 * std::pow(3.0, piece.tier - 1) *
                                    std::pow(piece.upgradeLevel + 1, 1.5)));
}

bool upgradeGear(GameState& game, GearPiece& piece) {
  const int cost = upgradeGoldCost(piece);
  if (cost == 0 || game.player.gold < cost ||
      game.player.arcaneDust < piece.upgradeLevel + 1) {
    return false;
  }
  game.player.gold -= cost;
  game.player.arcaneDust -= piece.upgradeLevel + 1;
  ++piece.upgradeLevel;
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
