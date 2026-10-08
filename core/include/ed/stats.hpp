#pragma once

#include "ed/game_state.hpp"
#include "ed/inventory.hpp"

namespace ed {

struct PlayerStats {
  double miningPower = 3.0;
  double miningPowerBonusPercent = 0.0;
  int pierce = 0;
  int critChanceBasisPoints = 500;
  int critDamageBasisPoints = 15000;
  int maxHp = 100;
  int attack = 10;
  int attackBonusBasisPoints = 0;
  int defense = 0;
  int speed = 10;
  int speedBonusBasisPoints = 0;
  int luck = 0;
  int luckBonusBasisPoints = 0;
  int pierceBonusBasisPoints = 0;
  int defenseBonusBasisPoints = 0;
  int maxHpBonusBasisPoints = 0;
  int xpBonusBasisPoints = 0;
  int focusBasisPoints = 0;
  int lifestealBasisPoints = 0;
};

PlayerStats calculatePlayerStats(const GameState& game, const InventoryState& inventory);

} // namespace ed
