#include "ed/inventory.hpp"
#include "ed/game_state.hpp"

#include <algorithm>

namespace ed {

bool addItemToInventory(InventoryState& inventory,
                        const std::string& type,
                        const std::string& itemId,
                        int quantity) {
  if (quantity <= 0) {
    return false;
  }

  if (type == "ore") {
    inventory.ore[itemId] += quantity;
    return true;
  }
  if (type == "ingots") {
    inventory.ingots[itemId] += quantity;
    return true;
  }
  if (type == "gems") {
    inventory.gems[itemId] += quantity;
    return true;
  }
  if (type == "trophies") {
    inventory.trophies[itemId] += quantity;
    return true;
  }
  return false;
}

bool removeItemFromInventory(InventoryState& inventory,
                           const std::string& type,
                           const std::string& itemId,
                           int quantity) {
  if (quantity <= 0) {
    return false;
  }

  auto* bucket = [&]() -> std::map<std::string, int>* {
    if (type == "ore") {
      return &inventory.ore;
    }
    if (type == "ingots") {
      return &inventory.ingots;
    }
    if (type == "gems") {
      return &inventory.gems;
    }
    if (type == "trophies") {
      return &inventory.trophies;
    }
    return nullptr;
  }();

  if (!bucket) {
    return false;
  }

  auto it = bucket->find(itemId);
  if (it == bucket->end() || it->second < quantity) {
    return false;
  }
  it->second -= quantity;
  if (it->second == 0) {
    bucket->erase(it);
  }
  return true;
}

bool equipGear(InventoryState& inventory, const GearPiece& piece) {
  bool equipped = false;
  if (piece.slot == "pickaxe") {
    inventory.equipped.pickaxeId = piece.itemId;
    equipped = true;
  }
  if (piece.slot == "helm") {
    inventory.equipped.helmId = piece.itemId;
    equipped = true;
  }
  if (piece.slot == "chestplate") {
    inventory.equipped.chestplateId = piece.itemId;
    equipped = true;
  }
  if (piece.slot == "gauntlets") {
    inventory.equipped.gauntletsId = piece.itemId;
    equipped = true;
  }
  if (piece.slot == "boots") {
    inventory.equipped.bootsId = piece.itemId;
    equipped = true;
  }
  if (piece.slot == "amulet") {
    inventory.equipped.amuletId = piece.itemId;
    equipped = true;
  }
  return equipped;
}

int gearSocketCount(int tier, bool legendary, int upgradeLevel) {
  int count = 0;
  if (tier >= 10) {
    count = (upgradeLevel >= 11) ? 4 : 3;
  } else if (tier >= 7) {
    count = 2;
  } else if (tier >= 4) {
    count = 1;
  } else {
    count = 0;
  }
  if (legendary) {
    count += 1;
  }
  return count;
}

int countItem(const InventoryState& inventory, const std::string& type, const std::string& itemId) {
  const std::map<std::string, int>* bucket = nullptr;
  if (type == "ore") {
    bucket = &inventory.ore;
  } else if (type == "ingots") {
    bucket = &inventory.ingots;
  } else if (type == "gems") {
    bucket = &inventory.gems;
  } else if (type == "trophies") {
    bucket = &inventory.trophies;
  }
  if (bucket == nullptr) {
    return 0;
  }
  const auto it = bucket->find(itemId);
  return it == bucket->end() ? 0 : it->second;
}

bool insertGem(InventoryState& inventory, GearPiece& piece, const std::string& gemId) {
  if (gemId.empty() || piece.socketedGems.size() >= static_cast<std::size_t>(piece.sockets) ||
      !removeItemFromInventory(inventory, "gems", gemId)) {
    return false;
  }
  piece.socketedGems.push_back(gemId);
  return true;
}

bool removeGem(InventoryState& inventory, GearPiece& piece, std::size_t socketIndex) {
  if (socketIndex >= piece.socketedGems.size()) return false;
  const std::string gemId = piece.socketedGems[socketIndex];
  piece.socketedGems.erase(piece.socketedGems.begin() + static_cast<std::ptrdiff_t>(socketIndex));
  return addItemToInventory(inventory, "gems", gemId);
}

struct LegendaryDef {
  const char* id;
  const char* name;
  const char* slot;
  int tier;
  const char* effect;
};

static constexpr LegendaryDef kLegendaries[10] = {
    {"burrowkings-crown", "Burrowking's Crown", "helm", 1, "Tremor: every 10th click deals x3 damage."},
    {"bonecleaver-gauntlets", "Bonecleaver Gauntlets", "gauntlets", 2, "Rattling Rhythm: consecutive crits stack +5% Crit Damage (max 10 stacks)."},
    {"cinderheart-amulet", "Cinderheart Amulet", "amulet", 3, "Cinderlight: 8% per click to ignite the block for 3 s, dealing 40% of Power per second."},
    {"hydrascale-cuirass", "Hydrascale Cuirass", "chestplate", 4, "Regrowth: heal 3% Max HP at the start of each dungeon round."},
    {"sunforged-pickaxe", "Sunforged Pickaxe", "pickaxe", 5, "Midas Strike: 10% chance on block break to grant bonus gold equal to 5x that ore's sell value."},
    {"runewardens-diadem", "Runewarden's Diadem", "helm", 6, "Scholar's Insight: +30% XP and Arcane Dust drops x1.5."},
    {"starfall-treads", "Starfall Treads", "boots", 7, "Stargazer's Stride: +15% Speed and always act first in round 1 of dungeon fights."},
    {"wyrmfang-breaker", "Wyrmfang Breaker", "pickaxe", 8, "Earthsplitter: crits deal +100% damage to blocks above 50% HP."},
    {"voidglass-phylactery", "Voidglass Phylactery", "amulet", 9, "Soul Siphon: heal 5% of damage dealt in the dungeon; +20% Pierce effect on blocks."},
    {"aetherbrand", "Aetherbrand, the Hollow Pick", "pickaxe", 10, "Hollow Echo: every click has a 15% chance to strike again for 60% damage."}
};

bool isLegendaryId(const std::string& id) {
  for (const auto& leg : kLegendaries) {
    if (id == leg.id) return true;
  }
  return false;
}

std::vector<std::string> allLegendaryIds() {
  std::vector<std::string> ids;
  ids.reserve(10);
  for (const auto& leg : kLegendaries) {
    ids.push_back(leg.id);
  }
  return ids;
}

std::string legendaryEffectDescription(const std::string& legendaryId) {
  for (const auto& leg : kLegendaries) {
    if (legendaryId == leg.id) return leg.effect;
  }
  return "";
}

GearPiece createLegendaryGear(const std::string& legendaryId) {
  for (const auto& leg : kLegendaries) {
    if (legendaryId == leg.id) {
      GearPiece piece;
      piece.slot = leg.slot;
      piece.itemId = leg.id;
      piece.name = leg.name;
      piece.tier = leg.tier;
      piece.upgradeLevel = 0;
      piece.legendary = true;
      piece.sockets = gearSocketCount(leg.tier, true, 0);
      return piece;
    }
  }
  GearPiece fallback;
  fallback.slot = "amulet";
  fallback.itemId = legendaryId;
  fallback.name = legendaryId;
  fallback.tier = 1;
  fallback.legendary = true;
  fallback.sockets = gearSocketCount(1, true, 0);
  return fallback;
}

GemCut gemCutFromId(const std::string& gemId) {
  if (gemId.rfind("flawless-", 0) == 0) return GemCut::Flawless;
  if (gemId.rfind("cut-", 0) == 0) return GemCut::Cut;
  return GemCut::Rough;
}

int gemCutMultiplier(const std::string& gemId) {
  switch (gemCutFromId(gemId)) {
    case GemCut::Flawless: return 4;
    case GemCut::Cut: return 2;
    case GemCut::Rough: return 1;
  }
  return 1;
}

std::string baseGemId(const std::string& gemId) {
  if (gemId.rfind("flawless-", 0) == 0) return gemId.substr(9);
  if (gemId.rfind("cut-", 0) == 0) return gemId.substr(4);
  if (gemId.rfind("rough-", 0) == 0) return gemId.substr(6);
  return gemId;
}

std::string gemDisplayName(const std::string& gemId) {
  const std::string base = baseGemId(gemId);
  std::string formattedBase = base;
  if (!formattedBase.empty()) {
    formattedBase[0] = static_cast<char>(std::toupper(formattedBase[0]));
    for (std::size_t i = 1; i < formattedBase.size(); ++i) {
      if (formattedBase[i - 1] == '-' || formattedBase[i - 1] == ' ') {
        formattedBase[i] = static_cast<char>(std::toupper(formattedBase[i]));
      }
    }
  }
  const GemCut cut = gemCutFromId(gemId);
  if (cut == GemCut::Flawless) return "Flawless " + formattedBase;
  if (cut == GemCut::Cut) return "Cut " + formattedBase;
  return "Rough " + formattedBase;
}

std::string nextGemCutId(const std::string& gemId) {
  const GemCut cut = gemCutFromId(gemId);
  const std::string base = baseGemId(gemId);
  if (cut == GemCut::Rough) return "cut-" + base;
  if (cut == GemCut::Cut) return "flawless-" + base;
  return "";
}

int gemCombineDustCost(GemCut cut) {
  if (cut == GemCut::Rough) return 10;
  if (cut == GemCut::Cut) return 30;
  return 0;
}

int gemCombineGoldCost(GemCut cut) {
  if (cut == GemCut::Rough) return 50;
  if (cut == GemCut::Cut) return 200;
  return 0;
}

int countRoughGems(const InventoryState& inventory, const std::string& base) {
  const std::string b = baseGemId(base);
  return countItem(inventory, "gems", b) + countItem(inventory, "gems", "rough-" + b);
}

bool canCombineGems(const InventoryState& inventory, const GameState& game, const std::string& gemId) {
  const GemCut cut = gemCutFromId(gemId);
  if (cut == GemCut::Flawless) return false;
  int count = 0;
  if (cut == GemCut::Rough) {
    count = countRoughGems(inventory, gemId);
  } else {
    count = countItem(inventory, "gems", gemId);
  }
  if (count < 3) return false;
  if (game.player.arcaneDust < gemCombineDustCost(cut)) return false;
  if (game.player.gold < gemCombineGoldCost(cut)) return false;
  return true;
}

bool combineGems(InventoryState& inventory, GameState& game, const std::string& gemId) {
  if (!canCombineGems(inventory, game, gemId)) return false;
  const GemCut cut = gemCutFromId(gemId);
  if (cut == GemCut::Rough) {
    const std::string b = baseGemId(gemId);
    int needed = 3;
    int have1 = countItem(inventory, "gems", gemId);
    int take1 = std::min(have1, needed);
    if (take1 > 0) {
      removeItemFromInventory(inventory, "gems", gemId, take1);
      needed -= take1;
    }
    if (needed > 0) {
      const std::string alt = (gemId == b) ? ("rough-" + b) : b;
      removeItemFromInventory(inventory, "gems", alt, needed);
    }
  } else {
    if (!removeItemFromInventory(inventory, "gems", gemId, 3)) return false;
  }
  game.player.arcaneDust -= gemCombineDustCost(cut);
  game.player.gold -= gemCombineGoldCost(cut);
  const std::string nextId = nextGemCutId(gemId);
  addItemToInventory(inventory, "gems", nextId, 1);
  game.dirty = true;
  return true;
}

} // namespace ed
