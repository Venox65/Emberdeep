#pragma once

#include <map>
#include <string>
#include <vector>

namespace ed {

struct GameState;

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
  std::vector<std::string> socketedGems;
  std::string enchantment;
  int enchantRerolls = 0;

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
int gearSocketCount(int tier, bool legendary = false, int upgradeLevel = 0);
int countItem(const InventoryState& inventory, const std::string& type, const std::string& itemId);
bool insertGem(InventoryState& inventory, GearPiece& piece, const std::string& gemId);
bool removeGem(InventoryState& inventory, GearPiece& piece, std::size_t socketIndex);

GearPiece createLegendaryGear(const std::string& legendaryId);
bool isLegendaryId(const std::string& id);
std::vector<std::string> allLegendaryIds();
std::string legendaryEffectDescription(const std::string& legendaryId);

enum class GemCut {
  Rough,
  Cut,
  Flawless
};

GemCut gemCutFromId(const std::string& gemId);
int gemCutMultiplier(const std::string& gemId);
std::string baseGemId(const std::string& gemId);
std::string gemDisplayName(const std::string& gemId);
std::string nextGemCutId(const std::string& gemId);
int gemCombineDustCost(GemCut cut);
int gemCombineGoldCost(GemCut cut);
int countRoughGems(const InventoryState& inventory, const std::string& base);
bool canCombineGems(const InventoryState& inventory, const GameState& game, const std::string& gemId);
bool combineGems(InventoryState& inventory, GameState& game, const std::string& gemId);

} // namespace ed
