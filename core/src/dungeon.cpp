#include "ed/dungeon.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace ed {

namespace {

std::int64_t scaledValue(double base, double multiplier) {
  return std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(base * multiplier)));
}

std::int64_t attackDamage(std::int64_t attack,
                          std::int64_t defense,
                          Pcg32& rng,
                          int critChanceBasisPoints,
                          int critDamageBasisPoints) {
  const int variance = static_cast<int>(rng.nextRange(90, 110));
  std::int64_t damage = std::max<std::int64_t>(1, attack * variance / 100);
  if (rng.nextRange(0, 9999) < static_cast<std::uint32_t>(std::clamp(critChanceBasisPoints, 0, 10000))) {
    damage = damage * std::max(10000, critDamageBasisPoints) / 10000;
  }
  return std::max<std::int64_t>(1, damage * 100 / (100 + std::max<std::int64_t>(0, defense)));
}

} // namespace

std::vector<DungeonZoneDef> defaultDungeonZones() {
  return {
      {"ashen-warrens", "Ashen Warrens", 1,
       {{"cave-rat", "Cave Rat", "poison"}, {"goblin-grub", "Goblin Grub", "frenzy"},
        {"dust-imp", "Dust Imp", "guard"}},
       {"gnome-sergeant", "Gnome Sergeant", "heal"}},
      {"copper-crypt", "Copper Crypt", 2,
       {{"skeleton-thrall", "Skeleton Thrall", "guard"}, {"crypt-bat", "Crypt Bat", "poison"},
        {"green-slime", "Green Slime", "heal"}},
       {"bone-captain", "Bone Captain", "frenzy"}},
      {"iron-hollows", "Iron Hollows", 3,
       {{"cinder-cultist", "Cinder Cultist", "frenzy"}, {"fire-wisp", "Fire Wisp", "poison"},
        {"iron-golem-lite", "Iron Golem-lite", "guard"}},
       {"ashen-priestess", "Ashen Priestess", "heal"}},
      {"silvermire", "Silvermire", 4,
       {{"bog-toad", "Bog Toad", "heal"}, {"mire-wraith", "Mire Wraith", "poison"},
        {"leech-swarm", "Leech Swarm", "frenzy"}},
       {"mire-matron", "Mire Matron", "heal"}},
      {"aurelite-keep", "Aurelite Keep", 5,
       {{"gilded-guard", "Gilded Guard", "guard"}, {"cursed-hound", "Cursed Hound", "frenzy"},
        {"coin-mimic", "Coin Mimic", "heal"}},
       {"steward-of-gold", "Steward of Gold", "guard"}},
      {"runic-archives", "Runic Archives", 6,
       {{"ink-wisp", "Ink Wisp", "poison"}, {"tome-mimic", "Tome Mimic", "heal"},
        {"rune-sentinel", "Rune Sentinel", "guard"}},
       {"archivist-shade", "Archivist Shade", "frenzy"}},
      {"stargazer-spire", "Stargazer Spire", 7,
       {{"comet-sprite", "Comet Sprite", "frenzy"}, {"moonlit-gargoyle", "Moonlit Gargoyle", "guard"},
        {"star-weaver", "Star Weaver", "poison"}},
       {"astral-watcher", "Astral Watcher", "heal"}},
      {"wyrmbone-caverns", "Wyrmbone Caverns", 8,
       {{"drake-whelp", "Drake Whelp", "frenzy"}, {"ember-salamander", "Ember Salamander", "poison"},
        {"bone-wyrmling", "Bone Wyrmling", "guard"}},
       {"wyrm-warden", "Wyrm Warden", "frenzy"}},
      {"voidglass-sanctum", "Voidglass Sanctum", 9,
       {{"shadow-acolyte", "Shadow Acolyte", "poison"}, {"void-leech", "Void Leech", "heal"},
        {"hollow-knight", "Hollow Knight", "guard"}},
       {"void-reaver", "Void Reaver", "frenzy"}},
      {"aether-vault", "The Aether Vault", 10,
       {{"aether-phantom", "Aether Phantom", "poison"}, {"crown-shard", "Crown Shard", "guard"},
        {"vault-warden", "Vault Warden", "heal"}},
       {"echo-of-the-king", "Echo of the King", "frenzy"}},
  };
}

