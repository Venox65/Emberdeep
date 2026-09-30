#pragma once

#include <map>
#include <string>
#include <vector>

namespace ed {

struct InventoryItem {
  std::string id;
  std::string name;
  int count = 0;
  int tier = 1;
  int sellValue = 0;
};

struct GearPiece {
  std::string slot;
  std::string itemId;
  std::string name;
  int tier = 1;
  int upgradeLevel = 0;
  int sockets = 0;
  bool legendary = false;
};

struct GearSet {
  std::string pickaxeId;
  std::string helmId;
  std::string chestplateId;
  std::string gauntletsId;
  std::string bootsId;
  std::string amuletId;
};

struct InventoryState {
  std::map<std::string, int> ore;
  std::map<std::string, int> ingots;
  std::map<std::string, int> gems;
  std::map<std::string, int> trophies;
  std::vector<GearPiece> gear;
  GearSet equipped;
};

bool addItemToInventory(InventoryState& inventory,
                        const std::string& type,
                        const std::string& itemId,
                        int quantity = 1);
bool removeItemFromInventory(InventoryState& inventory,
                           const std::string& type,
                           const std::string& itemId,
                           int quantity = 1);
bool equipGear(InventoryState& inventory, const GearPiece& piece);
int gearSocketCount(int tier, bool legendary = false);

} // namespace ed
