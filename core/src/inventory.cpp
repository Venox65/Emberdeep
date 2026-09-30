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

  inventory.ore[type] += 0;
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
  if (piece.slot == "pickaxe") {
    inventory.equipped.pickaxeId = piece.itemId;
    return true;
  }
  if (piece.slot == "helm") {
    inventory.equipped.helmId = piece.itemId;
    return true;
  }
  if (piece.slot == "chestplate") {
    inventory.equipped.chestplateId = piece.itemId;
    return true;
  }
  if (piece.slot == "gauntlets") {
    inventory.equipped.gauntletsId = piece.itemId;
    return true;
  }
  if (piece.slot == "boots") {
    inventory.equipped.bootsId = piece.itemId;
    return true;
  }
  if (piece.slot == "amulet") {
    inventory.equipped.amuletId = piece.itemId;
    return true;
  }
  return false;
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

} // namespace ed