DungeonRunResult simulateDungeonRun(const DungeonPlayerStats& player,
                                    const DungeonZoneDef& zone,
                                    std::uint64_t seed) {
  if (zone.enemies.empty()) {
    throw std::invalid_argument("dungeon zone has no enemies");
  }

  Pcg32 rng(seed);
  DungeonRunResult result;
  std::int64_t playerHp = std::max<std::int64_t>(1, player.maxHp);
  const double tier = static_cast<double>(zone.tier);

  for (int floor = 1; floor <= 5; ++floor) {
    const bool miniBossFloor = floor == 5;
    const int enemyCount = miniBossFloor ? 1 : static_cast<int>(rng.nextRange(1, 3));
    for (int enemyIndex = 0; enemyIndex < enemyCount; ++enemyIndex) {
      const auto& enemy = miniBossFloor
                              ? zone.miniBoss
                              : zone.enemies[rng.nextRange(0, static_cast<std::uint32_t>(zone.enemies.size() - 1))];
      const double floorMultiplier = 1.0 + 0.15 * static_cast<double>(floor - 1);
      const double bossMultiplier = miniBossFloor ? 3.0 : 1.0;
      std::int64_t enemyHp = scaledValue(40.0 * std::pow(2.4, tier - 1) * floorMultiplier, bossMultiplier);
      const std::int64_t enemyAttack =
          scaledValue(6.0 * std::pow(2.2, tier - 1) * (1.0 + 0.10 * (floor - 1)),
                      miniBossFloor ? 1.5 : 1.0);
      const std::int64_t enemyDefense = scaledValue(4.0 * std::pow(2.1, tier - 1), 1.0);
      const std::int64_t enemyGold =
          scaledValue(10.0 * std::pow(2.5, tier - 1) * (1.0 + 0.10 * (floor - 1)),
                      miniBossFloor ? 3.0 : 1.0);
      const std::int64_t enemySpeed = 8 + 2 * zone.tier;
      int abilityCooldown = 0;
      int poisonRounds = 0;
      int frenzyRounds = 0;
      bool guarded = false;

      while (enemyHp > 0 && playerHp > 0 && result.rounds < 500) {
        ++result.rounds;
        if (player.speed >= enemySpeed) {
          auto damage = attackDamage(player.attack, enemyDefense, rng, player.critChanceBasisPoints,
                                     player.critDamageBasisPoints);
          if (guarded) {
            damage /= 2;
            guarded = false;
          }
          enemyHp -= std::max<std::int64_t>(1, damage);
        }
        if (enemyHp <= 0) {
          break;
        }

        if (abilityCooldown == 0 && rng.nextRange(0, 99) < 20) {
          abilityCooldown = 3;
          if (enemy.ability == "heal") {
            enemyHp += std::max<std::int64_t>(1, enemyHp / 6);
          } else if (enemy.ability == "poison") {
            poisonRounds = 3;
          } else if (enemy.ability == "guard") {
            guarded = true;
          } else if (enemy.ability == "frenzy") {
            frenzyRounds = 2;
          }
        } else if (abilityCooldown > 0) {
          --abilityCooldown;
        }

        if (poisonRounds > 0) {
          playerHp -= std::max<std::int64_t>(1, player.maxHp / 20);
          --poisonRounds;
        }
        const std::int64_t currentEnemyAttack =
            frenzyRounds > 0 ? enemyAttack * 130 / 100 : enemyAttack;
        const std::int64_t incoming =
            attackDamage(currentEnemyAttack, player.defense, rng, 0, 10000);
        playerHp -= incoming;
        if (frenzyRounds > 0) {
          --frenzyRounds;
        }
        if (playerHp <= 0) {
          break;
        }
        if (player.speed < enemySpeed) {
          auto damage = attackDamage(player.attack, enemyDefense, rng, player.critChanceBasisPoints,
                                     player.critDamageBasisPoints);
          if (guarded) {
            damage /= 2;
            guarded = false;
          }
          enemyHp -= std::max<std::int64_t>(1, damage);
        }
      }

      if (playerHp <= 0 || result.rounds >= 500) {
        return result;
      }
      result.gold += enemyGold;
      result.xp += static_cast<int>(enemyGold / 2);
    }
    ++result.floorsCleared;
    result.trophies += static_cast<int>(rng.nextRange(1, 2));
  }

  result.cleared = true;
  result.gold += result.gold * 3;
  result.gemFound = rng.nextRange(0, 99) < 5;
  result.arcaneDust = rng.nextRange(0, 99) < 15 ? static_cast<int>(rng.nextRange(1, 3)) : 0;
  return result;
}

} // namespace ed
