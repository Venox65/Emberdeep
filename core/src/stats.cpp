#include "ed/stats.hpp"
#include "ed/postgame.hpp"

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
  if (piece.upgradeLevel <= 10) {
    return 1.0 + 0.08 * piece.upgradeLevel;
  }
  return 1.8 + 0.10 * (piece.upgradeLevel - 10);
}

double tierMultiplier(int tier) {
  return std::pow(2.3, std::max(0, tier - 1));
}

void applyGemStats(PlayerStats& stats, const std::string& gemId) {
  if (gemId == "amber") ++stats.luck;
  else if (gemId == "garnet") stats.attackBonusBasisPoints += 200;
  else if (gemId == "jade") stats.maxHpBonusBasisPoints += 200;
  else if (gemId == "moonstone") stats.critChanceBasisPoints += 50;
  else if (gemId == "sapphire") stats.focusBasisPoints += 100;
  else if (gemId == "bloodstone") stats.lifestealBasisPoints += 100;
  else if (gemId == "star-ruby") stats.miningPowerBonusPercent += 3.0;
  else if (gemId == "arcane-opal") stats.xpBonusBasisPoints += 300;
  else if (gemId == "dragon-eye") {
    stats.pierce += 2;
    stats.critDamageBasisPoints += 500;
  } else if (gemId == "aether-diamond") {
    stats.miningPowerBonusPercent += 1.5;
    stats.attackBonusBasisPoints += 150;
    stats.maxHpBonusBasisPoints += 150;
    stats.defenseBonusBasisPoints += 150;
    stats.speedBonusBasisPoints += 150;
    stats.luckBonusBasisPoints += 150;
    stats.pierceBonusBasisPoints += 150;
    stats.critChanceBasisPoints += 15;
    stats.critDamageBasisPoints += 150;
    stats.xpBonusBasisPoints += 150;
    stats.focusBasisPoints += 15;
  }
}

void applyEnchantmentStats(PlayerStats& stats, const std::string& enchant) {
  if (enchant.empty()) return;
  if (enchant == "mining-power") stats.miningPowerBonusPercent += 3.0;
  else if (enchant == "crit-chance") stats.critChanceBasisPoints += 200;
  else if (enchant == "luck") stats.luck += 2;
  else if (enchant == "max-hp") stats.maxHpBonusBasisPoints += 500;
  else if (enchant == "attack") stats.attackBonusBasisPoints += 500;
  else if (enchant == "speed") stats.speed += 2;
  else if (enchant == "pierce") stats.pierce += 1;
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
    for (const auto& gem : pickaxe->socketedGems) applyGemStats(stats, gem); applyEnchantmentStats(stats, pickaxe->enchantment);
  }

  const GearPiece* helm = equippedPiece(inventory, inventory.equipped.helmId);
  if (helm != nullptr) {
    stats.maxHp += static_cast<int>(std::llround(30.0 * tierMultiplier(helm->tier) *
                                                 upgradeMultiplier(*helm)));
    stats.luck += helm->tier;
    for (const auto& gem : helm->socketedGems) applyGemStats(stats, gem); applyEnchantmentStats(stats, helm->enchantment);
  }

  const GearPiece* chest = equippedPiece(inventory, inventory.equipped.chestplateId);
  if (chest != nullptr) {
    stats.defense += static_cast<int>(std::llround(4.0 * tierMultiplier(chest->tier) *
                                                   upgradeMultiplier(*chest)));
    stats.maxHp += static_cast<int>(std::llround(50.0 * tierMultiplier(chest->tier) *
                                                 upgradeMultiplier(*chest)));
    for (const auto& gem : chest->socketedGems) applyGemStats(stats, gem); applyEnchantmentStats(stats, chest->enchantment);
  }

  const GearPiece* gauntlets = equippedPiece(inventory, inventory.equipped.gauntletsId);
  if (gauntlets != nullptr) {
    stats.attack += static_cast<int>(std::llround(4.0 * tierMultiplier(gauntlets->tier) *
                                                  upgradeMultiplier(*gauntlets)));
    stats.critChanceBasisPoints += 200;
    for (const auto& gem : gauntlets->socketedGems) applyGemStats(stats, gem); applyEnchantmentStats(stats, gauntlets->enchantment);
  }

  const GearPiece* boots = equippedPiece(inventory, inventory.equipped.bootsId);
  if (boots != nullptr) {
    stats.speed += static_cast<int>(std::llround(1.5 * boots->tier));
    stats.maxHp += static_cast<int>(std::llround(15.0 * tierMultiplier(boots->tier) *
                                                 upgradeMultiplier(*boots)));
    for (const auto& gem : boots->socketedGems) applyGemStats(stats, gem); applyEnchantmentStats(stats, boots->enchantment);
  }

  const GearPiece* amulet = equippedPiece(inventory, inventory.equipped.amuletId);
  if (amulet != nullptr) {
    stats.luck += static_cast<int>(std::llround(2.0 * tierMultiplier(amulet->tier) *
                                                upgradeMultiplier(*amulet)));
    stats.xpBonusBasisPoints += 300;
    stats.critDamageBasisPoints += 500;
    for (const auto& gem : amulet->socketedGems) applyGemStats(stats, gem); applyEnchantmentStats(stats, amulet->enchantment);
  }

  stats.miningPowerBonusPercent += game.player.level * 1.5;
  stats.miningPowerBonusPercent += perkMiningPowerBonusBasisPoints(game.player.emberPerks) / 100.0;
  stats.goldBonusBasisPoints += perkGoldBonusBasisPoints(game.player.emberPerks);
  stats.xpBonusBasisPoints += perkXpBonusBasisPoints(game.player.emberPerks);

  stats.attack += static_cast<int>(std::llround(stats.attack * stats.attackBonusBasisPoints / 10000.0));
  stats.maxHp += static_cast<int>(std::llround(stats.maxHp * stats.maxHpBonusBasisPoints / 10000.0));
  stats.defense += static_cast<int>(std::llround(stats.defense * stats.defenseBonusBasisPoints / 10000.0));
  stats.speed += static_cast<int>(std::llround(stats.speed * stats.speedBonusBasisPoints / 10000.0));
  stats.luck += static_cast<int>(std::llround(stats.luck * stats.luckBonusBasisPoints / 10000.0));
  stats.pierce += static_cast<int>(std::llround(stats.pierce * stats.pierceBonusBasisPoints / 10000.0));
  return stats;
}

} // namespace ed
