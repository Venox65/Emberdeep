#include "ed/descent/combat_sim.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace ed::descent {

void spawnEnemy(DescentSimState& sim, EnemyArchetype type, Vec2 pos, bool isElite) {
  DescentEnemy e;
  e.id = static_cast<int>(sim.nextEntityId++);
  e.archetype = type;
  e.pos = pos;
  e.isElite = isElite;

  switch (type) {
    case EnemyArchetype::Swarmer:
      e.name = "Bone Rat";
      e.hp = e.maxHp = 18.0F;
      e.contactDamage = 6.0F;
      e.speed = 90.0F;
      e.radius = 12.0F;
      e.xpReward = 1.8F;
      break;
    case EnemyArchetype::Grunt:
      e.name = "Skeleton Thrall";
      e.hp = e.maxHp = 65.0F;
      e.contactDamage = 12.0F;
      e.speed = 70.0F;
      e.radius = 14.0F;
      e.xpReward = 3.8F;
      break;
    case EnemyArchetype::Shooter:
      e.name = "Bone Archer";
      e.hp = e.maxHp = 48.0F;
      e.contactDamage = 7.0F;
      e.speed = 60.0F;
      e.radius = 13.0F;
      e.xpReward = 4.5F;
      e.shootCooldown = 1.2F + static_cast<float>(rand() % 10) * 0.1F;
      break;
    case EnemyArchetype::Charger:
      e.name = "Ghoul";
      e.hp = e.maxHp = 95.0F;
      e.contactDamage = 18.0F;
      e.speed = 60.0F;
      e.radius = 16.0F;
      e.xpReward = 7.0F;
      break;
    case EnemyArchetype::Brute:
      e.name = "Crypt Guard";
      e.hp = e.maxHp = 320.0F;
      e.armor = 20.0F;
      e.contactDamage = 25.0F;
      e.speed = 45.0F;
      e.radius = 22.0F;
      e.xpReward = 20.0F;
      break;
    case EnemyArchetype::Splitter:
      e.name = "Bone Golem";
      e.hp = e.maxHp = 75.0F;
      e.contactDamage = 11.0F;
      e.speed = 65.0F;
      e.radius = 15.0F;
      e.xpReward = 6.0F;
      break;
    case EnemyArchetype::Exploder:
      e.name = "Volatile Corpse";
      e.hp = e.maxHp = 42.0F;
      e.contactDamage = 10.0F;
      e.speed = 85.0F;
      e.radius = 13.0F;
      e.xpReward = 4.5F;
      break;
    case EnemyArchetype::Support:
      e.name = "Necrolyte";
      e.hp = e.maxHp = 85.0F;
      e.contactDamage = 8.0F;
      e.speed = 55.0F;
      e.radius = 15.0F;
      e.xpReward = 7.0F;
      e.supportTimer = 2.5F;
      break;
    case EnemyArchetype::Spawner:
      e.name = "Bone Hive";
      e.hp = e.maxHp = 160.0F;
      e.contactDamage = 10.0F;
      e.speed = 30.0F;
      e.radius = 20.0F;
      e.xpReward = 10.0F;
      e.spawnTimer = 3.5F;
      break;
    case EnemyArchetype::Stalker:
      e.name = "Shadow Stalker";
      e.hp = e.maxHp = 52.0F;
      e.contactDamage = 16.0F;
      e.speed = 105.0F;
      e.radius = 13.0F;
      e.armor = 5.0F;
      e.xpReward = 6.5F;
      e.supportTimer = 1.0F; // blink cooldown
      break;
    case EnemyArchetype::VoidCaster:
      e.name = "Void Weaver";
      e.hp = e.maxHp = 78.0F;
      e.contactDamage = 9.0F;
      e.speed = 52.0F;
      e.radius = 15.0F;
      e.armor = 4.0F;
      e.xpReward = 8.0F;
      e.shootCooldown = 1.5F;
      break;
  }

  // Apply heat and chamber difficulty multipliers
  // Scaling: HP & damage increase every consecutive room; speed increases once every 5 rooms
  const int roomIdx = std::max(1, sim.chamberIndex);
  const float loopHpMult = sim.isEndless ? (1.0F + 0.35F * static_cast<float>(sim.endlessLoop - 1)) : 1.0F;
  const float loopDmgMult = sim.isEndless ? (1.0F + 0.20F * static_cast<float>(sim.endlessLoop - 1)) : 1.0F;
  const float roomHpScale = (1.0F + static_cast<float>(roomIdx - 1) * 0.07F) * loopHpMult;
  const float roomDmgScale = (1.0F + static_cast<float>(roomIdx - 1) * 0.045F) * loopDmgMult;
  const float roomSpeedScale = 1.0F + static_cast<float>((roomIdx - 1) / 5) * 0.05F;

  e.hp *= (sim.heatHpMult * roomHpScale);
  e.maxHp = e.hp;
  e.contactDamage *= (sim.heatDamageMult * roomDmgScale);
  e.speed *= (sim.heatSpeedMult * roomSpeedScale);

  if (isElite) {
    e.hp *= 3.5F;
    e.maxHp = e.hp;
    e.contactDamage *= 1.3F;
    e.xpReward *= 3.0F;
    e.radius *= 1.25F;
    e.affixes.push_back("Hasty");
    e.speed *= 1.2F;
  }

  sim.enemies.push_back(e);
}

