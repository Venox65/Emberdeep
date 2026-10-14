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
  int poisonRounds = 0; // Poison is on the player

  struct EnemyInstance {
    int id;
    std::string name;
    std::string ability;
    std::int64_t maxHp;
    std::int64_t hp;
    std::int64_t attack;
    std::int64_t defense;
    std::int64_t speed;
    std::int64_t gold;
    int abilityCooldown;
    int frenzyRounds;
    bool guarded;
    bool defeated;
    bool isBoss;
  };

  for (int floor = 1; floor <= 5; ++floor) {
    result.log.push_back({DungeonEventType::FloorStart, "", floor, 0, 0, false});
    const bool miniBossFloor = floor == 5;
    const int enemyCount = miniBossFloor ? 1 : static_cast<int>(rng.nextRange(1, 3));
    
    std::vector<EnemyInstance> enemies;
    for (int i = 0; i < enemyCount; ++i) {
      const auto& enemyDef = miniBossFloor
                              ? zone.miniBoss
                              : zone.enemies[rng.nextRange(0, static_cast<std::uint32_t>(zone.enemies.size() - 1))];
      const double floorMultiplier = 1.0 + 0.15 * static_cast<double>(floor - 1);
      const double bossMultiplier = miniBossFloor ? 3.0 : 1.0;
      const std::int64_t eMaxHp = scaledValue(40.0 * std::pow(2.4, tier - 1) * floorMultiplier, bossMultiplier);
      const std::int64_t eAttack = scaledValue(6.0 * std::pow(2.2, tier - 1) * (1.0 + 0.10 * (floor - 1)), miniBossFloor ? 1.5 : 1.0);
      const std::int64_t eDefense = scaledValue(4.0 * std::pow(2.1, tier - 1), 1.0);
      const std::int64_t eGold = scaledValue(10.0 * std::pow(2.5, tier - 1) * (1.0 + 0.10 * (floor - 1)), miniBossFloor ? 3.0 : 1.0);
      const std::int64_t eSpeed = 8 + 2 * zone.tier;

      enemies.push_back({i, enemyDef.name, enemyDef.ability, eMaxHp, eMaxHp, eAttack, eDefense, eSpeed, eGold, 0, 0, false, false, miniBossFloor});
      result.log.push_back({DungeonEventType::EnemyAppear, enemyDef.name, eMaxHp, eMaxHp, i, miniBossFloor});
    }

    auto countAlive = [&]() {
      int count = 0;
      for (const auto& e : enemies) {
        if (!e.defeated) count++;
      }
      return count;
    };

    while (countAlive() > 0 && playerHp > 0 && result.rounds < 500) {
      ++result.rounds;
      
      auto getTarget = [&]() -> EnemyInstance* {
        for (auto& e : enemies) {
          if (!e.defeated) return &e;
        }
        return nullptr;
      };

      auto playerAttackTurn = [&]() {
        EnemyInstance* target = getTarget();
        if (!target) return;
        
        const int variance = static_cast<int>(rng.nextRange(90, 110));
        std::int64_t damage = std::max<std::int64_t>(1, player.attack * variance / 100);
        bool crit = false;
        if (rng.nextRange(0, 9999) < static_cast<std::uint32_t>(std::clamp(player.critChanceBasisPoints, 0, 10000))) {
          damage = damage * std::max(10000, player.critDamageBasisPoints) / 10000;
          crit = true;
        }
        damage = std::max<std::int64_t>(1, damage * 100 / (100 + std::max<std::int64_t>(0, target->defense)));
        
        if (target->guarded) {
          damage /= 2;
          target->guarded = false;
        }
        target->hp -= damage;
        result.log.push_back({DungeonEventType::PlayerAttack, target->name, damage, std::max<std::int64_t>(0, target->hp), target->id, crit});
        
        if (target->hp <= 0) {
          target->defeated = true;
          result.log.push_back({DungeonEventType::EnemyDefeated, target->name, target->gold, 0, target->id, false});
          result.gold += target->gold;
          result.xp += static_cast<int>(target->gold / 2);
        }
      };

      auto enemiesTurn = [&]() {
        for (auto& enemy : enemies) {
          if (enemy.defeated || playerHp <= 0) continue;
          
          if (enemy.abilityCooldown == 0 && rng.nextRange(0, 99) < 20) {
            enemy.abilityCooldown = 3;
            if (enemy.ability == "heal") {
              std::int64_t healAmt = std::max<std::int64_t>(1, enemy.maxHp / 6);
              enemy.hp = std::min(enemy.maxHp, enemy.hp + healAmt);
              result.log.push_back({DungeonEventType::EnemyAbility, "heal", healAmt, enemy.hp, enemy.id, false});
            } else if (enemy.ability == "poison") {
              poisonRounds = 3;
              result.log.push_back({DungeonEventType::EnemyAbility, "poison", 0, 0, enemy.id, false});
            } else if (enemy.ability == "guard") {
              enemy.guarded = true;
              result.log.push_back({DungeonEventType::EnemyAbility, "guard", 0, 0, enemy.id, false});
            } else if (enemy.ability == "frenzy") {
              enemy.frenzyRounds = 2;
              result.log.push_back({DungeonEventType::EnemyAbility, "frenzy", 0, 0, enemy.id, false});
            }
          } else {
            if (enemy.abilityCooldown > 0) --enemy.abilityCooldown;
            
            const std::int64_t currentEnemyAttack =
                enemy.frenzyRounds > 0 ? enemy.attack * 130 / 100 : enemy.attack;
            
            const int variance = static_cast<int>(rng.nextRange(90, 110));
            std::int64_t dmg = std::max<std::int64_t>(1, currentEnemyAttack * variance / 100);
            dmg = std::max<std::int64_t>(1, dmg * 100 / (100 + std::max<std::int64_t>(0, player.defense)));
            playerHp -= dmg;
            result.log.push_back({DungeonEventType::EnemyAttack, enemy.name, dmg, std::max<std::int64_t>(0, playerHp), enemy.id, enemy.frenzyRounds > 0});
          }
          
          if (enemy.frenzyRounds > 0) {
            --enemy.frenzyRounds;
          }
        }
      };

      // Since all enemies have same speed
      std::int64_t enemySpeed = enemies.empty() ? 0 : enemies[0].speed;

      if (player.speed >= enemySpeed) {
        playerAttackTurn();
        if (countAlive() > 0 && playerHp > 0) {
          enemiesTurn();
        }
      } else {
        enemiesTurn();
        if (playerHp > 0 && countAlive() > 0) {
          playerAttackTurn();
        }
      }

      if (poisonRounds > 0 && playerHp > 0) {
        std::int64_t pDmg = std::max<std::int64_t>(1, player.maxHp / 20);
        playerHp -= pDmg;
        --poisonRounds;
        result.log.push_back({DungeonEventType::PoisonTick, "", pDmg, std::max<std::int64_t>(0, playerHp), 0, false});
      }
    }

    if (playerHp <= 0 || result.rounds >= 500) {
      result.log.push_back({DungeonEventType::PlayerDefeated, "", 0, 0, 0, false});
      result.log.push_back({DungeonEventType::DungeonFailed, "", floor, 0, 0, false});
      return result;
    }
    
    ++result.floorsCleared;
    result.trophies += static_cast<int>(rng.nextRange(1, 2));
  }

  result.cleared = true;
  result.gold += result.gold * 3;
  result.gemFound = rng.nextRange(0, 99) < 5;
  result.arcaneDust = rng.nextRange(0, 99) < 15 ? static_cast<int>(rng.nextRange(1, 3)) : 0;
  result.log.push_back({DungeonEventType::DungeonClear, "", result.gold, result.xp, result.trophies, result.gemFound});
  return result;
}

