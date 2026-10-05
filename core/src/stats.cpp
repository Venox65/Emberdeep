#include "ed/stats.hpp"

#include <algorithm>
#include <cmath>

namespace ed {

namespace {

const GearPiece* equippedPiece(const InventoryState& inventory, const std::string& id) {
  for (const auto& piece : inventory.gear) {
    if (piece.itemId == id) return &piece;
  }
  return nullptr;
}

double upgradeMultiplier(const GearPiece& piece) {
  return 1.0 + 0.08 * piece.upgradeLevel;
}

double tierMultiplier(int tier) {
  return std::pow(2.3, std::max(0, tier - 1));
}

} // namespace

PlayerStats calculatePlayerStats(const GameState& game, const InventoryState& inventory) {
  PlayerStats stats;
  stats.miningPower = 3.0 + game.player.level * 0.15;
  stats.attack = 10 + game.player.level;
  stats.maxHp = 100 + game.player.level * 3;
  stats.speed = 10 + game.player.level / 3;

  const GearPiece* pickaxe = equippedPiece(inventory, inventory.equipped.pickaxeId);
  if (pickaxe != nullptr) {
    const double tier = static_cast<double>(pickaxe->tier);
    stats.miningPower = 3.0 * std::pow(2.6, tier - 1) * upgradeMultiplier(*pickaxe);
    stats.attack += static_cast<int>(std::llround(2.0 * tierMultiplier(pickaxe->tier) *
                                                  upgradeMultiplier(*pickaxe)));
    stats.pierce += (pickaxe->tier + 1) / 2;
    stats.critChanceBasisPoints += 500;
  }

  const GearPiece* helm = equippedPiece(inventory, inventory.equipped.helmId);
  if (helm != nullptr) {
    stats.maxHp += static_cast<int>(std::llround(30.0 * tierMultiplier(helm->tier) *
                                                 upgradeMultiplier(*helm)));
    stats.luck += helm->tier;
  }

  const GearPiece* chest = equippedPiece(inventory, inventory.equipped.chestplateId);
  if (chest != nullptr) {
    stats.defense += static_cast<int>(std::llround(4.0 * tierMultiplier(chest->tier) *
                                                   upgradeMultiplier(*chest)));
    stats.maxHp += static_cast<int>(std::llround(50.0 * tierMultiplier(chest->tier) *
                                                 upgradeMultiplier(*chest)));
  }

  const GearPiece* gauntlets = equippedPiece(inventory, inventory.equipped.gauntletsId);
  if (gauntlets != nullptr) {
    stats.attack += static_cast<int>(std::llround(4.0 * tierMultiplier(gauntlets->tier) *
                                                  upgradeMultiplier(*gauntlets)));
    stats.critChanceBasisPoints += 200;
  }

  const GearPiece* boots = equippedPiece(inventory, inventory.equipped.bootsId);
  if (boots != nullptr) {
    stats.speed += static_cast<int>(std::llround(1.5 * boots->tier));
    stats.maxHp += static_cast<int>(std::llround(15.0 * tierMultiplier(boots->tier) *
                                                 upgradeMultiplier(*boots)));
  }

  const GearPiece* amulet = equippedPiece(inventory, inventory.equipped.amuletId);
  if (amulet != nullptr) {
    stats.luck += static_cast<int>(std::llround(2.0 * tierMultiplier(amulet->tier) *
                                                upgradeMultiplier(*amulet)));
    stats.xpBonusBasisPoints += 300;
    stats.critDamageBasisPoints += 500;
  }

  stats.miningPowerBonusPercent += game.player.level * 1.5;
  return stats;
}

} // namespace ed