void spawnMiniBoss(DescentSimState& sim, Vec2 pos) {
  const int roomIdx = std::max(1, sim.chamberIndex);
  const float loopHpMult = sim.isEndless ? (1.0F + 0.35F * static_cast<float>(sim.endlessLoop - 1)) : 1.0F;
  const float loopDmgMult = sim.isEndless ? (1.0F + 0.20F * static_cast<float>(sim.endlessLoop - 1)) : 1.0F;
  const float roomHpScale = (1.0F + static_cast<float>(roomIdx - 1) * 0.07F) * loopHpMult;
  const float roomDmgScale = (1.0F + static_cast<float>(roomIdx - 1) * 0.045F) * loopDmgMult;
  const float roomSpeedScale = 1.0F + static_cast<float>((roomIdx - 1) / 5) * 0.05F;

  DescentEnemy e;
  e.id = static_cast<int>(sim.nextEntityId++);
  e.archetype = EnemyArchetype::Charger;
  e.name = "Bone Captain Rattlewick";
  e.hp = e.maxHp = 1200.0F * sim.heatHpMult * roomHpScale;
  e.armor = 18.0F;
  e.speed = 65.0F * sim.heatSpeedMult * roomSpeedScale;
  e.contactDamage = 30.0F * sim.heatDamageMult * roomDmgScale;
  e.radius = 30.0F;
  e.isMiniBoss = true;
  e.isElite = true;
  e.xpReward = 75.0F;
  e.emberChance = 1.0F;
  e.pos = pos;

  sim.bossEntityId = e.id;
  sim.bossName = e.name;
  sim.bossTitle = "The Undying Vanguard";
  sim.bossMaxHp = e.maxHp;
  sim.bossCurrentHp = e.hp;
  sim.bossPhase = 1;

  sim.enemies.push_back(e);
}

void spawnBossOsric(DescentSimState& sim, Vec2 pos) {
  const int roomIdx = std::max(1, sim.chamberIndex);
  const float loopHpMult = sim.isEndless ? (1.0F + 0.40F * static_cast<float>(sim.endlessLoop - 1)) : 1.0F;
  const float loopDmgMult = sim.isEndless ? (1.0F + 0.25F * static_cast<float>(sim.endlessLoop - 1)) : 1.0F;
  const float roomHpScale = (1.0F + static_cast<float>(roomIdx - 1) * 0.07F) * loopHpMult;
  const float roomDmgScale = (1.0F + static_cast<float>(roomIdx - 1) * 0.045F) * loopDmgMult;
  const float roomSpeedScale = 1.0F + static_cast<float>((roomIdx - 1) / 5) * 0.05F;

  DescentEnemy e;
  e.id = static_cast<int>(sim.nextEntityId++);
  e.archetype = EnemyArchetype::Brute;
  e.name = sim.isEndless ? ("Catacomb Overlord (Loop " + std::to_string(sim.endlessLoop) + ")") : "Grave Warden Osric";
  e.hp = e.maxHp = 2800.0F * sim.heatHpMult * roomHpScale;
  e.armor = 25.0F + (sim.isEndless ? static_cast<float>(sim.endlessLoop * 2) : 0.0F);
  e.speed = 55.0F * sim.heatSpeedMult * roomSpeedScale;
  e.contactDamage = 38.0F * sim.heatDamageMult * roomDmgScale;
  e.radius = 38.0F;
  e.isBoss = true;
  e.bossPhase = 1;
  e.xpReward = 200.0F * (sim.isEndless ? static_cast<float>(sim.endlessLoop) : 1.0F);
  e.emberChance = 1.0F;
  e.pos = pos;

  sim.bossEntityId = e.id;
  sim.bossName = e.name;
  sim.bossTitle = sim.isEndless ? ("Terror of Abyss Loop " + std::to_string(sim.endlessLoop)) : "Keeper of the Hollow Threshold";
  sim.bossMaxHp = e.maxHp;
  sim.bossCurrentHp = e.hp;
  sim.bossPhase = 1;

  sim.enemies.push_back(e);
}

