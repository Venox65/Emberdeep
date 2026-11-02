#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace ed::descent {

struct Vec2 {
  float x = 0.0F;
  float y = 0.0F;

  constexpr Vec2() = default;
  constexpr Vec2(float px, float py) : x(px), y(py) {}

  Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
  Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
  Vec2 operator*(float s) const { return {x * s, y * s}; }
  Vec2 operator/(float s) const { return s != 0.0F ? Vec2{x / s, y / s} : Vec2{0.0F, 0.0F}; }

  Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
  Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
  Vec2& operator*=(float s) { x *= s; y *= s; return *this; }

  float lengthSq() const { return x * x + y * y; }
  float length() const { return std::sqrt(lengthSq()); }

  Vec2 normalized() const {
    const float len = length();
    return len > 0.0001F ? (*this / len) : Vec2{0.0F, 0.0F};
  }

  static float distance(const Vec2& a, const Vec2& b) {
    return (a - b).length();
  }

  static float dot(const Vec2& a, const Vec2& b) {
    return a.x * b.x + a.y * b.y;
  }
};

enum class ClassId {
  Warrior,   // Ashblade [Melee] [Area] [Crit]
  Tank,      // Bastion Knight [Defense] [Melee] [Area]
  Archer,    // Moonfeather Ranger [Projectile] [Crit]
  Mage,      // Runeweaver [Projectile] [Area]
  Summoner   // Soulshepherd [Summon] [Area]
};

enum class EnemyArchetype {
  Swarmer,   // HP 8, contact 4, speed 90
  Grunt,     // HP 30, contact 8, speed 70
  Shooter,   // HP 22, projectile 7, speed 60
  Charger,   // HP 45, charge 14, speed 60 -> 260
  Brute,     // HP 160, slam 18, armor 20, speed 50
  Splitter,  // HP 35, splits on death
  Exploder,  // HP 18, AoE 20 on fuse
  Support,   // HP 40, heals/shields
  Spawner,   // HP 70, spawns swarmers
  Stalker,   // HP 28, contact 11, speed 115, shadow blink dasher
  VoidCaster // HP 38, contact 8, speed 55, homing void orb caster
};

enum class PickupType {
  XpGemSmall,
  XpGemMedium,
  XpGemLarge,
  Ember,
  Roast,
  EmberStone,
  Chest,
  SoulFragment,
  Brew
};

enum class StatusEffectType {
  Burn,      // DoT, stacks to 3
  Chill,     // -30% move speed
  Freeze,    // stopped 1.5s, +50% dmg taken
  Hex,       // +12% damage taken
  Poison,    // DoT, ignores armor
  Stun,      // stopped 0.5s
  Shock,     // chain on hit
  Bleed      // damage when moving
};

struct StatusEffect {
  StatusEffectType type;
  float duration = 0.0F;
  float tickTimer = 0.0F;
  float dps = 0.0F;
  int stacks = 1;
};

enum class ChamberType {
  Combat,
  Elite,
  Treasure,
  Shrine,
  Shop,
  Boss
};

struct ChamberChoice {
  ChamberType type = ChamberType::Combat;
  std::string name;
  std::string iconType = "sword"; // "sword", "heal", "levelup", "skull", "boss"
  std::string description;
  std::string rewardPreview;
  std::string rewardType; // "none", "heal", "levelup", "elite", etc.
  Vec2 doorPos{0.0F, 0.0F};
  float doorWidth = 76.0F;
  float doorHeight = 44.0F;
};

struct DescentPlayerStats {
  float maxHp = 120.0F;
  float currentHp = 120.0F;
  float barrier = 0.0F;
  float maxBarrier = 24.0F;
  float armor = 3.0F;
  float damageReduction = 0.0F;      // cap 0.60 (60%)
  float moveSpeed = 190.0F;          // px/s
  float might = 0.0F;                // +% damage dealt
  float attackSpeed = 0.0F;          // +% attack rate
  float area = 0.0F;                 // +% attack size
  float projectileSpeed = 0.0F;      // +% speed
  float duration = 0.0F;             // +% effect duration
  int amount = 0;                    // +N projectiles/minions
  float cooldownReduction = 0.0F;    // cap 0.50 (50%)
  float critChance = 0.05F;          // base 5%
  float critDamage = 0.50F;          // base +50%
  int luck = 0;
  float pickupRadius = 72.0F;        // px
  float growth = 0.0F;               // +% XP
  float greed = 0.0F;                // +% Embers
  float regen = 0.0F;                // HP/s
  int revives = 0;
  int rerolls = 1;
  int banishes = 1;
  int skips = 1;

