#pragma once

#include <string>
#include "ed/pcg.hpp"

#include <vector>

#include "ed/game_state.hpp"
#include "ed/inventory.hpp"

namespace ed {

struct CraftRecipe {
  std::string slot;
  int tier = 1;
  int ingots = 0;
  int trophies = 0;
  int gold = 0;
  bool requiresGem = false;
  std::string itemId;
  std::string itemName;
};

std::vector<CraftRecipe> defaultCraftRecipes(int maxTier = 10);
bool smeltOre(InventoryState& inventory, const std::string& oreId, int batches);
bool craftGear(GameState& game, InventoryState& inventory, const CraftRecipe& recipe);
bool enchantGearPiece(GameState& game, InventoryState& inventory, const std::string& itemId, Pcg32& rng);
int enchantDustCost(const GearPiece& piece);
int enchantGoldCost(const GearPiece& piece);

bool upgradeGear(GameState& game, GearPiece& piece);
int upgradeGoldCost(const GearPiece& piece);

} // namespace ed
