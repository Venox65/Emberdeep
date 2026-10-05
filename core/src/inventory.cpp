#include "ed/inventory.hpp"

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

int gearSocketCount(int tier, bool legendary) {
  if (legendary) {
    return std::max(0, ((tier >= 10) ? 3 : (tier >= 7 ? 2 : (tier >= 4 ? 1 : 0))) + 1);
  }
  if (tier >= 10) {
    return 3;
  }
  if (tier >= 7) {
    return 2;
  }
  if (tier >= 4) {
    return 1;
  }
  return 0;
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

} // namespace ed
