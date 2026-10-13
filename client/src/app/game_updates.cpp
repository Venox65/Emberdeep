#include "game_updates.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>

#include "game_actions.hpp"
#include "save_system.hpp"

namespace ed::app {

void updateEffects(AppState& app, float delta) {
  app.blockPulse = std::max(0.0F, app.blockPulse - delta);
  app.shake = std::max(0.0F, app.shake - delta);
  app.noticeLifetime = std::max(0.0F, app.noticeLifetime - delta);

  if (app.inDungeonCombat) {
    if (app.dungeonShowResults) {
      if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) {
        app.inDungeonCombat = false;
        app.dungeonShowResults = false;
      }
      return;
    }

    auto updateEnemyHp = [&](int id, std::int64_t hp) {
      for (auto& e : app.dungeonEnemies) {
        if (e.id == id) e.hp = hp;
      }
    };

    bool fastForward = false;
    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE)) {
      fastForward = true;
    }

    if (fastForward) {
      while (app.dungeonLogIndex < app.dungeonResult.log.size()) {
        auto& ev = app.dungeonResult.log[app.dungeonLogIndex];
        app.dungeonLogIndex++;

        switch (ev.type) {
          case ed::DungeonEventType::FloorStart:
            app.dungeonBattleLog.push_back("--- Floor " + std::to_string(ev.numData1) + " ---");
            app.dungeonEnemies.clear();
            return;
          case ed::DungeonEventType::EnemyAppear:
            app.dungeonEnemies.push_back({static_cast<int>(ev.numData3), ev.stringData, ev.numData2, ev.numData1});
            break;
          case ed::DungeonEventType::PlayerAttack:
            updateEnemyHp(static_cast<int>(ev.numData3), ev.numData2);
            break;
          case ed::DungeonEventType::EnemyAttack:
            app.dungeonPlayerHp = ev.numData2;
            break;
          case ed::DungeonEventType::EnemyAbility:
            if (ev.stringData == "heal") {
              updateEnemyHp(static_cast<int>(ev.numData3), ev.numData2);
            }
            break;
          case ed::DungeonEventType::PoisonTick:
            app.dungeonPlayerHp = ev.numData2;
            break;
          case ed::DungeonEventType::EnemyDefeated:
            updateEnemyHp(static_cast<int>(ev.numData3), 0);
            break;
          case ed::DungeonEventType::DungeonClear:
          case ed::DungeonEventType::DungeonFailed:
            app.dungeonShowResults = true;
            return;
          default:
            break;
        }
      }
    } else {
      if (app.dungeonLogIndex >= app.dungeonResult.log.size()) {
        app.dungeonShowResults = true;
      } else {
        app.dungeonCombatTimer += delta;
        if (app.dungeonCombatTimer >= 0.5F) {
          app.dungeonCombatTimer = 0.0F;
          auto& ev = app.dungeonResult.log[app.dungeonLogIndex];

          switch (ev.type) {
            case ed::DungeonEventType::FloorStart:
              app.dungeonBattleLog.push_back("--- Floor " + std::to_string(ev.numData1) + " ---");
              app.dungeonEnemies.clear();
              break;
            case ed::DungeonEventType::EnemyAppear:
              app.dungeonEnemies.push_back({static_cast<int>(ev.numData3), ev.stringData, ev.numData2, ev.numData1});
              app.dungeonBattleLog.push_back("A wild " + ev.stringData + " appears!");
              break;
            case ed::DungeonEventType::PlayerAttack:
              updateEnemyHp(static_cast<int>(ev.numData3), ev.numData2);
              if (ev.flag) {
                app.dungeonBattleLog.push_back("CRITICAL! You hit " + ev.stringData + " for " + std::to_string(ev.numData1) + "!");
                app.shake = 0.3F;
              } else {
                app.dungeonBattleLog.push_back("You hit " + ev.stringData + " for " + std::to_string(ev.numData1) + ".");
              }
              break;
            case ed::DungeonEventType::EnemyAttack:
              app.dungeonPlayerHp = ev.numData2;
              if (ev.flag) {
                app.dungeonBattleLog.push_back("Frenzied " + ev.stringData + " hits you for " + std::to_string(ev.numData1) + "!");
                app.shake = 0.2F;
              } else {
                app.dungeonBattleLog.push_back(ev.stringData + " hits you for " + std::to_string(ev.numData1) + ".");
              }
              break;
            case ed::DungeonEventType::EnemyAbility:
              if (ev.stringData == "heal") {
                updateEnemyHp(static_cast<int>(ev.numData3), ev.numData2);
                app.dungeonBattleLog.push_back("Enemy heals for " + std::to_string(ev.numData1) + ".");
              } else if (ev.stringData == "poison") {
                app.dungeonBattleLog.push_back("Enemy applies Poison!");
              } else if (ev.stringData == "guard") {
                app.dungeonBattleLog.push_back("Enemy is Guarding!");
              } else if (ev.stringData == "frenzy") {
                app.dungeonBattleLog.push_back("Enemy goes into a Frenzy!");
              }
              break;
            case ed::DungeonEventType::PoisonTick:
              app.dungeonPlayerHp = ev.numData2;
              app.dungeonBattleLog.push_back("Poison deals " + std::to_string(ev.numData1) + " damage.");
              break;
            case ed::DungeonEventType::PlayerDefeated:
              app.dungeonBattleLog.push_back("You were defeated...");
              break;
            case ed::DungeonEventType::EnemyDefeated:
              updateEnemyHp(static_cast<int>(ev.numData3), 0);
              app.dungeonBattleLog.push_back(ev.stringData + " is defeated! +" + std::to_string(ev.numData1) + " Gold");
              break;
            case ed::DungeonEventType::DungeonClear:
              app.dungeonShowResults = true;
              app.dungeonBattleLog.push_back("Dungeon Cleared! Total: " + std::to_string(ev.numData1) + "G, " + std::to_string(ev.numData2) + "XP.");
              if (ev.flag) app.dungeonBattleLog.push_back("Found a Gem!");
              break;
            case ed::DungeonEventType::DungeonFailed:
              app.dungeonShowResults = true;
              app.dungeonBattleLog.push_back("Failed on floor " + std::to_string(ev.numData1) + ".");
              break;
          }
          app.dungeonLogIndex++;
        }
      }
    }
  }

  if (app.screen == Screen::Campaign && app.campaignAttempt) {
    const auto challenge = app.bosses[static_cast<std::size_t>(app.selectedBoss)].challenge;
    if (challenge == ed::BossChallenge::FrenzyDig) {
      ed::updateFrenzyDig(app.frenzyDig, delta, app.rng);
      if (app.frenzyDig.finished) {
        const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
        app.campaignScore = ed::frenzyDigScore(app.frenzyDig, stats.attack);
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::SoulDodge) {
      const float moveX = (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) -
                          (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A));
      const float moveY = (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) -
                          (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W));
      ed::updateSoulDodge(app.soulDodge, delta, moveX, moveY,
                          IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT), 10);
      if (app.soulDodge.finished) {
        app.campaignScore = ed::soulDodgeScore(app.soulDodge);
        app.campaignHeartsLost = app.soulDodge.hits;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::FiveTrials) {
      if (app.fiveTrials.phase == 1) {
        app.fiveTrials.burstElapsed += delta;
        if (app.fiveTrials.burstElapsed >= 6.0F) {
          const bool passed = app.fiveTrials.burstTaps >= 48;
          ed::advanceFiveTrials(app.fiveTrials, passed, passed ? 0 : 1);
          if (app.fiveTrials.phase == 2) {
            app.cinderDance = ed::startCinderDance(0xF17EULL, 30.0F, 36);
            app.cinderDance.gauntletMode = true;
          }
        }
      } else if (app.fiveTrials.phase == 2) {
        app.fiveTrials.phaseElapsed += delta;
        ed::updateCinderDance(app.cinderDance, delta);
        if (app.fiveTrials.phaseElapsed >= 30.0F) {
          app.cinderDance.finished = true;
          app.cinderDance.failed = app.cinderDance.groove == 0;
        }
        if (app.cinderDance.finished) {
          const bool passed = !app.cinderDance.failed;
          ed::advanceFiveTrials(app.fiveTrials, passed, passed ? 0 : 1);
          if (app.fiveTrials.phase == 3) {
            app.parryDuel = ed::startParryDuel(0xF17EULL, 12);
            app.parryDuel.elapsed = -0.75F;
          }
        }
      } else if (app.fiveTrials.phase == 3) {
        ed::updateParryDuel(app.parryDuel, delta, app.rng);
        if (app.parryDuel.finished) {
          ed::advanceFiveTrials(app.fiveTrials, app.parryDuel.cleared,
                                app.parryDuel.heartsLost);
          if (app.fiveTrials.phase == 4) {
            app.soulDodge = ed::startSoulDodge(0xF17EULL, 0, 40.0F);
            app.soulDodge.invulnerable = 1.0F;
          }
        }
      } else if (app.fiveTrials.phase == 4) {
        const float moveX = (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) -
                            (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A));
        const float moveY = (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) -
                            (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W));
        ed::updateSoulDodge(app.soulDodge, delta, moveX, moveY,
                            IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT), 10);
        if (app.soulDodge.finished) {
          ed::advanceFiveTrials(app.fiveTrials, app.soulDodge.cleared,
                                app.soulDodge.hits);
          if (app.fiveTrials.phase == 5) app.crownStrike = ed::startCrownStrike();
        }
      } else {
        ed::updateCrownStrike(app.crownStrike, delta);
        if (app.crownStrike.finished) {
          ed::advanceFiveTrials(app.fiveTrials, app.crownStrike.cleared,
                                app.crownStrike.misses);
        }
      }
      if (app.fiveTrials.finished) {
        app.campaignScore = ed::fiveTrialsScore(app.fiveTrials);
        app.campaignHeartsLost = app.fiveTrials.heartsLost;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::StarTrace) {
      ed::updateStarTrace(app.starTrace, delta, app.rng);
      if (app.starTrace.finished) {
        const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
        app.campaignScore = ed::starTraceScore(app.starTrace, stats.speed);
        app.campaignHeartsLost = app.starTrace.heartsLost;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::Heartpierce) {
      ed::updateHeartpierce(app.heartpierce, delta,
                            IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_BUTTON_LEFT));
      if (app.heartpierce.finished) {
        app.campaignScore = ed::heartpierceScore(app.heartpierce);
        app.campaignHeartsLost = app.heartpierce.heartsLost;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::BoneDuel) {
      ed::updateBoneDuel(app.boneDuel, delta);
      if (app.boneDuel.finished) {
        const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
        app.campaignScore = ed::boneDuelScore(app.boneDuel, stats.attack, stats.pierce);
        app.campaignHeartsLost = app.boneDuel.misses;
        if (app.boneDuel.misses >= 3) app.campaignScore = 0;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::CinderDance) {
      ed::updateCinderDance(app.cinderDance, delta);
      if (app.cinderDance.finished) {
        const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
        app.campaignScore = ed::cinderDanceScore(app.cinderDance, stats.attack) / 100;
        app.campaignHeartsLost = app.cinderDance.failed ? 3 : 0;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::HeadHunt) {
      const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
      ed::updateHeadHunt(app.headHunt, delta, app.rng,
                         static_cast<double>(stats.speed) / 100.0);
      if (app.headHunt.finished) {
        app.campaignScore = ed::headHuntScore(app.headHunt, stats.attack);
        app.campaignHeartsLost = app.headHunt.heartsLost;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::ParryDuel) {
      ed::updateParryDuel(app.parryDuel, delta, app.rng);
      if (app.parryDuel.finished) {
        const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
        app.campaignScore = ed::parryDuelScore(app.parryDuel, stats.attack);
        app.campaignHeartsLost = app.parryDuel.heartsLost;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::RuneRecall) {
      ed::updateRuneRecall(app.runeRecall, delta, app.rng);
      if (app.runeRecall.finished) {
        const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
        app.campaignScore = ed::runeRecallScore(app.runeRecall, stats.focusBasisPoints / 100);
        app.campaignHeartsLost = app.runeRecall.mistakes;
        attemptCampaign(app);
      }
    }
  }

  if (app.game.dirty &&
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::steady_clock::now() - app.lastAutosave)
              .count() >= 10) {
    requestSave(app);
    app.lastAutosave = std::chrono::steady_clock::now();
  }

  for (auto& text : app.floatingText) {
    text.lifetime -= delta;
    text.position.y -= 28.0F * delta;
  }
  app.floatingText.erase(
      std::remove_if(app.floatingText.begin(), app.floatingText.end(),
                     [](const FloatingText& text) { return text.lifetime <= 0.0F; }),
      app.floatingText.end());

  for (auto& particle : app.particles) {
    particle.lifetime -= delta;
    particle.position.x += particle.velocity.x * delta;
    particle.position.y += particle.velocity.y * delta;
    particle.velocity.y += 160.0F * delta;
  }
  app.particles.erase(
      std::remove_if(app.particles.begin(), app.particles.end(),
                     [](const Particle& particle) { return particle.lifetime <= 0.0F; }),
      app.particles.end());
}

} // namespace ed::app