  // Ability State
  int dashCharges = 2;
  int maxDashCharges = 2;
  float dashRechargeTimer = 0.0F;
  float dashDurationTimer = 0.0F;
  Vec2 dashDirection{0.0F, 0.0F};
  float invulnerabilityTimer = 0.0F;

  float primaryCooldown = 0.0F;
  float skillCooldown = 0.0F;
  float maxSkillCooldown = 8.0F;

  float wrath = 0.0F;                // 0 to 100
  float ultimateActiveTimer = 0.0F;

  // Class-specific kit state
  int arrowCritCount = 0;            // Archer Backroll buff (next N shots guaranteed crit)
  float whirlTimer = 0.0F;           // Warrior Whirling Blades active timer
  float bulwarkTimer = 0.0F;         // Tank Bulwark shield active timer
  float rainOfArrowsTimer = 0.0F;    // Archer Rain of Arrows duration
  Vec2 rainTarget{0.0F, 0.0F};       // Archer Rain target center
  float fortressTimer = 0.0F;        // Tank Fortress ultimate timer
  float fortressTickTimer = 0.0F;
  float frostNovaTimer = 0.0F;       // Mage Frost Nova
  float meteorSwarmTimer = 0.0F;     // Mage Meteor Swarm
  float soulHarvestTimer = 0.0F;     // Summoner Soul Harvest
};

struct DescentEnemy {
  int id = 0;
  EnemyArchetype archetype = EnemyArchetype::Grunt;
  std::string name;
  Vec2 pos{0.0F, 0.0F};
  Vec2 vel{0.0F, 0.0F};
  float radius = 14.0F;
  float hp = 30.0F;
  float maxHp = 30.0F;
  float armor = 0.0F;
  float speed = 70.0F;
  float contactDamage = 8.0F;
  float attackCooldown = 0.0F;
  float telegraphTimer = 0.0F;
  float flashTimer = 0.0F;
  bool alive = true;
  bool isElite = false;
  std::vector<std::string> affixes;
  std::vector<StatusEffect> statuses;
  float xpReward = 5.0F;
  float emberChance = 0.2F;

  // Archetype & Boss specific behaviors
  bool isCharging = false;
  float chargeTimer = 0.0F;
  Vec2 chargeDir{0.0F, 0.0F};

  bool isSlamming = false;
  float slamTimer = 0.0F;

  bool isFusing = false;
  float fuseTimer = 0.0F;

  float shootCooldown = 0.0F;
  float spawnTimer = 0.0F;
  float supportTimer = 0.0F;

  // Boss / Mini-boss data
  bool isBoss = false;
  bool isMiniBoss = false;
  int bossPhase = 1;
  float bossSpecialCooldown = 0.0F;
  bool isLeaping = false;
  float leapTimer = 0.0F;
  Vec2 leapTarget{0.0F, 0.0F};
};

struct DescentProjectile {
  int id = 0;
  Vec2 pos{0.0F, 0.0F};
  Vec2 vel{0.0F, 0.0F};
  float radius = 6.0F;
  float damage = 10.0F;
  bool isCrit = false;
  int pierceRemaining = 0;
  float lifetime = 2.0F;
  bool fromPlayer = true;
  bool alive = true;
  std::string sourceArm;
};

struct DescentPickup {
  int id = 0;
  PickupType type = PickupType::XpGemSmall;
  Vec2 pos{0.0F, 0.0F};
  float value = 1.0F;
  bool alive = true;
  bool magnetized = false;
};

struct DescentDamageEvent {
  Vec2 pos{0.0F, 0.0F};
  float damage = 0.0F;
  bool isCrit = false;
  bool toPlayer = false;
  float lifetime = 0.6F;
};

struct ThornPatch {
  Vec2 pos{0.0F, 0.0F};
  float radius = 36.0F;
  float lifetime = 4.0F;
  float tickTimer = 0.0F;
  float damage = 5.0F;
};

