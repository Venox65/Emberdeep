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
  return 1.8 + 0.20 * (piece.upgradeLevel - 10);
}

double tierMultiplier(int tier) {
  return std::pow(2.3, std::max(0, tier - 1));
}

void applyGemStats(PlayerStats& stats, const std::string& gemId) {
  const int mult = gemCutMultiplier(gemId);
  const std::string base = baseGemId(gemId);
  if (base == "amber") stats.luck += 1 * mult;
  else if (base == "garnet") stats.attackBonusBasisPoints += 200 * mult;
  else if (base == "jade") stats.maxHpBonusBasisPoints += 200 * mult;
  else if (base == "moonstone") stats.critChanceBasisPoints += 50 * mult;
  else if (base == "sapphire") stats.focusBasisPoints += 100 * mult;
  else if (base == "bloodstone") stats.lifestealBasisPoints += 100 * mult;
  else if (base == "star-ruby") stats.miningPowerBonusPercent += 3.0 * mult;
  else if (base == "arcane-opal") stats.xpBonusBasisPoints += 300 * mult;
  else if (base == "dragon-eye") {
    stats.pierce += 2 * mult;
    stats.critDamageBasisPoints += 500 * mult;
  } else if (base == "aether-diamond") {
    stats.miningPowerBonusPercent += 1.5 * mult;
    stats.attackBonusBasisPoints += 150 * mult;
    stats.maxHpBonusBasisPoints += 150 * mult;
    stats.defenseBonusBasisPoints += 150 * mult;
    stats.speedBonusBasisPoints += 150 * mult;
    stats.luckBonusBasisPoints += 150 * mult;
    stats.pierceBonusBasisPoints += 150 * mult;
    stats.critChanceBasisPoints += 15 * mult;
    stats.critDamageBasisPoints += 150 * mult;
    stats.xpBonusBasisPoints += 150 * mult;
    stats.focusBasisPoints += 15 * mult;
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
    const double legMult = pickaxe->legendary ? 1.5 : 1.0;
    const double tier = static_cast<double>(pickaxe->tier);
    stats.miningPower = 3.0 * std::pow(2.6, tier - 1) * upgradeMultiplier(*pickaxe) * legMult;
    stats.attack += static_cast<int>(std::llround(2.0 * tierMultiplier(pickaxe->tier) *
                                                  upgradeMultiplier(*pickaxe) * legMult));
    stats.pierce += (pickaxe->tier + 1) / 2;
    if (pickaxe->upgradeLevel > 10) {
      stats.pierce += (pickaxe->upgradeLevel - 10) * 15;
    }
    stats.critChanceBasisPoints += 500;
    for (const auto& gem : pickaxe->socketedGems) applyGemStats(stats, gem); applyEnchantmentStats(stats, pickaxe->enchantment);
  }

  const GearPiece* helm = equippedPiece(inventory, inventory.equipped.helmId);
  if (helm != nullptr) {
    const double legMult = helm->legendary ? 1.5 : 1.0;
    stats.maxHp += static_cast<int>(std::llround(30.0 * tierMultiplier(helm->tier) *
                                                 upgradeMultiplier(*helm) * legMult));
    stats.luck += static_cast<int>(std::llround(helm->tier * legMult));
    for (const auto& gem : helm->socketedGems) applyGemStats(stats, gem); applyEnchantmentStats(stats, helm->enchantment);
  }

  const GearPiece* chest = equippedPiece(inventory, inventory.equipped.chestplateId);
  if (chest != nullptr) {
    const double legMult = chest->legendary ? 1.5 : 1.0;
    stats.defense += static_cast<int>(std::llround(4.0 * tierMultiplier(chest->tier) *
                                                   upgradeMultiplier(*chest) * legMult));
    stats.maxHp += static_cast<int>(std::llround(50.0 * tierMultiplier(chest->tier) *
                                                 upgradeMultiplier(*chest) * legMult));
    for (const auto& gem : chest->socketedGems) applyGemStats(stats, gem); applyEnchantmentStats(stats, chest->enchantment);
  }

  const GearPiece* gauntlets = equippedPiece(inventory, inventory.equipped.gauntletsId);
  if (gauntlets != nullptr) {
    const double legMult = gauntlets->legendary ? 1.5 : 1.0;
    stats.attack += static_cast<int>(std::llround(4.0 * tierMultiplier(gauntlets->tier) *
                                                  upgradeMultiplier(*gauntlets) * legMult));
    stats.critChanceBasisPoints += 200;
    for (const auto& gem : gauntlets->socketedGems) applyGemStats(stats, gem); applyEnchantmentStats(stats, gauntlets->enchantment);
  }

  const GearPiece* boots = equippedPiece(inventory, inventory.equipped.bootsId);
  if (boots != nullptr) {
    const double legMult = boots->legendary ? 1.5 : 1.0;
    stats.speed += static_cast<int>(std::llround(1.5 * boots->tier * legMult));
    stats.maxHp += static_cast<int>(std::llround(15.0 * tierMultiplier(boots->tier) *
                                                 upgradeMultiplier(*boots) * legMult));
    for (const auto& gem : boots->socketedGems) applyGemStats(stats, gem); applyEnchantmentStats(stats, boots->enchantment);
  }

  const GearPiece* amulet = equippedPiece(inventory, inventory.equipped.amuletId);
  if (amulet != nullptr) {
    const double legMult = amulet->legendary ? 1.5 : 1.0;
    stats.luck += static_cast<int>(std::llround(2.0 * tierMultiplier(amulet->tier) *
                                                upgradeMultiplier(*amulet) * legMult));
    stats.xpBonusBasisPoints += 300;
    stats.critDamageBasisPoints += 500;
    for (const auto& gem : amulet->socketedGems) applyGemStats(stats, gem); applyEnchantmentStats(stats, amulet->enchantment);
  }

  // Section 8.6 Legendary Set Bonus:
  int equippedLegendaries = 0;
  if (pickaxe != nullptr && pickaxe->legendary) ++equippedLegendaries;
  if (helm != nullptr && helm->legendary) ++equippedLegendaries;
  if (chest != nullptr && chest->legendary) ++equippedLegendaries;
  if (gauntlets != nullptr && gauntlets->legendary) ++equippedLegendaries;
  if (boots != nullptr && boots->legendary) ++equippedLegendaries;
  if (amulet != nullptr && amulet->legendary) ++equippedLegendaries;

  if (equippedLegendaries >= 2) {
    stats.miningPowerBonusPercent += 10.0;
    stats.luckBonusBasisPoints += 1000;
  }
  if (equippedLegendaries >= 4) {
    stats.attackBonusBasisPoints += 2500;
    stats.maxHpBonusBasisPoints += 2500;
    stats.defenseBonusBasisPoints += 2500;
  }
  if (equippedLegendaries >= 6) {
    stats.xpBonusBasisPoints += 5000;
  }

  // Miner's Vows bonuses (Section 7.2)
  stats.miningPowerBonusPercent += game.player.vows.strongArms * 3.0;
  stats.critChanceBasisPoints += game.player.vows.keenEye * 100;
  stats.luck += game.player.vows.prospector;
  stats.maxHpBonusBasisPoints += game.player.vows.stoutHeart * 400;
  stats.attackBonusBasisPoints += game.player.vows.whetstone * 300;
  stats.defenseBonusBasisPoints += game.player.vows.ironskin * 200;
  stats.speed += game.player.vows.swiftBoots;
  stats.xpBonusBasisPoints += game.player.vows.quickStudy * 300;
  stats.goldBonusBasisPoints += game.player.vows.haggler * 300;
  stats.focusBasisPoints += game.player.vows.steadyHand * 100;

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