DungeonRunResult simulateAbyssRun(const DungeonPlayerStats& player,
                                  std::uint64_t seed,
                                  int maxFloors) {
  Pcg32 rng(seed);
  DungeonRunResult result;
  result.isAbyss = true;
  std::int64_t playerHp = std::max<std::int64_t>(1, player.maxHp);
  int poisonRounds = 0;

  struct EnemyInstance {
    int id;
    std::string name;
    std::string ability;
    std::int64_t maxHp;
    std::int64_t hp;
    std::int64_t attack;
    std::int64_t defense;
    std::int64_t speed;
    std::int64_t gold;
    int abilityCooldown;
    int frenzyRounds;
    bool guarded;
    bool defeated;
    bool isBoss;
  };

  const std::vector<std::string> normalNames = {
      "Abyssal Stalker", "Void Fiend", "Shadow Revenant",
      "Dread Leech", "Nether Wraith", "Abyss Gargoyle", "Void Cultist"
  };
  const std::vector<std::string> normalAbilities = {"poison", "frenzy", "guard", "heal"};

  const int totalFloors = std::max(1, maxFloors);

  for (int floor = 1; floor <= totalFloors; ++floor) {
    result.log.push_back({DungeonEventType::FloorStart, "Abyss Floor " + std::to_string(floor), floor, 0, 0, false});
    const bool miniBossFloor = (floor % 10 == 0);
    const int enemyCount = miniBossFloor ? 1 : static_cast<int>(rng.nextRange(1, 3));

    const double floorMult = std::pow(1.08, static_cast<double>(floor - 1));

    std::vector<EnemyInstance> enemies;
    for (int i = 0; i < enemyCount; ++i) {
      std::string eName;
      std::string eAbility;
      if (miniBossFloor) {
        if (floor == 10) eName = "Abyssal Gatekeeper";
        else if (floor == 20) eName = "Void Leviathan";
        else if (floor == 30) eName = "Dread Sovereign";
        else if (floor == 40) eName = "Nether Devourer";
        else if (floor == 50) eName = "Chaos Harbinger";
        else eName = "Abyssal Titan Prime (" + std::to_string(floor) + "F)";
        eAbility = (floor % 20 == 0) ? "frenzy" : "guard";
      } else {
        eName = normalNames[rng.nextRange(0, static_cast<std::uint32_t>(normalNames.size() - 1))];
        eAbility = normalAbilities[rng.nextRange(0, static_cast<std::uint32_t>(normalAbilities.size() - 1))];
      }

      const double bossMultiplier = miniBossFloor ? 2.5 : 1.0;
      const std::int64_t eMaxHp = scaledValue(105000.0 * floorMult, bossMultiplier);
      const std::int64_t eAttack = scaledValue(7200.0 * floorMult, miniBossFloor ? 1.35 : 1.0);
      const std::int64_t eDefense = scaledValue(3200.0 * floorMult, 1.0);
      const std::int64_t eGold = scaledValue(600.0 * floorMult, miniBossFloor ? 3.0 : 1.0);
      const std::int64_t eSpeed = 24 + std::min<std::int64_t>(16, floor / 3);

      enemies.push_back({i, eName, eAbility, eMaxHp, eMaxHp, eAttack, eDefense, eSpeed, eGold, 0, 0, false, false, miniBossFloor});
      result.log.push_back({DungeonEventType::EnemyAppear, eName, eMaxHp, eMaxHp, i, miniBossFloor});
    }

    auto countAlive = [&]() {
      int count = 0;
      for (const auto& e : enemies) {
        if (!e.defeated) count++;
      }
      return count;
    };

    int floorRounds = 0;
    while (countAlive() > 0 && playerHp > 0 && floorRounds < 200) {
      ++floorRounds;
      ++result.rounds;

      auto getTarget = [&]() -> EnemyInstance* {
        for (auto& e : enemies) {
          if (!e.defeated) return &e;
        }
        return nullptr;
      };

      auto playerAttackTurn = [&]() {
        EnemyInstance* target = getTarget();
        if (!target) return;

        const int variance = static_cast<int>(rng.nextRange(90, 110));
        std::int64_t rawDamage = std::max<std::int64_t>(1, player.attack * variance / 100);
        bool crit = false;
        if (rng.nextRange(0, 9999) < static_cast<std::uint32_t>(std::clamp(player.critChanceBasisPoints, 0, 10000))) {
          rawDamage = rawDamage * std::max(10000, player.critDamageBasisPoints) / 10000;
          crit = true;
        }
        std::int64_t damage = std::max<std::int64_t>(1, (rawDamage * rawDamage) / (rawDamage + std::max<std::int64_t>(0, target->defense)));

        if (target->guarded) {
          damage /= 2;
          target->guarded = false;
        }
        target->hp -= damage;
        result.log.push_back({DungeonEventType::PlayerAttack, target->name, damage, std::max<std::int64_t>(0, target->hp), target->id, crit});

        if (target->hp <= 0) {
          target->defeated = true;
          result.log.push_back({DungeonEventType::EnemyDefeated, target->name, target->gold, 0, target->id, false});
          result.gold += target->gold;
          result.xp += static_cast<int>(target->gold / 2);
        }
      };

      auto enemiesTurn = [&]() {
        for (auto& enemy : enemies) {
          if (enemy.defeated || playerHp <= 0) continue;

          if (enemy.abilityCooldown == 0 && rng.nextRange(0, 99) < 25) {
            enemy.abilityCooldown = 3;
            if (enemy.ability == "heal") {
              const std::int64_t healAmt = enemy.maxHp / 5;
              enemy.hp = std::min(enemy.maxHp, enemy.hp + healAmt);
              result.log.push_back({DungeonEventType::EnemyAbility, enemy.name, healAmt, enemy.hp, enemy.id, false});
            } else if (enemy.ability == "frenzy") {
              enemy.frenzyRounds = 2;
              result.log.push_back({DungeonEventType::EnemyAbility, enemy.name, 0, 0, enemy.id, false});
            } else if (enemy.ability == "poison") {
              poisonRounds = 3;
              result.log.push_back({DungeonEventType::EnemyAbility, enemy.name, 0, 0, enemy.id, false});
            } else if (enemy.ability == "guard") {
              enemy.guarded = true;
              result.log.push_back({DungeonEventType::EnemyAbility, enemy.name, 0, 0, enemy.id, false});
            }
          } else {
            if (enemy.abilityCooldown > 0) --enemy.abilityCooldown;
            const int eVariance = static_cast<int>(rng.nextRange(90, 110));
            std::int64_t atk = enemy.attack * eVariance / 100;
            if (enemy.frenzyRounds > 0) {
              atk = atk * 150 / 100;
              --enemy.frenzyRounds;
            }
            const std::int64_t eDmg = std::max<std::int64_t>(1, (atk * atk) / (atk + std::max<std::int64_t>(0, player.defense)));
            playerHp = std::max<std::int64_t>(0, playerHp - eDmg);
            result.log.push_back({DungeonEventType::EnemyAttack, enemy.name, eDmg, playerHp, enemy.id, false});
          }
        }
      };

      const std::int64_t enemySpeed = enemies.empty() ? 0 : enemies[0].speed;
      if (player.speed >= enemySpeed) {
        playerAttackTurn();
        if (countAlive() > 0 && playerHp > 0) {
          enemiesTurn();
        }
      } else {
        enemiesTurn();
        if (playerHp > 0 && countAlive() > 0) {
          playerAttackTurn();
        }
      }

      if (poisonRounds > 0 && playerHp > 0) {
        const std::int64_t pDmg = std::max<std::int64_t>(1, player.maxHp / 25);
        playerHp = std::max<std::int64_t>(0, playerHp - pDmg);
        --poisonRounds;
        result.log.push_back({DungeonEventType::PoisonTick, "", pDmg, playerHp, 0, false});
      }
    }

    if (playerHp <= 0 || floorRounds >= 200) {
      result.log.push_back({DungeonEventType::PlayerDefeated, "", 0, 0, 0, false});
      result.log.push_back({DungeonEventType::DungeonFailed, "", floor, 0, 0, false});
      return result;
    }

    ++result.floorsCleared;
    result.trophies += static_cast<int>(rng.nextRange(1, 2));

    // Loot per floor
    if (rng.nextRange(0, 99) < 25) {
      result.arcaneDust += static_cast<int>(rng.nextRange(1, 3));
    }
    if (rng.nextRange(0, 99) < 15) {
      result.aetherShards += 1;
    }
    if (rng.nextRange(0, 99) < 10) {
      result.echoSigils += 1;
    }

    // Mini-boss milestone floor rewards
    if (miniBossFloor) {
      result.echoSigils += 1 + (floor / 20);
      result.aetherShards += 2 + (floor / 20);
      result.arcaneDust += 5 + (floor / 10);
      result.trophies += 3;
      if (rng.nextRange(0, 99) < 25) {
        result.gemFound = true;
      }
      // 25% max HP recovery breather
      playerHp = std::min(player.maxHp, playerHp + player.maxHp / 4);
    }
  }

  result.cleared = true;
  result.gold += result.gold / 2;
  result.log.push_back({DungeonEventType::DungeonClear, "", result.gold, result.xp, result.trophies, result.gemFound});
  return result;
}

} // namespace ed