static Vec2 getRandomSpawnPos(const DescentSimState& sim, float minPlayerDist = 175.0F) {
  const float worldW = static_cast<float>(sim.roomWidthTiles) * DescentSimState::kTileSize;
  const float worldH = static_cast<float>(sim.roomHeightTiles) * DescentSimState::kTileSize;

  for (int attempt = 0; attempt < 8; ++attempt) {
    Vec2 pos;
    const int pattern = rand() % 4;
    if (pattern == 0) {
      // Outer perimeter with variable depth (45 to 140)
      const float margin = 45.0F + static_cast<float>(rand() % 95);
      const int side = rand() % 4;
      if (side == 0) pos = Vec2{margin + static_cast<float>(rand() % static_cast<int>(std::max(10.0F, worldW - 2.0F * margin))), margin};
      else if (side == 1) pos = Vec2{worldW - margin, margin + static_cast<float>(rand() % static_cast<int>(std::max(10.0F, worldH - 2.0F * margin)))};
      else if (side == 2) pos = Vec2{margin + static_cast<float>(rand() % static_cast<int>(std::max(10.0F, worldW - 2.0F * margin))), worldH - margin};
      else pos = Vec2{margin, margin + static_cast<float>(rand() % static_cast<int>(std::max(10.0F, worldH - 2.0F * margin)))};
    } else if (pattern == 1) {
      // Corner cluster ambush
      const int corner = rand() % 4;
      const float cx = (corner % 2 == 0) ? 70.0F : worldW - 70.0F;
      const float cy = (corner / 2 == 0) ? 70.0F : worldH - 70.0F;
      const float r = static_cast<float>(rand() % 80);
      const float a = static_cast<float>(rand() % 628) / 100.0F;
      pos = Vec2{cx + std::cos(a) * r, cy + std::sin(a) * r};
    } else if (pattern == 2) {
      // Side flank inward
      const bool left = (rand() % 2 == 0);
      const float fx = left ? (50.0F + static_cast<float>(rand() % 120)) : (worldW - 50.0F - static_cast<float>(rand() % 120));
      const float fy = 60.0F + static_cast<float>(rand() % static_cast<int>(std::max(10.0F, worldH - 120.0F)));
      pos = Vec2{fx, fy};
    } else {
      // General arena interior scatter
      const float rx = 70.0F + static_cast<float>(rand() % static_cast<int>(std::max(10.0F, worldW - 140.0F)));
      const float ry = 70.0F + static_cast<float>(rand() % static_cast<int>(std::max(10.0F, worldH - 140.0F)));
      pos = Vec2{rx, ry};
    }

    pos.x = std::clamp(pos.x, 40.0F, worldW - 40.0F);
    pos.y = std::clamp(pos.y, 40.0F, worldH - 40.0F);

    if ((pos - sim.playerPos).lengthSq() >= minPlayerDist * minPlayerDist) {
      return pos;
    }
  }

  // Fallback: spawn on opposite side of arena from player
  Vec2 oppPos{worldW - sim.playerPos.x, worldH - sim.playerPos.y};
  oppPos.x = std::clamp(oppPos.x, 50.0F, worldW - 50.0F);
  oppPos.y = std::clamp(oppPos.y, 50.0F, worldH - 50.0F);
  return oppPos;
}

