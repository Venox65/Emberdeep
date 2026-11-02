#pragma once

#include "ed/descent/types.hpp"
#include <vector>
#include <string>
#include <memory>

namespace ed::descent {

struct LevelUpCard {
  std::string id;
  std::string name;
  std::string description;
  std::string type; // "Arm", "Tome", "Heal", "Embers"
  int currentLevel = 0;
  int maxLevel = 5;
};

struct FrostRingVisual {
  Vec2 pos{0.0F, 0.0F};
  float radius = 0.0F;
  float maxRadius = 130.0F;
  float timer = 0.40F;
  float maxTimer = 0.40F;
};

struct LightningArcVisual {
  Vec2 from{0.0F, 0.0F};
  Vec2 to{0.0F, 0.0F};
  float timer = 0.20F;
  float maxTimer = 0.20F;
};

struct FlowField {
  int width = 0;
  int height = 0;
  std::vector<uint8_t> walkable;
  std::vector<Vec2> directions;
  int playerTileX = -1;
  int playerTileY = -1;
  float updateTimer = 0.0F;

  void resize(int w, int h);
  void compute(int targetTileX, int targetTileY);
  Vec2 getDirection(float worldX, float worldY) const;
};

struct SpatialHash {
  static constexpr float kCellSize = 64.0F;
  int cols = 0;
  int rows = 0;
  std::vector<std::vector<int>> cells;

  void init(float worldWidth, float worldHeight);
  void clear();
  void insert(int entityIndex, Vec2 pos);
  void query(Vec2 pos, float radius, std::vector<int>& outIndices) const;
};

struct DescentSimState {
  ClassId selectedClass = ClassId::Warrior;
  Vec2 playerPos{896.0F, 512.0F};
  Vec2 playerVel{0.0F, 0.0F};
  Vec2 playerAim{1.0F, 0.0F};
  DescentPlayerStats player;

  int roomWidthTiles = 56;
  int roomHeightTiles = 32;
  static constexpr float kTileSize = 32.0F;

  FlowField flowField;
  SpatialHash spatialHash;

  std::vector<DescentEnemy> enemies;
  std::vector<DescentProjectile> projectiles;
  std::vector<DescentPickup> pickups;
  std::vector<DescentDamageEvent> damageEvents;
  std::vector<ThornPatch> thornPatches;
  std::vector<FriendlyMinion> friendlyMinions;
  std::vector<TelegraphShape> telegraphs;
  std::vector<FrostRingVisual> frostRings;
  std::vector<LightningArcVisual> lightningArcs;

  // Owned Arms and Tomes (id -> level)
  struct ItemSlot {
    std::string id;
    int level = 1;
  };
  std::vector<ItemSlot> ownedArms;
  std::vector<ItemSlot> ownedTomes;

  // Run stats
  int runLevel = 1;
  int currentXp = 0;
  int xpToNext = 95;
  int embers = 0;
  int soulFragments = 0;
  int crownShards = 0;

  // Level-up modal state
  bool levelUpPending = false;
  std::vector<LevelUpCard> levelUpChoices;
  std::vector<std::string> banishedIds;

  // Roguelike Multi-Chamber Run Progression
  int chamberIndex = 1;
  int totalChambers = 20;
  bool isEndless = false;
  int endlessLoop = 1;
  ChamberType currentChamberType = ChamberType::Combat;
  bool doorChoicesPending = false;
  std::vector<ChamberChoice> doorChoices;
  bool runCompleted = false;

  // Room wave state
  int currentWave = 1;
  int totalWaves = 2;
  int currentWaveSpawned = 0;
  int currentWaveKilled = 0;
  bool roomCleared = false;
  float waveTimer = 0.0F;
  float nextEntityId = 1;

  // Arm timers
  float censerTimer = 0.0F;
  float axeTimer = 0.0F;
  float daggersTimer = 0.0F;
  float lightningTimer = 0.0F;
  float frostBellTimer = 0.0F;
  float brambleTimer = 0.0F;
  float lanternTimer = 0.0F;
  float lanternAngle = 0.0F;
  float runebladesAngle = 0.0F;
  float gravityWellTimer = 0.0F;
  float skeletalHordeTimer = 0.0F;
  float soulSiphonTimer = 0.0F;
  float arcaneOrbTimer = 0.0F;
  float seekingGaleTimer = 0.0F;
  float earthTremorTimer = 0.0F;

  // Boss / Mini-boss active tracking
  int bossEntityId = -1;
  bool bossDefeated = false;
  std::string bossName;
  std::string bossTitle;
  float bossMaxHp = 1.0F;
  float bossCurrentHp = 0.0F;
  int bossPhase = 1;

  // Heat & Pact Multipliers
  int heatLevel = 0;
  float heatDamageMult = 1.0F;
  float heatSpeedMult = 1.0F;
  float heatHpMult = 1.0F;
  float eliteChance = 0.15F;

  // Visual/juice effects
  float hitStopTimer = 0.0F;
  float screenShake = 0.0F;
  Vec2 lastSlashArcCenter{0.0F, 0.0F};
  float lastSlashArcAngle = 0.0F;
  float lastSlashArcTimer = 0.0F;

  // Summary counters
  int enemiesKilled = 0;
  float totalDamageDealt = 0.0F;
  float totalDamageTaken = 0.0F;
  float runTimeSeconds = 0.0F;
};

void initRoom(DescentSimState& sim, int widthTiles, int heightTiles);
void applyMetaToPlayer(DescentSimState& sim, const DescentSave& meta);
void spawnEnemy(DescentSimState& sim, EnemyArchetype type, Vec2 pos, bool isElite = false);
void spawnMiniBoss(DescentSimState& sim, Vec2 pos);
void spawnBossOsric(DescentSimState& sim, Vec2 pos);
void spawnWave(DescentSimState& sim, int waveNumber);
void spawnChamberWave(DescentSimState& sim, int waveNumber, ChamberType type);
void initNextChamber(DescentSimState& sim, ChamberType type);
void generateChamberChoices(DescentSimState& sim);
bool selectChamberChoice(DescentSimState& sim, int index);
void stepDescentSim(DescentSimState& sim, const DescentInput& input, float dt);

void triggerClassSkill(DescentSimState& sim);
void triggerClassUltimate(DescentSimState& sim);

void generateLevelUpChoices(DescentSimState& sim);
bool selectLevelUpChoice(DescentSimState& sim, int index);
bool rerollLevelUpChoices(DescentSimState& sim);
bool banishLevelUpChoice(DescentSimState& sim, int index);
bool skipLevelUpChoice(DescentSimState& sim);

} // namespace ed::descent
