#pragma once

#include <string>
#include <vector>

#include "ed/pcg.hpp"

namespace ed {

struct DungeonEnemyDef {
  std::string id;
  std::string name;
  std::string ability;
};

struct DungeonZoneDef {
  std::string id;
  std::string name;
  int tier = 1;
  std::vector<DungeonEnemyDef> enemies;
  DungeonEnemyDef miniBoss;
};

struct DungeonPlayerStats {
  std::int64_t maxHp = 100;
  std::int64_t attack = 10;
  std::int64_t defense = 0;
  std::int64_t speed = 10;
  int critChanceBasisPoints = 500;
  int critDamageBasisPoints = 15000;
};

struct DungeonRunResult {
  bool cleared = false;
  int floorsCleared = 0;
  std::int64_t gold = 0;
  int xp = 0;
  int trophies = 0;
  int arcaneDust = 0;
  bool gemFound = false;
  int rounds = 0;
};

std::vector<DungeonZoneDef> defaultDungeonZones();
DungeonRunResult simulateDungeonRun(const DungeonPlayerStats& player,
                                    const DungeonZoneDef& zone,
                                    std::uint64_t seed);

} // namespace ed