void spawnChamberWave(DescentSimState& sim, int waveNumber, ChamberType type) {
  const float worldW = static_cast<float>(sim.roomWidthTiles) * DescentSimState::kTileSize;
  const float worldH = static_cast<float>(sim.roomHeightTiles) * DescentSimState::kTileSize;
  const int scale = std::max(1, sim.chamberIndex);
  const std::size_t countBefore = sim.enemies.size();

  if (type == ChamberType::Boss || (!sim.isEndless && sim.chamberIndex == sim.totalChambers)) {
    // Boss Chamber: Grave Warden Osric / Catacomb Overlord + Heavy Royal Vanguard
    const int brutes = 4;
    const int supports = 4;
    const int stalkers = 2;
    const int voidCasters = 4;
    const int shooters = 8;
    const int chargers = 8;
    const int swarmers = 20;

    auto spawnMinion = [&](EnemyArchetype arch, int count, bool elite = false) {
      for (int i = 0; i < count; ++i) {
        spawnEnemy(sim, arch, getRandomSpawnPos(sim, 180.0F), elite);
      }
    };

    spawnMinion(EnemyArchetype::Brute, brutes);
    spawnMinion(EnemyArchetype::Support, supports);
    spawnMinion(EnemyArchetype::Stalker, stalkers);
    spawnMinion(EnemyArchetype::VoidCaster, voidCasters);
    spawnMinion(EnemyArchetype::Shooter, shooters);
    spawnMinion(EnemyArchetype::Charger, chargers);
    spawnMinion(EnemyArchetype::Swarmer, swarmers);

    float bx = std::clamp(sim.playerPos.x, 140.0F, worldW - 140.0F);
    float by = std::clamp(sim.playerPos.y - 220.0F, 100.0F, worldH - 100.0F);
    spawnBossOsric(sim, Vec2{bx, by});
  } else if (type == ChamberType::Elite) {
    // Elite Chamber: 4 distinct deadly waves
    auto spawnE = [&](EnemyArchetype arch, int count, bool elite = false) {
      for (int i = 0; i < count; ++i) {
        spawnEnemy(sim, arch, getRandomSpawnPos(sim, 180.0F), elite);
      }
    };

    if (waveNumber == 1) {
      spawnE(EnemyArchetype::Grunt, 14 + scale);
      spawnE(EnemyArchetype::Shooter, 6 + scale / 2);
      spawnE(EnemyArchetype::Brute, 2 + scale / 4, true);
    } else if (waveNumber == 2) {
      spawnE(EnemyArchetype::Swarmer, 16 + scale);
      spawnE(EnemyArchetype::Exploder, 6 + scale / 2);
      spawnE(EnemyArchetype::VoidCaster, 4 + scale / 3);
      spawnE(EnemyArchetype::Charger, 5 + scale / 3, true);
      spawnE(EnemyArchetype::Stalker, 1);
    } else if (waveNumber == 3) {
      spawnE(EnemyArchetype::Swarmer, 18 + scale);
      spawnE(EnemyArchetype::Brute, 3 + scale / 3);
      spawnE(EnemyArchetype::Support, 3 + scale / 4);
      spawnE(EnemyArchetype::Stalker, 1);
    } else {
      // Wave 4: Bone Captain Rattlewick + Vanguard
      spawnE(EnemyArchetype::Swarmer, 16 + scale);
      spawnE(EnemyArchetype::Charger, 6 + scale / 2);
      spawnE(EnemyArchetype::VoidCaster, 4 + scale / 3);
      spawnE(EnemyArchetype::Shooter, 6 + scale / 2);

      float bx = std::clamp(worldW * 0.5F, 80.0F, worldW - 80.0F);
      float by = std::clamp(worldH * 0.25F, 80.0F, worldH - 80.0F);
      spawnMiniBoss(sim, Vec2{bx, by});
    }
  } else {
    // Combat Stage: distributed into 3 to 5 waves depending on chamber index
    auto spawnE = [&](EnemyArchetype arch, int count, bool elite = false) {
      for (int i = 0; i < count; ++i) {
        spawnEnemy(sim, arch, getRandomSpawnPos(sim, 180.0F), elite);
      }
    };

    if (waveNumber == 1) {
      spawnE(EnemyArchetype::Swarmer, 14 + scale);
      spawnE(EnemyArchetype::Grunt, 8 + scale / 2);
      spawnE(EnemyArchetype::Splitter, 2 + scale / 3);
    } else if (waveNumber == 2) {
      spawnE(EnemyArchetype::Swarmer, 12 + scale);
      spawnE(EnemyArchetype::Shooter, 6 + scale / 2);
      spawnE(EnemyArchetype::Charger, 4 + scale / 3);
      spawnE(EnemyArchetype::Exploder, 4 + scale / 3);
      if (scale >= 3) spawnE(EnemyArchetype::VoidCaster, 2 + scale / 4);
      spawnE(EnemyArchetype::Stalker, (scale >= 8) ? 2 : 1);
    } else if (waveNumber == 3) {
      spawnE(EnemyArchetype::Swarmer, 16 + scale);
      spawnE(EnemyArchetype::Shooter, 4 + scale / 3);
      spawnE(EnemyArchetype::Grunt, 6 + scale / 3);
      if (scale >= 4) spawnE(EnemyArchetype::Support, 1 + scale / 5);
    } else if (waveNumber == 4) {
      spawnE(EnemyArchetype::Swarmer, 16 + scale);
      spawnE(EnemyArchetype::Brute, 2 + scale / 4);
      spawnE(EnemyArchetype::Charger, 5 + scale / 3);
      spawnE(EnemyArchetype::VoidCaster, 3 + scale / 3);
    } else {
      // Wave 5 (chambers 13+)
      spawnE(EnemyArchetype::Swarmer, 18 + scale);
      spawnE(EnemyArchetype::Charger, 5 + scale / 3);
      spawnE(EnemyArchetype::VoidCaster, 4 + scale / 3);
      spawnE(EnemyArchetype::Brute, 3 + scale / 4);
      spawnE(EnemyArchetype::Exploder, 5 + scale / 3);
    }
  }

  sim.currentWaveSpawned = static_cast<int>(sim.enemies.size() - countBefore);
  sim.currentWaveKilled = 0;
  sim.waveTimer = 0.0F;
}

void spawnWave(DescentSimState& sim, int waveNumber) {
  spawnChamberWave(sim, waveNumber, sim.currentChamberType);
}

} // namespace ed::descent
