#include "ed/descent/combat_sim.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

namespace ed::descent {

void triggerClassSkill(DescentSimState& sim) {
  if (sim.player.skillCooldown > 0.0F) return;
  sim.player.skillCooldown = sim.player.maxSkillCooldown * (1.0F - std::min(0.50F, sim.player.cooldownReduction));

  switch (sim.selectedClass) {
    case ClassId::Warrior:
      // Whirling Blades: 3.0s duration
      sim.player.whirlTimer = 3.0F;
      break;

    case ClassId::Tank:
      // Bulwark: fills barrier to max, absorbs and reflects damage for 4s
      sim.player.barrier = sim.player.maxBarrier;
      sim.player.bulwarkTimer = 4.0F;
      break;

    case ClassId::Archer: {
      // Volley: fires 5 piercing arrows in a 40-deg fan
      const float baseAngle = std::atan2(sim.playerAim.y, sim.playerAim.x);
      for (int i = -2; i <= 2; ++i) {
        const float angle = baseAngle + static_cast<float>(i) * 0.15F;
        DescentProjectile p;
        p.id = static_cast<int>(sim.nextEntityId++);
        p.pos = sim.playerPos;
        p.vel = Vec2{std::cos(angle), std::sin(angle)} * 420.0F * (1.0F + sim.player.projectileSpeed);
        p.damage = 18.0F;
        p.pierceRemaining = 2;
        p.lifetime = 1.6F;
        p.fromPlayer = true;
        p.sourceArm = "archer_volley";
        sim.projectiles.push_back(p);
      }
      break;
    }

    case ClassId::Mage:
      // Frost Nova: freezes all enemies within 130px for 2.0s and deals 25 damage
      sim.player.frostNovaTimer = 0.25F;
      sim.frostRings.push_back({sim.playerPos, 20.0F, 130.0F * (1.0F + sim.player.area), 0.45F, 0.45F});
      for (auto& e : sim.enemies) {
        if (!e.alive) continue;
        if (Vec2::distance(e.pos, sim.playerPos) <= 130.0F * (1.0F + sim.player.area) + e.radius) {
          const float dmg = calculateDamageDealt(25.0F, sim.player.might, false, sim.player.critDamage, e.armor);
          e.hp -= dmg;
          e.flashTimer = 0.1F;
          sim.totalDamageDealt += dmg;
          sim.damageEvents.push_back({e.pos, dmg, false, false, 0.6F});

          StatusEffect freeze;
          freeze.type = StatusEffectType::Freeze;
          freeze.duration = 2.0F * (1.0F + sim.player.duration);
          freeze.stacks = 1;
          e.statuses.push_back(freeze);
        }
      }
      break;

    case ClassId::Summoner:
      // Raise Minions: summons 3 ghost wolves
      for (int i = 0; i < 3 + sim.player.amount; ++i) {
        FriendlyMinion m;
        m.id = static_cast<int>(sim.nextEntityId++);
        m.pos = sim.playerPos + Vec2{static_cast<float>((i - 1) * 20), 20.0F};
        m.damage = 9.0F * (1.0F + sim.player.might);
        m.lifetime = 8.0F * (1.0F + sim.player.duration);
        sim.friendlyMinions.push_back(m);
      }
      break;
  }
}

void triggerClassUltimate(DescentSimState& sim) {
  if (sim.player.wrath < 100.0F) return;
  sim.player.wrath = 0.0F;

  switch (sim.selectedClass) {
    case ClassId::Warrior:
      // War Cry: +30% Might, +25% speed, restores 25% max HP
      sim.player.ultimateActiveTimer = 6.0F;
      sim.player.might += 0.30F;
      sim.player.moveSpeed *= 1.25F;
      sim.player.currentHp = std::min(sim.player.maxHp, sim.player.currentHp + sim.player.maxHp * 0.25F);
      break;

    case ClassId::Tank:
      // Fortress: 7s stationary zone, -50% damage taken, holy pulses
      sim.player.fortressTimer = 7.0F;
      sim.player.fortressTickTimer = 0.0F;
      break;

    case ClassId::Archer:
      // Rain of Arrows: barrage on target area
      sim.player.rainOfArrowsTimer = 4.0F;
      sim.player.rainTarget = sim.playerPos + sim.playerAim * 160.0F;
      break;

    case ClassId::Mage:
      // Meteor Swarm: 4.0s storm of meteors
      sim.player.meteorSwarmTimer = 4.0F;
      break;

    case ClassId::Summoner: {
      // Soul Harvest: damages all Hexed/nearby enemies, detonates them, heals player
      float totalHeal = 0.0F;
      for (auto& e : sim.enemies) {
        if (!e.alive) continue;
        if (Vec2::distance(e.pos, sim.playerPos) <= 220.0F) {
          const float dmg = calculateDamageDealt(50.0F, sim.player.might, true, sim.player.critDamage, e.armor);
          e.hp -= dmg;
          e.flashTimer = 0.15F;
          sim.totalDamageDealt += dmg;
          sim.damageEvents.push_back({e.pos, dmg, true, false, 0.7F});
          totalHeal += 6.0F;
        }
      }
      sim.player.currentHp = std::min(sim.player.maxHp, sim.player.currentHp + totalHeal);
      break;
    }
  }
}

void stepDescentSim(DescentSimState& sim, const DescentInput& input, float dt) {
  if (sim.levelUpPending) return;

  sim.runTimeSeconds += dt;

  // Update floating damage numbers
  for (auto& ev : sim.damageEvents) {
    ev.lifetime -= dt;
    ev.pos.y -= 25.0F * dt;
  }
  std::erase_if(sim.damageEvents, [](const auto& ev) { return ev.lifetime <= 0.0F; });

  if (sim.hitStopTimer > 0.0F) {
    sim.hitStopTimer -= dt;
    return;
  }

  if (sim.screenShake > 0.0F) {
    sim.screenShake = std::max(0.0F, sim.screenShake - dt * 2.5F);
  }

  if (sim.lastSlashArcTimer > 0.0F) {
    sim.lastSlashArcTimer -= dt;
  }

  for (auto it = sim.frostRings.begin(); it != sim.frostRings.end(); ) {
    it->timer -= dt;
    if (it->timer <= 0.0F) {
      it = sim.frostRings.erase(it);
    } else {
      const float progress = 1.0F - (it->timer / it->maxTimer);
      it->radius = 20.0F + (it->maxRadius - 20.0F) * progress;
      ++it;
    }
  }

  for (auto it = sim.lightningArcs.begin(); it != sim.lightningArcs.end(); ) {
    it->timer -= dt;
    if (it->timer <= 0.0F) {
      it = sim.lightningArcs.erase(it);
    } else {
      ++it;
    }
  }

  const float worldW = static_cast<float>(sim.roomWidthTiles) * DescentSimState::kTileSize;
  const float worldH = static_cast<float>(sim.roomHeightTiles) * DescentSimState::kTileSize;

  // Aim direction
  Vec2 aimVec = input.aimPosition - sim.playerPos;
  if (aimVec.lengthSq() > 1.0F) {
    sim.playerAim = aimVec.normalized();
  }

  // Skill & Ultimate Triggers
  if (input.skillPressed) {
    triggerClassSkill(sim);
  }
  if (input.ultimatePressed) {
    triggerClassUltimate(sim);
  }

  if (sim.player.skillCooldown > 0.0F) {
    sim.player.skillCooldown -= dt;
  }

  // Ultimate timer expirations (e.g. War Cry expiration)
  if (sim.player.ultimateActiveTimer > 0.0F) {
    sim.player.ultimateActiveTimer -= dt;
    if (sim.player.ultimateActiveTimer <= 0.0F && sim.selectedClass == ClassId::Warrior) {
      sim.player.might -= 0.30F;
      sim.player.moveSpeed /= 1.25F;
    }
  }

  // Dash handling
  if (input.dashPressed && sim.player.dashCharges > 0 && sim.player.dashDurationTimer <= 0.0F) {
    sim.player.dashCharges--;
    sim.player.dashDurationTimer = 0.18F;
    Vec2 dir = input.moveVector;
    if (dir.lengthSq() > 0.01F) {
      sim.player.dashDirection = dir.normalized();
    } else {
      sim.player.dashDirection = sim.playerAim;
    }

    // Archer Backroll: dashes backwards from aim, next 3 shots guaranteed crit
    if (sim.selectedClass == ClassId::Archer) {
      sim.player.dashDirection = sim.playerAim * -1.0F;
      sim.player.arrowCritCount = 3;
    }
    // Mage Blink: instant teleport
    else if (sim.selectedClass == ClassId::Mage) {
      sim.playerPos += sim.player.dashDirection * 150.0F;
      sim.player.dashDurationTimer = 0.05F;
    }

    sim.player.invulnerabilityTimer = 0.20F;
  }

  // Movement & Dash update
  if (sim.player.dashDurationTimer > 0.0F) {
    sim.player.dashDurationTimer -= dt;
    const float dashSpeed = 200.0F / 0.18F;
    sim.playerPos += sim.player.dashDirection * dashSpeed * dt;
  } else {
    Vec2 moveDir = input.moveVector;
    if (moveDir.lengthSq() > 0.01F) {
      moveDir = moveDir.normalized();
      sim.playerPos += moveDir * sim.player.moveSpeed * dt;
    }
  }

  // Clamp player to walkable room bounds
  sim.playerPos.x = std::clamp(sim.playerPos.x, DescentSimState::kTileSize * 1.5F, worldW - DescentSimState::kTileSize * 1.5F);
  sim.playerPos.y = std::clamp(sim.playerPos.y, DescentSimState::kTileSize * 1.5F, worldH - DescentSimState::kTileSize * 1.5F);

  // Dash recharge
  if (sim.player.dashCharges < sim.player.maxDashCharges) {
    sim.player.dashRechargeTimer += dt * (1.0F + sim.player.cooldownReduction);
    if (sim.player.dashRechargeTimer >= 3.0F) {
      sim.player.dashCharges++;
      sim.player.dashRechargeTimer = 0.0F;
    }
  }

  // Invulnerability timer
  if (sim.player.invulnerabilityTimer > 0.0F) {
    sim.player.invulnerabilityTimer -= dt;
  }

  // Passive HP regen
  if (sim.player.regen > 0.0F && sim.player.currentHp < sim.player.maxHp) {
    sim.player.currentHp = std::min(sim.player.maxHp, sim.player.currentHp + sim.player.regen * dt);
  }

  // Compute flow field periodically
  sim.flowField.updateTimer -= dt;
  const int ptx = static_cast<int>(sim.playerPos.x / DescentSimState::kTileSize);
  const int pty = static_cast<int>(sim.playerPos.y / DescentSimState::kTileSize);
  if (sim.flowField.updateTimer <= 0.0F || ptx != sim.flowField.playerTileX || pty != sim.flowField.playerTileY) {
    sim.flowField.compute(ptx, pty);
    sim.flowField.updateTimer = 0.25F;
  }

  // Primary Attacks
  sim.player.primaryCooldown -= dt * (1.0F + sim.player.attackSpeed);
  if (sim.player.primaryCooldown <= 0.0F) {
    switch (sim.selectedClass) {
      case ClassId::Warrior: {
        const float sweepRange = 70.0F * (1.0F + sim.player.area);
        const float sweepAngleSpread = 140.0F * 0.0174533F;
        Vec2 targetDir = sim.playerAim;
        bool hitAny = false;

        for (auto& enemy : sim.enemies) {
          if (!enemy.alive) continue;
          const Vec2 toEnemy = enemy.pos - sim.playerPos;
          const float dist = toEnemy.length();
          if (dist <= sweepRange + enemy.radius) {
            const Vec2 dirNorm = toEnemy.normalized();
            if (Vec2::dot(targetDir, dirNorm) >= std::cos(sweepAngleSpread * 0.5F)) {
              hitAny = true;
              const bool isCrit = (static_cast<float>(rand() % 1000) / 1000.0F) < sim.player.critChance;
              const float dmg = calculateDamageDealt(14.0F, sim.player.might, isCrit, sim.player.critDamage, enemy.armor);
              enemy.hp -= dmg;
              enemy.flashTimer = 0.1F;
              enemy.pos += dirNorm * 18.0F;

              sim.totalDamageDealt += dmg;
              sim.damageEvents.push_back({enemy.pos, dmg, isCrit, false, 0.6F});

              if (isCrit) {
                sim.hitStopTimer = 0.03F;
                sim.screenShake = 0.2F;
              }
            }
          }
        }
        if (hitAny) {
          sim.lastSlashArcCenter = sim.playerPos + targetDir * (sweepRange * 0.5F);
          sim.lastSlashArcAngle = std::atan2(targetDir.y, targetDir.x);
          sim.lastSlashArcTimer = 0.15F;
        }
        sim.player.primaryCooldown = 1.0F / 1.1F;
        break;
      }

      case ClassId::Archer: {
        // Aimed Arrow: piercing projectile
        DescentProjectile p;
        p.id = static_cast<int>(sim.nextEntityId++);
        p.pos = sim.playerPos;
        p.vel = sim.playerAim * 440.0F * (1.0F + sim.player.projectileSpeed);
        p.damage = 16.0F;
        if (sim.player.arrowCritCount > 0) {
          p.isCrit = true;
          sim.player.arrowCritCount--;
        } else {
          p.isCrit = (static_cast<float>(rand() % 1000) / 1000.0F) < sim.player.critChance;
        }
        p.pierceRemaining = 1;
        p.lifetime = 1.8F;
        p.fromPlayer = true;
        p.sourceArm = "archer_arrow";
        sim.projectiles.push_back(p);
        sim.player.primaryCooldown = 1.0F / 1.3F;
        break;
      }

      case ClassId::Tank: {
        // Shield Slam: 90 deg frontal cone, grants barrier
        const float slamRange = 55.0F * (1.0F + sim.player.area);
        for (auto& enemy : sim.enemies) {
          if (!enemy.alive) continue;
          const Vec2 toEnemy = enemy.pos - sim.playerPos;
          if (toEnemy.length() <= slamRange + enemy.radius) {
            if (Vec2::dot(sim.playerAim, toEnemy.normalized()) >= std::cos(45.0F * 0.0174533F)) {
              const float dmg = calculateDamageDealt(16.0F, sim.player.might, false, sim.player.critDamage, enemy.armor);
              enemy.hp -= dmg;
              enemy.flashTimer = 0.1F;
              enemy.pos += toEnemy.normalized() * 25.0F;
              sim.totalDamageDealt += dmg;
              sim.damageEvents.push_back({enemy.pos, dmg, false, false, 0.5F});

              // Grant small barrier
              sim.player.barrier = std::min(sim.player.maxBarrier, sim.player.barrier + 3.0F);
            }
          }
        }
        sim.player.primaryCooldown = 1.0F / 1.0F;
        break;
      }

      case ClassId::Mage: {
        // Arcane Bolt: homing projectile
        DescentProjectile p;
        p.id = static_cast<int>(sim.nextEntityId++);
        p.pos = sim.playerPos;
        p.vel = sim.playerAim * 320.0F * (1.0F + sim.player.projectileSpeed);
        p.damage = 15.0F;
        p.lifetime = 2.2F;
        p.fromPlayer = true;
        p.sourceArm = "arcane_bolt";
        sim.projectiles.push_back(p);
        sim.player.primaryCooldown = 1.0F / 1.2F;
        break;
      }

      case ClassId::Summoner: {
        // Soul Scythe: crescent projectile applying Hex
        DescentProjectile p;
        p.id = static_cast<int>(sim.nextEntityId++);
        p.pos = sim.playerPos;
        p.vel = sim.playerAim * 260.0F * (1.0F + sim.player.projectileSpeed);
        p.damage = 13.0F;
        p.pierceRemaining = 2;
        p.lifetime = 2.0F;
        p.fromPlayer = true;
        p.sourceArm = "soul_scythe";
        sim.projectiles.push_back(p);
        sim.player.primaryCooldown = 1.0F / 1.1F;
        break;
      }
    }
  }

  // Active Skill: Whirling Blades tick
  if (sim.player.whirlTimer > 0.0F) {
    sim.player.whirlTimer -= dt;
    for (auto& enemy : sim.enemies) {
      if (!enemy.alive) continue;
      if (Vec2::distance(enemy.pos, sim.playerPos) <= 75.0F + enemy.radius) {
        const float dmg = calculateDamageDealt(3.5F, sim.player.might, false, sim.player.critDamage, enemy.armor);
        enemy.hp -= dmg;
        enemy.flashTimer = 0.04F;
        sim.totalDamageDealt += dmg;
      }
    }
  }

  // Active Ultimate: Fortress Holy Pulse
  if (sim.player.fortressTimer > 0.0F) {
    sim.player.fortressTimer -= dt;
    sim.player.fortressTickTimer += dt;
    if (sim.player.fortressTickTimer >= 1.0F) {
      sim.player.fortressTickTimer -= 1.0F;
      for (auto& enemy : sim.enemies) {
        if (!enemy.alive) continue;
        if (Vec2::distance(enemy.pos, sim.playerPos) <= 120.0F + enemy.radius) {
          const float dmg = calculateDamageDealt(22.0F, sim.player.might, true, sim.player.critDamage, enemy.armor);
          enemy.hp -= dmg;
          enemy.flashTimer = 0.1F;
          sim.totalDamageDealt += dmg;
          sim.damageEvents.push_back({enemy.pos, dmg, true, false, 0.6F});
        }
      }
    }
  }

  // Active Ultimate: Rain of Arrows
  if (sim.player.rainOfArrowsTimer > 0.0F) {
    sim.player.rainOfArrowsTimer -= dt;
    for (auto& enemy : sim.enemies) {
      if (!enemy.alive) continue;
      if (Vec2::distance(enemy.pos, sim.player.rainTarget) <= 90.0F + enemy.radius) {
        const float dmg = calculateDamageDealt(4.0F, sim.player.might, false, sim.player.critDamage, enemy.armor);
        enemy.hp -= dmg;
        enemy.flashTimer = 0.05F;
        sim.totalDamageDealt += dmg;
      }
    }
  }

  // Active Ultimate: Meteor Swarm
  if (sim.player.meteorSwarmTimer > 0.0F) {
    sim.player.meteorSwarmTimer -= dt;
    if (rand() % 15 == 0 && !sim.enemies.empty()) {
      const auto& targetEnemy = sim.enemies[static_cast<std::size_t>(rand()) % sim.enemies.size()];
      if (targetEnemy.alive) {
        for (auto& enemy : sim.enemies) {
          if (!enemy.alive) continue;
          if (Vec2::distance(enemy.pos, targetEnemy.pos) <= 80.0F + enemy.radius) {
            const float dmg = calculateDamageDealt(40.0F, sim.player.might, true, sim.player.critDamage, enemy.armor);
            enemy.hp -= dmg;
            enemy.flashTimer = 0.15F;
            sim.totalDamageDealt += dmg;
            sim.damageEvents.push_back({enemy.pos, dmg, true, false, 0.7F});
          }
        }
      }
    }
  }

  // Arms Execution
  for (const auto& arm : sim.ownedArms) {
    if (arm.id == "ember_censer") {
      sim.censerTimer -= dt;
      if (sim.censerTimer <= 0.0F) {
        sim.censerTimer = 0.5F / (1.0F + std::max(0.0F, sim.player.attackSpeed));
        const float censerRadius = (60.0F + static_cast<float>(arm.level - 1) * 14.0F) * (1.0F + sim.player.area);
        const float baseDmg = 5.0F + static_cast<float>(arm.level - 1) * 2.5F;
        const float burnDps = (3.0F + static_cast<float>(arm.level - 1) * 1.5F) * (1.0F + sim.player.might);

        for (auto& enemy : sim.enemies) {
          if (!enemy.alive) continue;
          const float dist = Vec2::distance(enemy.pos, sim.playerPos);
          if (dist <= censerRadius + enemy.radius) {
            const bool isCrit = (static_cast<float>(rand() % 1000) / 1000.0F) < sim.player.critChance;
            const float dmg = calculateDamageDealt(baseDmg, sim.player.might, isCrit, sim.player.critDamage, enemy.armor);
            enemy.hp -= dmg;
            enemy.flashTimer = 0.08F;
            sim.totalDamageDealt += dmg;
            sim.damageEvents.push_back({enemy.pos, dmg, isCrit, false, 0.6F});

            bool foundBurn = false;
            for (auto& status : enemy.statuses) {
              if (status.type == StatusEffectType::Burn) {
                status.duration = 3.0F * (1.0F + sim.player.duration);
                if (status.stacks < 3) status.stacks++;
                status.dps = std::max(status.dps, burnDps);
                foundBurn = true;
                break;
              }
            }
            if (!foundBurn) {
              StatusEffect burnEffect;
              burnEffect.type = StatusEffectType::Burn;
              burnEffect.duration = 3.0F * (1.0F + sim.player.duration);
              burnEffect.tickTimer = 0.0F;
              burnEffect.dps = burnDps;
              burnEffect.stacks = 1;
              enemy.statuses.push_back(burnEffect);
            }
          }
        }
      }
    } else if (arm.id == "throwing_daggers") {
      sim.daggersTimer -= dt;
      if (sim.daggersTimer <= 0.0F) {
        sim.daggersTimer = 1.1F / (1.0F + sim.player.attackSpeed);
        // Find closest enemy
        Vec2 daggerDir = sim.playerAim;
        float bestDistSq = 999999.0F;
        for (const auto& e : sim.enemies) {
          if (!e.alive) continue;
          const float dSq = (e.pos - sim.playerPos).lengthSq();
          if (dSq < bestDistSq) {
            bestDistSq = dSq;
            daggerDir = (e.pos - sim.playerPos).normalized();
          }
        }
        const int daggerCount = 2 + arm.level;
        const float baseAngle = std::atan2(daggerDir.y, daggerDir.x);
        for (int i = 0; i < daggerCount; ++i) {
          const float spread = (static_cast<float>(i) - static_cast<float>(daggerCount - 1) * 0.5F) * 0.12F;
          DescentProjectile p;
          p.id = static_cast<int>(sim.nextEntityId++);
          p.pos = sim.playerPos;
          p.vel = Vec2{std::cos(baseAngle + spread), std::sin(baseAngle + spread)} * 460.0F;
          p.damage = 11.0F + static_cast<float>(arm.level) * 3.0F;
          p.pierceRemaining = 1;
          p.lifetime = 1.4F;
          p.fromPlayer = true;
          p.sourceArm = "throwing_daggers";
          sim.projectiles.push_back(p);
        }
      }
    } else if (arm.id == "orbiting_runeblades") {
      sim.runebladesAngle += dt * 3.5F;
      const int bladeCount = 2 + arm.level;
      const float bladeRadius = 65.0F * (1.0F + sim.player.area);
      for (int i = 0; i < bladeCount; ++i) {
        const float angle = sim.runebladesAngle + (static_cast<float>(i) / static_cast<float>(bladeCount)) * 6.283185F;
        const Vec2 bladePos = sim.playerPos + Vec2{std::cos(angle), std::sin(angle)} * bladeRadius;
        for (auto& enemy : sim.enemies) {
          if (!enemy.alive) continue;
          if (Vec2::distance(enemy.pos, bladePos) <= 18.0F + enemy.radius) {
            const float dmg = calculateDamageDealt(6.0F + static_cast<float>(arm.level) * 2.0F, sim.player.might, false, sim.player.critDamage, enemy.armor);
            enemy.hp -= dmg;
            enemy.flashTimer = 0.05F;
            sim.totalDamageDealt += dmg;
          }
        }
      }
    } else if (arm.id == "chain_lightning_rod") {
      sim.lightningTimer -= dt;
      if (sim.lightningTimer <= 0.0F) {
        sim.lightningTimer = 1.8F / (1.0F + sim.player.attackSpeed);
        // Find closest enemy to strike
        int targetIdx = -1;
        float bestDistSq = 350.0F * 350.0F;
        for (std::size_t i = 0; i < sim.enemies.size(); ++i) {
          if (!sim.enemies[i].alive) continue;
          const float dSq = (sim.enemies[i].pos - sim.playerPos).lengthSq();
          if (dSq < bestDistSq) {
            bestDistSq = dSq;
            targetIdx = static_cast<int>(i);
          }
        }
        if (targetIdx != -1) {
          auto& first = sim.enemies[static_cast<std::size_t>(targetIdx)];
          const float dmg = calculateDamageDealt(18.0F + static_cast<float>(arm.level) * 5.0F, sim.player.might, true, sim.player.critDamage, first.armor);
          first.hp -= dmg;
          first.flashTimer = 0.12F;
          sim.totalDamageDealt += dmg;
          sim.damageEvents.push_back({first.pos, dmg, true, false, 0.6F});
          sim.lightningArcs.push_back({sim.playerPos, first.pos, 0.22F, 0.22F});

          // Chain to 2 nearby enemies
          int chained = 0;
          for (auto& other : sim.enemies) {
            if (!other.alive || other.id == first.id) continue;
            if (Vec2::distance(other.pos, first.pos) <= 120.0F) {
              const float cDmg = dmg * 0.75F;
              other.hp -= cDmg;
              other.flashTimer = 0.1F;
              sim.totalDamageDealt += cDmg;
              sim.damageEvents.push_back({other.pos, cDmg, false, false, 0.5F});
              sim.lightningArcs.push_back({first.pos, other.pos, 0.20F, 0.20F});
              chained++;
              if (chained >= 1 + arm.level) break;
            }
          }
        }
      }
    } else if (arm.id == "frost_bell") {
      sim.frostBellTimer -= dt;
      if (sim.frostBellTimer <= 0.0F) {
        sim.frostBellTimer = 2.8F / (1.0F + sim.player.attackSpeed);
        const float radius = 130.0F * (1.0F + sim.player.area);
        sim.frostRings.push_back({sim.playerPos, 20.0F, radius, 0.45F, 0.45F});
        for (auto& enemy : sim.enemies) {
          if (!enemy.alive) continue;
          if (Vec2::distance(enemy.pos, sim.playerPos) <= radius + enemy.radius) {
            const float dmg = calculateDamageDealt(12.0F + static_cast<float>(arm.level) * 3.0F, sim.player.might, false, sim.player.critDamage, enemy.armor);
            enemy.hp -= dmg;
            enemy.flashTimer = 0.1F;
            sim.totalDamageDealt += dmg;
            sim.damageEvents.push_back({enemy.pos, dmg, false, false, 0.5F});

            StatusEffect chill;
            chill.type = StatusEffectType::Chill;
            chill.duration = 2.5F;
            enemy.statuses.push_back(chill);
          }
        }
      }
    } else if (arm.id == "thorn_bramble") {
      sim.brambleTimer -= dt;
      if (sim.brambleTimer <= 0.0F) {
        sim.brambleTimer = 2.0F;
        ThornPatch patch;
        patch.pos = sim.playerPos;
        patch.radius = (36.0F + static_cast<float>(arm.level) * 6.0F) * (1.0F + sim.player.area);
        patch.damage = 6.0F + static_cast<float>(arm.level) * 2.0F;
        patch.lifetime = 5.0F * (1.0F + sim.player.duration);
        sim.thornPatches.push_back(patch);
      }
    } else if (arm.id == "boomerang_axe") {
      sim.axeTimer -= dt;
      if (sim.axeTimer <= 0.0F) {
        sim.axeTimer = 2.2F / (1.0F + sim.player.attackSpeed);
        DescentProjectile p;
        p.id = static_cast<int>(sim.nextEntityId++);
        p.pos = sim.playerPos;
        p.vel = sim.playerAim * 320.0F;
        p.damage = 18.0F + static_cast<float>(arm.level) * 5.0F;
        p.pierceRemaining = 99;
        p.lifetime = 1.4F;
        p.fromPlayer = true;
        p.sourceArm = "boomerang_axe";
        sim.projectiles.push_back(p);
      }
    } else if (arm.id == "holy_lantern") {
      sim.lanternAngle += dt * 2.2F;
      sim.lanternTimer -= dt;
      if (sim.lanternTimer <= 0.0F) {
        sim.lanternTimer = 0.2F;
        const Vec2 beamDir{std::cos(sim.lanternAngle), std::sin(sim.lanternAngle)};
        for (auto& enemy : sim.enemies) {
          if (!enemy.alive) continue;
          const Vec2 toEnemy = enemy.pos - sim.playerPos;
          if (toEnemy.length() <= 160.0F && Vec2::dot(beamDir, toEnemy.normalized()) >= 0.85F) {
            const float dmg = calculateDamageDealt(5.0F + static_cast<float>(arm.level) * 2.0F, sim.player.might, false, sim.player.critDamage, enemy.armor);
            enemy.hp -= dmg;
            enemy.flashTimer = 0.06F;
            sim.totalDamageDealt += dmg;
          }
        }
      }
    } else if (arm.id == "gravity_well") {
      sim.gravityWellTimer -= dt;
      if (sim.gravityWellTimer <= 0.0F) {
        sim.gravityWellTimer = 4.5F;
        // Vortex at cursor aim
        const Vec2 wellCenter = sim.playerPos + sim.playerAim * 120.0F;
        for (auto& enemy : sim.enemies) {
          if (!enemy.alive) continue;
          const float dist = Vec2::distance(enemy.pos, wellCenter);
          if (dist <= 140.0F) {
            enemy.pos += (wellCenter - enemy.pos).normalized() * 50.0F * dt;
            const float dmg = calculateDamageDealt(14.0F + static_cast<float>(arm.level) * 3.0F, sim.player.might, false, sim.player.critDamage, enemy.armor);
            enemy.hp -= dmg;
            sim.totalDamageDealt += dmg;
          }
        }
      }
    } else if (arm.id == "skeletal_horde") {
      sim.skeletalHordeTimer -= dt;
      const std::size_t maxSkeletons = 1 + arm.level;
      std::size_t activeCount = 0;
      for (const auto& m : sim.friendlyMinions) {
        if (m.alive) activeCount++;
      }
      if (activeCount < maxSkeletons && sim.skeletalHordeTimer <= 0.0F) {
        sim.skeletalHordeTimer = 3.2F;
        FriendlyMinion m;
        m.id = static_cast<int>(sim.nextEntityId++);
        m.pos = sim.playerPos + Vec2{static_cast<float>((rand() % 40) - 20), static_cast<float>((rand() % 40) - 20)};
        m.radius = 13.0F;
        m.maxHp = 35.0F + static_cast<float>(arm.level) * 15.0F;
        m.hp = m.maxHp;
        m.damage = (8.0F + static_cast<float>(arm.level) * 3.5F) * (1.0F + sim.player.might);
        m.speed = 145.0F;
        m.lifetime = 15.0F * (1.0F + sim.player.duration);
        m.alive = true;
        sim.friendlyMinions.push_back(m);
      }
    } else if (arm.id == "soul_siphon") {
      sim.soulSiphonTimer -= dt;
      if (sim.soulSiphonTimer <= 0.0F) {
        sim.soulSiphonTimer = 3.6F / (1.0F + sim.player.attackSpeed);
        int hitCount = 0;
        const float siphonRadius = (95.0F + static_cast<float>(arm.level) * 15.0F) * (1.0F + sim.player.area);
        sim.frostRings.push_back({sim.playerPos, 15.0F, siphonRadius, 0.35F, 0.35F});
        for (auto& enemy : sim.enemies) {
          if (!enemy.alive) continue;
          if (Vec2::distance(enemy.pos, sim.playerPos) <= siphonRadius + enemy.radius) {
            const float dmg = calculateDamageDealt(10.0F + static_cast<float>(arm.level) * 3.5F, sim.player.might, false, sim.player.critDamage, enemy.armor);
            enemy.hp -= dmg;
            enemy.flashTimer = 0.1F;
            sim.totalDamageDealt += dmg;
            sim.damageEvents.push_back({enemy.pos, dmg, false, false, 0.5F});
            hitCount++;
          }
        }
        if (hitCount > 0) {
          const float healAmt = std::min(sim.player.maxHp * 0.05F, static_cast<float>(hitCount) * 1.5F);
          sim.player.currentHp = std::min(sim.player.maxHp, sim.player.currentHp + healAmt);
        }
      }
    } else if (arm.id == "arcane_orb") {
      sim.arcaneOrbTimer -= dt;
      if (sim.arcaneOrbTimer <= 0.0F) {
        sim.arcaneOrbTimer = 2.2F / (1.0F + sim.player.attackSpeed);
        DescentProjectile p;
        p.id = static_cast<int>(sim.nextEntityId++);
        p.pos = sim.playerPos;
        p.vel = sim.playerAim * 190.0F;
        p.damage = 16.0F + static_cast<float>(arm.level) * 5.5F;
        p.radius = 14.0F;
        p.pierceRemaining = 99;
        p.lifetime = 1.6F;
        p.fromPlayer = true;
        p.sourceArm = "arcane_orb";
        sim.projectiles.push_back(p);
      }
    } else if (arm.id == "seeking_gale") {
      sim.seekingGaleTimer -= dt;
      if (sim.seekingGaleTimer <= 0.0F) {
        sim.seekingGaleTimer = 1.3F / (1.0F + sim.player.attackSpeed);
        const int arrowCount = 1 + arm.level;
        Vec2 aimDir = sim.playerAim;
        float bestDistSq = 400.0F * 400.0F;
        for (const auto& e : sim.enemies) {
          if (!e.alive) continue;
          const float dSq = (e.pos - sim.playerPos).lengthSq();
          if (dSq < bestDistSq) {
            bestDistSq = dSq;
            aimDir = (e.pos - sim.playerPos).normalized();
          }
        }
        const float baseAngle = std::atan2(aimDir.y, aimDir.x);
        for (int i = 0; i < arrowCount; ++i) {
          const float spread = (static_cast<float>(i) - static_cast<float>(arrowCount - 1) * 0.5F) * 0.22F;
          DescentProjectile p;
          p.id = static_cast<int>(sim.nextEntityId++);
          p.pos = sim.playerPos;
          p.vel = Vec2{std::cos(baseAngle + spread), std::sin(baseAngle + spread)} * 380.0F;
          p.damage = 11.0F + static_cast<float>(arm.level) * 3.5F;
          p.radius = 6.0F;
          p.pierceRemaining = 1;
          p.lifetime = 1.8F;
          p.fromPlayer = true;
          p.sourceArm = "seeking_gale";
          sim.projectiles.push_back(p);
        }
      }
    } else if (arm.id == "earth_tremor") {
      sim.earthTremorTimer -= dt;
      if (sim.earthTremorTimer <= 0.0F) {
        sim.earthTremorTimer = 2.8F / (1.0F + sim.player.attackSpeed);
        const float tremorRadius = (80.0F + static_cast<float>(arm.level) * 16.0F) * (1.0F + sim.player.area);
        sim.frostRings.push_back({sim.playerPos, 15.0F, tremorRadius, 0.35F, 0.35F});
        sim.screenShake = std::max(sim.screenShake, 0.22F);
        for (auto& enemy : sim.enemies) {
          if (!enemy.alive) continue;
          const float dist = Vec2::distance(enemy.pos, sim.playerPos);
          if (dist <= tremorRadius + enemy.radius) {
            const bool isCrit = (static_cast<float>(rand() % 1000) / 1000.0F) < sim.player.critChance;
            const float dmg = calculateDamageDealt(15.0F + static_cast<float>(arm.level) * 4.5F, sim.player.might, isCrit, sim.player.critDamage, enemy.armor);
            enemy.hp -= dmg;
            enemy.flashTimer = 0.12F;
            const Vec2 kb = (enemy.pos - sim.playerPos).normalized() * 50.0F;
            enemy.pos += kb;
            sim.totalDamageDealt += dmg;
            sim.damageEvents.push_back({enemy.pos, dmg, isCrit, false, 0.6F});
          }
        }
      }
    }
  }

  // Update Thorn Patches
  for (auto it = sim.thornPatches.begin(); it != sim.thornPatches.end(); ) {
    it->lifetime -= dt;
    it->tickTimer += dt;
    if (it->tickTimer >= 0.5F) {
      it->tickTimer -= 0.5F;
      for (auto& enemy : sim.enemies) {
        if (!enemy.alive) continue;
        if (Vec2::distance(enemy.pos, it->pos) <= it->radius + enemy.radius) {
          const float dmg = calculateDamageDealt(it->damage * 0.5F, sim.player.might, false, sim.player.critDamage, enemy.armor);
          enemy.hp -= dmg;
          enemy.flashTimer = 0.05F;
          sim.totalDamageDealt += dmg;
        }
      }
    }
    if (it->lifetime <= 0.0F) {
      it = sim.thornPatches.erase(it);
    } else {
      ++it;
    }
  }

  // Update Friendly Minions
  for (auto it = sim.friendlyMinions.begin(); it != sim.friendlyMinions.end(); ) {
    it->lifetime -= dt;
    if (it->lifetime <= 0.0F || it->hp <= 0.0F) {
      it = sim.friendlyMinions.erase(it);
      continue;
    }

    // Seek nearest enemy
    int targetIdx = -1;
    float bestDistSq = 999999.0F;
    for (std::size_t i = 0; i < sim.enemies.size(); ++i) {
      if (!sim.enemies[i].alive) continue;
      const float dSq = (sim.enemies[i].pos - it->pos).lengthSq();
      if (dSq < bestDistSq) {
        bestDistSq = dSq;
        targetIdx = static_cast<int>(i);
      }
    }

    if (targetIdx != -1) {
      auto& target = sim.enemies[static_cast<std::size_t>(targetIdx)];
      const Vec2 toTarget = target.pos - it->pos;
      if (toTarget.length() > 20.0F) {
        it->pos += toTarget.normalized() * it->speed * dt;
      }
      it->attackCooldown -= dt;
      if (toTarget.length() <= 24.0F + target.radius && it->attackCooldown <= 0.0F) {
        it->attackCooldown = 0.6F;
        const float dmg = calculateDamageDealt(it->damage, sim.player.might, false, sim.player.critDamage, target.armor);
        target.hp -= dmg;
        target.flashTimer = 0.08F;
        sim.totalDamageDealt += dmg;
      }
    } else {
      // Follow player
      if (Vec2::distance(it->pos, sim.playerPos) > 40.0F) {
        it->pos += (sim.playerPos - it->pos).normalized() * it->speed * dt;
      }
    }
    ++it;
  }

  // Update Projectiles
  std::vector<DescentProjectile> spawnedProjectiles;
  for (auto it = sim.projectiles.begin(); it != sim.projectiles.end(); ) {
    it->lifetime -= dt;
    it->pos += it->vel * dt;

    // Boomerang returning axe behavior
    if (it->sourceArm == "boomerang_axe" && it->lifetime < 0.7F) {
      const Vec2 toPlayer = (sim.playerPos - it->pos).normalized();
      it->vel = toPlayer * 340.0F;
    }

    // Homing seeking gale arrows
    if (it->fromPlayer && it->sourceArm == "seeking_gale") {
      float closestDistSq = 350.0F * 350.0F;
      Vec2 homingTarget = it->vel;
      for (const auto& enemy : sim.enemies) {
        if (!enemy.alive) continue;
        const float dSq = (enemy.pos - it->pos).lengthSq();
        if (dSq < closestDistSq) {
          closestDistSq = dSq;
          homingTarget = (enemy.pos - it->pos).normalized() * 380.0F;
        }
      }
      it->vel = (it->vel + homingTarget * 0.25F).normalized() * 380.0F;
    }

    // Void orb homing towards player
    if (!it->fromPlayer && it->sourceArm == "void_orb") {
      const Vec2 toPlayer = (sim.playerPos - it->pos).normalized() * 160.0F;
      it->vel = (it->vel + toPlayer * 0.12F).normalized() * 160.0F;
    }

    if (it->lifetime <= 0.0F ||
        it->pos.x < 32.0F || it->pos.x > worldW - 32.0F ||
        it->pos.y < 32.0F || it->pos.y > worldH - 32.0F) {
      if (it->sourceArm == "arcane_orb") {
        for (int k = 0; k < 6; ++k) {
          const float angle = (static_cast<float>(k) / 6.0F) * 6.283185F;
          DescentProjectile bolt;
          bolt.id = static_cast<int>(sim.nextEntityId++);
          bolt.pos = it->pos;
          bolt.vel = Vec2{std::cos(angle), std::sin(angle)} * 260.0F;
          bolt.damage = it->damage * 0.55F;
          bolt.radius = 6.0F;
          bolt.pierceRemaining = 1;
          bolt.lifetime = 0.6F;
          bolt.fromPlayer = true;
          bolt.sourceArm = "arcane_bolt";
          spawnedProjectiles.push_back(bolt);
        }
      }
      it = sim.projectiles.erase(it);
      continue;
    }

    bool hitSomething = false;

    if (it->fromPlayer) {
      // Homing Arcane Bolt adjustment
      if (it->sourceArm == "arcane_bolt") {
        float closestDistSq = 200.0F * 200.0F;
        Vec2 homingTarget = it->vel;
        for (const auto& enemy : sim.enemies) {
          if (!enemy.alive) continue;
          const float dSq = (enemy.pos - it->pos).lengthSq();
          if (dSq < closestDistSq) {
            closestDistSq = dSq;
            homingTarget = (enemy.pos - it->pos).normalized() * 320.0F;
          }
        }
        it->vel = (it->vel + homingTarget * 0.15F).normalized() * 320.0F;
      }

      for (auto& enemy : sim.enemies) {
        if (!enemy.alive) continue;
        if (Vec2::distance(enemy.pos, it->pos) <= it->radius + enemy.radius) {
          const float dmg = calculateDamageDealt(it->damage, sim.player.might, it->isCrit, sim.player.critDamage, enemy.armor);
          enemy.hp -= dmg;
          enemy.flashTimer = 0.08F;
          sim.totalDamageDealt += dmg;
          sim.damageEvents.push_back({enemy.pos, dmg, it->isCrit, false, 0.6F});

          // Soul Scythe Hex
          if (it->sourceArm == "soul_scythe") {
            StatusEffect hex;
            hex.type = StatusEffectType::Hex;
            hex.duration = 4.0F;
            enemy.statuses.push_back(hex);
          }

          if (it->pierceRemaining > 0) {
            it->pierceRemaining--;
          } else {
            hitSomething = true;
            break;
          }
        }
      }
    } else {
      // Enemy projectile hit player
      if (Vec2::distance(sim.playerPos, it->pos) <= it->radius + 15.0F && sim.player.invulnerabilityTimer <= 0.0F) {
        const float taken = calculateDamageTaken(it->damage, sim.player.damageReduction, sim.player.armor);
        float hpDmg = taken;
        if (sim.player.barrier > 0.0F) {
          if (sim.player.barrier >= hpDmg) {
            sim.player.barrier -= hpDmg;
            hpDmg = 0.0F;
          } else {
            hpDmg -= sim.player.barrier;
            sim.player.barrier = 0.0F;
          }
        }
        sim.player.currentHp = std::max(0.0F, sim.player.currentHp - hpDmg);
        sim.player.invulnerabilityTimer = 0.40F;
        sim.screenShake = 0.3F;
        sim.totalDamageTaken += taken;
        sim.player.wrath = std::min(100.0F, sim.player.wrath + 2.0F * (taken / 10.0F));
        sim.damageEvents.push_back({sim.playerPos, taken, false, true, 0.6F});
        hitSomething = true;
      }
    }

    if (hitSomething) {
      it = sim.projectiles.erase(it);
    } else {
      ++it;
    }
  }

  if (!spawnedProjectiles.empty()) {
    sim.projectiles.insert(sim.projectiles.end(), spawnedProjectiles.begin(), spawnedProjectiles.end());
  }

  // Update Spatial Hash with enemies
  sim.spatialHash.clear();
  for (std::size_t i = 0; i < sim.enemies.size(); ++i) {
    if (sim.enemies[i].alive) {
      sim.spatialHash.insert(static_cast<int>(i), sim.enemies[i].pos);
    }
  }

  // Update Enemies
  std::vector<int> nearbyIndices;
  nearbyIndices.reserve(32);
  for (auto& enemy : sim.enemies) {
    if (!enemy.alive) continue;

    if (enemy.flashTimer > 0.0F) enemy.flashTimer -= dt;

    // Check status effects modifiers
    bool isFrozen = false;
    float speedMod = 1.0F;
    float damageTakenMod = 1.0F;

    for (auto it = enemy.statuses.begin(); it != enemy.statuses.end(); ) {
      it->duration -= dt;
      it->tickTimer += dt;
      if (it->type == StatusEffectType::Freeze) {
        isFrozen = true;
        damageTakenMod *= 1.50F;
      } else if (it->type == StatusEffectType::Chill) {
        speedMod *= 0.70F;
      } else if (it->type == StatusEffectType::Hex) {
        damageTakenMod *= 1.12F;
      } else if (it->type == StatusEffectType::Burn && it->tickTimer >= 0.5F) {
        it->tickTimer -= 0.5F;
        const float burnTickDmg = (it->dps * 0.5F) * static_cast<float>(it->stacks);
        enemy.hp -= burnTickDmg;
        enemy.flashTimer = 0.05F;
        sim.totalDamageDealt += burnTickDmg;
        sim.damageEvents.push_back({enemy.pos, burnTickDmg, false, false, 0.5F});
      }

      if (it->duration <= 0.0F) {
        it = enemy.statuses.erase(it);
      } else {
        ++it;
      }
    }

    if (isFrozen) {
      continue;
    }

    // Archetype-specific AI
    const float distToPlayer = Vec2::distance(enemy.pos, sim.playerPos);

    if (enemy.archetype == EnemyArchetype::Shooter) {
      enemy.shootCooldown -= dt;
      if (distToPlayer < 180.0F) {
        // Back away
        enemy.pos += (enemy.pos - sim.playerPos).normalized() * enemy.speed * speedMod * dt;
      } else if (distToPlayer > 260.0F) {
        // Move closer
        enemy.pos += (sim.playerPos - enemy.pos).normalized() * enemy.speed * speedMod * dt;
      }
      if (enemy.shootCooldown <= 0.0F) {
        enemy.shootCooldown = 2.0F;
        DescentProjectile bp;
        bp.id = static_cast<int>(sim.nextEntityId++);
        bp.pos = enemy.pos;
        bp.vel = (sim.playerPos - enemy.pos).normalized() * 220.0F;
        bp.damage = 7.0F * sim.heatDamageMult;
        bp.lifetime = 2.5F;
        bp.fromPlayer = false;
        bp.sourceArm = "bone_arrow";
        sim.projectiles.push_back(bp);
      }
    } else if (enemy.archetype == EnemyArchetype::Charger) {
      if (enemy.isCharging) {
        enemy.chargeTimer -= dt;
        enemy.pos += enemy.chargeDir * 260.0F * sim.heatSpeedMult * dt;
        if (enemy.chargeTimer <= 0.0F) {
          enemy.isCharging = false;
        }
      } else {
        if (distToPlayer < 240.0F && (rand() % 40 == 0)) {
          enemy.isCharging = true;
          enemy.chargeTimer = 0.8F;
          enemy.chargeDir = (sim.playerPos - enemy.pos).normalized();
        } else {
          Vec2 moveDir = sim.flowField.getDirection(enemy.pos.x, enemy.pos.y);
          enemy.pos += moveDir * enemy.speed * speedMod * dt;
        }
      }
    } else if (enemy.archetype == EnemyArchetype::Exploder) {
      if (enemy.isFusing) {
        enemy.fuseTimer -= dt;
        enemy.flashTimer = 0.05F;
        if (enemy.fuseTimer <= 0.0F) {
          // Detonate!
          enemy.hp = 0.0F;
          if (distToPlayer <= 60.0F && sim.player.invulnerabilityTimer <= 0.0F) {
            const float taken = calculateDamageTaken(25.0F * sim.heatDamageMult, sim.player.damageReduction, sim.player.armor);
            sim.player.currentHp = std::max(0.0F, sim.player.currentHp - taken);
            sim.player.invulnerabilityTimer = 0.4F;
            sim.damageEvents.push_back({sim.playerPos, taken, false, true, 0.6F});
          }
        }
      } else {
        if (distToPlayer <= 48.0F) {
          enemy.isFusing = true;
          enemy.fuseTimer = 0.9F;
        } else {
          Vec2 moveDir = sim.flowField.getDirection(enemy.pos.x, enemy.pos.y);
          enemy.pos += moveDir * enemy.speed * speedMod * dt;
        }
      }
    } else if (enemy.archetype == EnemyArchetype::Support) {
      enemy.supportTimer -= dt;
      if (enemy.supportTimer <= 0.0F) {
        enemy.supportTimer = 2.8F;
        for (auto& other : sim.enemies) {
          if (!other.alive || other.id == enemy.id) continue;
          if (Vec2::distance(other.pos, enemy.pos) <= 150.0F && other.hp < other.maxHp) {
            other.hp = std::min(other.maxHp, other.hp + 20.0F);
            other.flashTimer = 0.1F;
          }
        }
      }
      Vec2 moveDir = sim.flowField.getDirection(enemy.pos.x, enemy.pos.y);
      enemy.pos += moveDir * enemy.speed * speedMod * dt;
    } else if (enemy.archetype == EnemyArchetype::Spawner) {
      enemy.spawnTimer -= dt;
      if (enemy.spawnTimer <= 0.0F) {
        enemy.spawnTimer = 3.5F;
        spawnEnemy(sim, EnemyArchetype::Swarmer, enemy.pos + Vec2{15.0F, 0.0F});
      }
      // Very slow movement
      Vec2 moveDir = sim.flowField.getDirection(enemy.pos.x, enemy.pos.y);
      enemy.pos += moveDir * enemy.speed * speedMod * dt;
    } else if (enemy.archetype == EnemyArchetype::Stalker) {
      enemy.supportTimer -= dt;
      if (enemy.supportTimer <= 0.0F && distToPlayer <= 220.0F && distToPlayer >= 80.0F) {
        enemy.supportTimer = 3.2F;
        const Vec2 dashDir = (sim.playerPos - enemy.pos).normalized();
        enemy.pos = sim.playerPos - dashDir * 35.0F;
        enemy.flashTimer = 0.20F;
        sim.screenShake = std::max(sim.screenShake, 0.15F);
      } else {
        const Vec2 moveDir = (sim.playerPos - enemy.pos).normalized();
        enemy.pos += moveDir * enemy.speed * speedMod * dt;
      }
    } else if (enemy.archetype == EnemyArchetype::VoidCaster) {
      enemy.shootCooldown -= dt;
      if (distToPlayer < 190.0F) {
        enemy.pos += (enemy.pos - sim.playerPos).normalized() * enemy.speed * speedMod * dt;
      } else if (distToPlayer > 280.0F) {
        enemy.pos += (sim.playerPos - enemy.pos).normalized() * enemy.speed * speedMod * dt;
      }
      if (enemy.shootCooldown <= 0.0F) {
        enemy.shootCooldown = 2.6F;
        DescentProjectile vp;
        vp.id = static_cast<int>(sim.nextEntityId++);
        vp.pos = enemy.pos;
        vp.vel = (sim.playerPos - enemy.pos).normalized() * 160.0F;
        vp.damage = 9.0F * sim.heatDamageMult;
        vp.lifetime = 3.0F;
        vp.radius = 7.0F;
        vp.fromPlayer = false;
        vp.sourceArm = "void_orb";
        sim.projectiles.push_back(vp);
      }
    } else {
      // Swarmers, Grunts, Brutes standard flow
      Vec2 moveDir = sim.flowField.getDirection(enemy.pos.x, enemy.pos.y);
      if (moveDir.lengthSq() < 0.01F) {
        moveDir = (sim.playerPos - enemy.pos).normalized();
      }

      // Separation
      sim.spatialHash.query(enemy.pos, enemy.radius * 2.0F, nearbyIndices);
      Vec2 separation{0.0F, 0.0F};
      int sepCount = 0;
      for (int otherIdx : nearbyIndices) {
        if (otherIdx >= 0 && static_cast<std::size_t>(otherIdx) < sim.enemies.size()) {
          const auto& other = sim.enemies[static_cast<std::size_t>(otherIdx)];
          if (other.id != enemy.id && other.alive) {
            const Vec2 diff = enemy.pos - other.pos;
            const float distSq = diff.lengthSq();
            const float minDist = enemy.radius + other.radius;
            if (distSq > 0.001F && distSq < minDist * minDist) {
              separation += diff.normalized() * (minDist - std::sqrt(distSq));
              sepCount++;
              if (sepCount >= 6) break;
            }
          }
        }
      }

      Vec2 finalVel = (moveDir + separation * 1.5F).normalized() * enemy.speed * speedMod;
      enemy.pos += finalVel * dt;
    }

    // Boss Osric Mechanics
    if (enemy.isBoss) {
      sim.bossCurrentHp = enemy.hp;
      sim.bossMaxHp = enemy.maxHp;

      // Phase 2 check (under 50% HP)
      if (enemy.bossPhase == 1 && enemy.hp <= enemy.maxHp * 0.50F) {
        enemy.bossPhase = 2;
        sim.bossPhase = 2;
        enemy.speed *= 1.35F;
      }

      enemy.bossSpecialCooldown -= dt;
      if (enemy.bossSpecialCooldown <= 0.0F) {
        if (enemy.bossPhase == 1) {
          // Phase 1: 8-way radial bone burst
          enemy.bossSpecialCooldown = 3.2F;
          for (int i = 0; i < 8; ++i) {
            const float angle = (static_cast<float>(i) / 8.0F) * 6.283185F;
            DescentProjectile bp;
            bp.id = static_cast<int>(sim.nextEntityId++);
            bp.pos = enemy.pos;
            bp.vel = Vec2{std::cos(angle), std::sin(angle)} * 200.0F;
            bp.damage = 12.0F * sim.heatDamageMult;
            bp.lifetime = 3.0F;
            bp.fromPlayer = false;
            bp.sourceArm = "osric_bone_burst";
            sim.projectiles.push_back(bp);
          }
        } else {
          // Phase 2: Gravefall Leap Slam
          enemy.bossSpecialCooldown = 4.0F;
          enemy.isLeaping = true;
          enemy.leapTimer = 1.0F;
          enemy.leapTarget = sim.playerPos;
        }
      }

      if (enemy.isLeaping) {
        enemy.leapTimer -= dt;
        if (enemy.leapTimer <= 0.0F) {
          enemy.isLeaping = false;
          enemy.pos = enemy.leapTarget;
          // Shockwave damage
          if (Vec2::distance(sim.playerPos, enemy.pos) <= 85.0F && sim.player.invulnerabilityTimer <= 0.0F) {
            const float taken = calculateDamageTaken(38.0F * sim.heatDamageMult, sim.player.damageReduction, sim.player.armor);
            sim.player.currentHp = std::max(0.0F, sim.player.currentHp - taken);
            sim.player.invulnerabilityTimer = 0.5F;
            sim.screenShake = 0.5F;
            sim.damageEvents.push_back({sim.playerPos, taken, false, true, 0.7F});
          }
        }
      }
    }

    // Bounds clamp
    enemy.pos.x = std::clamp(enemy.pos.x, DescentSimState::kTileSize * 1.2F, worldW - DescentSimState::kTileSize * 1.2F);
    enemy.pos.y = std::clamp(enemy.pos.y, DescentSimState::kTileSize * 1.2F, worldH - DescentSimState::kTileSize * 1.2F);

    // Contact damage against player
    if (enemy.hp > 0.0F && distToPlayer <= (enemy.radius + 14.0F) && sim.player.invulnerabilityTimer <= 0.0F) {
      float incoming = enemy.contactDamage;
      // Tank Fortress 50% damage reduction
      if (sim.player.fortressTimer > 0.0F) incoming *= 0.50F;

      const float taken = calculateDamageTaken(incoming, sim.player.damageReduction, sim.player.armor);
      
      float hpDamage = taken;
      if (sim.player.barrier > 0.0F) {
        if (sim.player.barrier >= hpDamage) {
          sim.player.barrier -= hpDamage;
          hpDamage = 0.0F;
        } else {
          hpDamage -= sim.player.barrier;
          sim.player.barrier = 0.0F;
        }
      }

      sim.player.currentHp = std::max(0.0F, sim.player.currentHp - hpDamage);
      sim.player.invulnerabilityTimer = 0.50F;
      sim.screenShake = 0.35F;
      sim.totalDamageTaken += taken;
      sim.player.wrath = std::min(100.0F, sim.player.wrath + 2.0F * (taken / 10.0F));

      sim.damageEvents.push_back({sim.playerPos, taken, false, true, 0.6F});

      // Bulwark Reflect 30% back to attacker
      if (sim.player.bulwarkTimer > 0.0F) {
        const float reflected = taken * 0.30F;
        enemy.hp -= reflected;
        enemy.flashTimer = 0.1F;
        sim.totalDamageDealt += reflected;
      }
    }

    // Check enemy death
    if (enemy.hp <= 0.0F) {
      enemy.alive = false;
      sim.enemiesKilled++;
      sim.currentWaveKilled++;
      sim.player.wrath = std::min(100.0F, sim.player.wrath + (enemy.isBoss ? 50.0F : (enemy.isElite ? 10.0F : 1.0F)));

      // Splitter spawns 2 swarmers on death
      if (enemy.archetype == EnemyArchetype::Splitter) {
        spawnEnemy(sim, EnemyArchetype::Swarmer, enemy.pos + Vec2{-12.0F, 0.0F});
        spawnEnemy(sim, EnemyArchetype::Swarmer, enemy.pos + Vec2{12.0F, 0.0F});
      }

      // Drop XP Gem
      DescentPickup p;
      p.id = static_cast<int>(sim.nextEntityId++);
      p.pos = enemy.pos;
      p.type = (enemy.isBoss || enemy.isElite) ? PickupType::XpGemLarge : PickupType::XpGemSmall;
      p.value = enemy.xpReward;
      sim.pickups.push_back(p);

      // Embers drop
      if (enemy.isBoss || enemy.isElite || (rand() % 100) < 30) {
        DescentPickup emberDrop;
        emberDrop.id = static_cast<int>(sim.nextEntityId++);
        emberDrop.pos = enemy.pos + Vec2{8.0F, 8.0F};
        emberDrop.type = PickupType::Ember;
        emberDrop.value = enemy.isBoss ? 150.0F : (enemy.isElite ? 35.0F : 4.0F);
        sim.pickups.push_back(emberDrop);
      }

      if (enemy.isBoss) {
        sim.bossDefeated = true;
        sim.roomCleared = true;
        if (!sim.isEndless) {
          sim.runCompleted = true;
          sim.doorChoicesPending = false;
        } else {
          // Endless mode: guardian vanquished! Advance loop, award bonus spoils & open doors
          sim.endlessLoop++;
          sim.embers += 200 * sim.endlessLoop;
          sim.soulFragments += 2;
          sim.crownShards += 1;
          sim.player.currentHp = sim.player.maxHp;
          sim.doorChoicesPending = true;
          generateChamberChoices(sim);
        }
      }
    }
  }

  // Update Pickups
  for (auto& pickup : sim.pickups) {
    if (!pickup.alive) continue;
    const float dist = Vec2::distance(pickup.pos, sim.playerPos);

    if (pickup.magnetized || dist <= sim.player.pickupRadius) {
      const Vec2 toPlayer = (sim.playerPos - pickup.pos).normalized();
      const float magSpeed = pickup.magnetized ? 540.0F : 380.0F;
      pickup.pos += toPlayer * magSpeed * dt;

      if (dist <= 22.0F) {
        pickup.alive = false;
        if (pickup.type == PickupType::XpGemSmall || pickup.type == PickupType::XpGemMedium || pickup.type == PickupType::XpGemLarge) {
          const int xpGain = static_cast<int>(std::llround(pickup.value * (1.0F + sim.player.growth)));
          sim.currentXp += xpGain;
          if (sim.currentXp >= sim.xpToNext) {
            sim.runLevel++;
            sim.currentXp -= sim.xpToNext;
            sim.xpToNext = runXpNeed(sim.runLevel);
            generateLevelUpChoices(sim);
          }
        } else if (pickup.type == PickupType::Ember) {
          sim.embers += static_cast<int>(std::llround(pickup.value * (1.0F + sim.player.greed)));
        }
      }
    }
  }

  // Check wave status
  sim.waveTimer += dt;
  int aliveEnemies = 0;
  for (const auto& e : sim.enemies) {
    if (e.alive) aliveEnemies++;
  }

  if (sim.currentChamberType != ChamberType::Treasure) {
    const int threshold = std::max(1, static_cast<int>(std::ceil(sim.currentWaveSpawned * 0.75F)));
    const bool wave75PctKilled = (sim.currentWaveSpawned > 0 && sim.currentWaveKilled >= threshold);

    if (sim.currentWave < sim.totalWaves) {
      if (wave75PctKilled || aliveEnemies <= 2 || sim.waveTimer >= 18.0F) {
        sim.currentWave++;
        spawnChamberWave(sim, sim.currentWave, sim.currentChamberType);
      }
    } else {
      // Final wave of the chamber: clear only when all enemies defeated or boss down
      if (aliveEnemies == 0 || sim.bossDefeated || (sim.waveTimer >= 35.0F && aliveEnemies <= 1)) {
        if (!sim.roomCleared) {
          sim.roomCleared = true;
          for (auto& p : sim.pickups) {
            p.magnetized = true;
          }
          if (!sim.isEndless && (sim.chamberIndex >= sim.totalChambers || sim.bossDefeated || sim.currentChamberType == ChamberType::Boss)) {
            sim.runCompleted = true;
            sim.doorChoicesPending = false;
          } else {
            if (sim.isEndless && (sim.currentChamberType == ChamberType::Boss || sim.bossDefeated)) {
              sim.endlessLoop++;
              sim.embers += 200 * sim.endlessLoop;
              sim.soulFragments += 2;
              sim.crownShards += 1;
              sim.player.currentHp = sim.player.maxHp;
            }
            sim.doorChoicesPending = true;
            generateChamberChoices(sim);
          }
        }
      }
    }
  }

  // Walk into in-room physical doors when room is cleared
  if (sim.roomCleared && !sim.runCompleted && sim.doorChoicesPending) {
    for (std::size_t i = 0; i < sim.doorChoices.size(); ++i) {
      const auto& d = sim.doorChoices[i];
      const float dx = std::abs(sim.playerPos.x - d.doorPos.x);
      const float dy = std::abs(sim.playerPos.y - d.doorPos.y);
      if (dx <= 40.0F && dy <= 30.0F) {
        selectChamberChoice(sim, static_cast<int>(i));
        break;
      }
    }
  }

  // Cleanup dead pickups and dead enemies to minimize memory and eliminate loop overhead
  std::erase_if(sim.pickups, [](const auto& p) { return !p.alive; });
  std::erase_if(sim.enemies, [](const auto& e) { return !e.alive; });
}

} // namespace ed::descent
