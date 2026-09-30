#pragma once

#include "ed/types.hpp"

namespace ed {

inline MaterialList defaultMaterials() {
  return {
      {"ashen-stone", "Ashen Stone", "ashen-warrens", "#8a8a94", 20, 0, 5, 1, 1},
      {"copper", "Copper", "copper-crypt", "#c9773f", 52, 4, 12, 3, 5},
      {"iron", "Iron", "iron-hollows", "#7b8794", 135, 10, 29, 8, 10},
      {"silvervein", "Silvervein", "silvermire", "#d7e2ec", 350, 20, 69, 20, 15},
      {"aurelite", "Aurelite", "aurelite-keep", "#f2c14e", 915, 36, 166, 50, 20},
      {"runic-slate", "Runic Slate", "runic-archives", "#5a4fcf", 2380, 60, 398, 125, 27},
      {"starmetal", "Starmetal", "stargazer-spire", "#6fd3ff", 6190, 95, 955, 310, 34},
      {"wyrmite", "Wyrmite", "wyrmbone-caverns", "#d94f30", 16100, 145, 2290, 780, 42},
      {"voidglass", "Voidglass", "voidglass-sanctum", "#2b0f3a", 41800, 210, 5500, 1950, 50},
      {"aetherium", "Aetherium", "aether-vault", "#7cf7e0", 108700, 300, 13200, 4900, 60},
  };
}

inline LayerList defaultLayers() {
  return {
      {"ashen-warrens", "Ashen Warrens", 1, 1, 0},
      {"copper-crypt", "Copper Crypt", 2, 5, 1},
      {"iron-hollows", "Iron Hollows", 3, 10, 2},
      {"silvermire", "Silvermire", 4, 15, 3},
      {"aurelite-keep", "Aurelite Keep", 5, 20, 4},
      {"runic-archives", "Runic Archives", 6, 27, 5},
      {"stargazer-spire", "Stargazer Spire", 7, 34, 6},
      {"wyrmbone-caverns", "Wyrmbone Caverns", 8, 42, 7},
      {"voidglass-sanctum", "Voidglass Sanctum", 9, 50, 8},
      {"aether-vault", "The Aether Vault", 10, 60, 9},
  };
}

inline GemList defaultGems() {
  return {
      {"amber", "Amber", "common", "+1 Luck", 1},
      {"garnet", "Garnet", "common", "+2% Attack", 2},
      {"jade", "Jade", "uncommon", "+2% Max HP", 3},
      {"moonstone", "Moonstone", "uncommon", "+0.5% Crit Chance", 4},
      {"sapphire", "Sapphire", "rare", "+1% Focus", 5},
      {"bloodstone", "Bloodstone", "rare", "1% lifesteal", 6},
      {"star-ruby", "Star Ruby", "epic", "+3% Mining Power", 7},
      {"arcane-opal", "Arcane Opal", "epic", "+3% XP gain", 8},
      {"dragon-eye", "Dragon's Eye", "legendary", "+2 Pierce, +5% Crit Damage", 9},
      {"aether-diamond", "Aether Diamond", "legendary", "+1.5% to all stats", 10},
  };
}

inline VowNodeList defaultVows() {
  return {
      {"Delver", "strong-arms", "Strong Arms", 10, "+3% Mining Power per rank"},
      {"Delver", "keen-eye", "Keen Eye", 10, "+1% Crit Chance per rank"},
      {"Delver", "prospector", "Prospector", 10, "+1 Luck per rank"},
      {"Delver", "deep-breath", "Deep Breath", 5, "Weak Spot appears 5% more often"},
      {"Warrior", "stout-heart", "Stout Heart", 10, "+4% Max HP per rank"},
      {"Warrior", "whetstone", "Whetstone", 10, "+3% Attack per rank"},
      {"Warrior", "ironskin", "Ironskin", 10, "+2% Defense per rank"},
      {"Warrior", "swift-boots", "Swift Boots", 5, "+1 Speed per rank"},
      {"Scholar", "quick-study", "Quick Study", 10, "+3% XP per rank"},
      {"Scholar", "haggler", "Haggler", 10, "+3% gold from sales per rank"},
      {"Scholar", "steady-hand", "Steady Hand", 10, "+1% Focus per rank"},
      {"Scholar", "dust-collector", "Dust Collector", 5, "+2% Arcane Dust chance"},
  };
}

} // namespace ed