struct FriendlyMinion {
  int id = 0;
  Vec2 pos{0.0F, 0.0F};
  float radius = 12.0F;
  float hp = 40.0F;
  float maxHp = 40.0F;
  float speed = 130.0F;
  float lifetime = 8.0F;
  float attackCooldown = 0.0F;
  float damage = 7.0F;
  bool alive = true;
};

struct TelegraphShape {
  enum class ShapeType { Circle, Line, Arc };
  ShapeType type = ShapeType::Circle;
  Vec2 pos{0.0F, 0.0F};
  Vec2 target{0.0F, 0.0F};
  float radius = 40.0F;
  float angle = 0.0F;
  float spread = 0.0F;
  float timer = 0.6F;
  float maxTimer = 0.6F;
};

struct DescentSave {
  int embers = 0;
  int soulFragments = 0;
  int crownShards = 0;

  // Anvil Ranks (0 to max)
  int anvilMight = 0;        // +5% damage per rank (max 10)
  int anvilVitality = 0;     // +10 HP per rank (max 10)
  int anvilSwiftness = 0;    // +4% speed per rank (max 5)
  int anvilMagnetism = 0;    // +15% pickup radius per rank (max 5)
  int anvilResilience = 0;   // +1 Armor per rank (max 5)
  int anvilGreed = 0;        // +10% Embers per rank (max 5)
  int anvilRerolls = 0;      // +1 reroll per rank (max 3)
  int anvilBanishes = 0;     // +1 banish per rank (max 3)

  // Class Selection
  ClassId selectedClass = ClassId::Warrior;

  // Pacts (0 to max rank)
  int pactClaws = 0;         // +15% enemy dmg per rank (Heat +1)
  int pactFrenzy = 0;        // +10% enemy speed per rank (Heat +1)
  int pactFlesh = 0;         // +20% enemy hp per rank (Heat +1)
  int pactShadows = 0;       // +25% elite spawn rate (Heat +2)
  int pactNumbers = 0;       // +25% horde size (Heat +2)
  int pactWounds = 0;        // -30% healing (Heat +1)

  // Records & Statistics
  int highestBiome = 1;
  int bestHeat = 0;
  int victories = 0;
  int lifetimeKills = 0;
  int lifetimeRoomsCleared = 0;
  int bestEndlessChamber = 0;

  // Selected Sigil Gems (borrowed from main inventory)
  std::vector<std::string> sigilGems;

  // Selected Provisions
  bool provisionStew = false; // +20% HP
  bool provisionBrew = false; // +15% Move Speed
  bool provisionOil = false;  // +15% Might

  int calculateTotalHeat() const {
    return pactClaws * 1 + pactFrenzy * 1 + pactFlesh * 1 +
           pactShadows * 2 + pactNumbers * 2 + pactWounds * 1;
  }
};

struct DescentInput {
  Vec2 moveVector{0.0F, 0.0F};       // normalized -1.0 to 1.0
  Vec2 aimPosition{0.0F, 0.0F};      // cursor pos in world px
  bool dashPressed = false;
  bool skillPressed = false;
  bool ultimatePressed = false;
  bool interactPressed = false;
  bool autoAim = false;
};

// Pacing & Progression formulas
inline int runXpNeed(int level) {
  if (level <= 1) return 95;
  const double l = static_cast<double>(level);
  return static_cast<int>(std::llround(70.0 + 25.0 * l + 4.5 * l * l));
}

inline float calculateDamageDealt(float baseDamage, float might, bool isCrit, float critDamage, float enemyArmor) {
  const float hit = baseDamage * (1.0F + might) * (isCrit ? (1.0F + critDamage) : 1.0F);
  const float finalDmg = (hit * 100.0F) / (100.0F + std::max(0.0F, enemyArmor));
  return std::max(1.0F, finalDmg);
}

inline float calculateDamageTaken(float incoming, float damageReduction, float armor) {
  const float dr = std::clamp(damageReduction, 0.0F, 0.60F); // cap 60%
  const float minTaken = std::ceil(incoming * 0.20F);
  const float reduced = incoming * (1.0F - dr) - armor;
  return std::max(minTaken, reduced);
}

} // namespace ed::descent
