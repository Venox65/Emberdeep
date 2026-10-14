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


enum class DungeonEventType {
  FloorStart,
  EnemyAppear,
  PlayerAttack,
  EnemyAttack,
  EnemyAbility,
  PlayerDefeated,
  EnemyDefeated,
  PoisonTick,
  DungeonClear,
  DungeonFailed
};

struct DungeonCombatEvent {
  DungeonEventType type;
  std::string stringData; 
  std::int64_t numData1 = 0; // e.g. damage
  std::int64_t numData2 = 0; // e.g. current hp
  std::int64_t numData3 = 0; // e.g. max hp
  bool flag = false;         // e.g. crit
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
  int echoSigils = 0;
  int aetherShards = 0;
  bool isAbyss = false;
  std::vector<DungeonCombatEvent> log;
};


std::vector<DungeonZoneDef> defaultDungeonZones();
DungeonRunResult simulateDungeonRun(const DungeonPlayerStats& player,
                                    const DungeonZoneDef& zone,
                                    std::uint64_t seed);
DungeonRunResult simulateAbyssRun(const DungeonPlayerStats& player,
                                  std::uint64_t seed,
                                  int maxFloors = 100);

} // namespace ed
