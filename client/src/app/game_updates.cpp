#include "game_updates.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>

#include "game_actions.hpp"
#include "save_system.hpp"

namespace ed::app {

void updateEffects(AppState& app, float delta) {
  if (app.screen == Screen::Loading) {
    app.loadingTimer += delta;
    if (app.loadingTimer >= 1.0F) {
      app.screen = Screen::Title;
    }
    return;
  }
  if (app.screen == Screen::Title) {
    return;
  }
  if (app.screen == Screen::DescentRun) {
    app.noticeLifetime = std::max(0.0F, app.noticeLifetime - delta);
    if (app.descentPopupTimer > 0.0F) {
      app.descentPopupTimer = std::max(0.0F, app.descentPopupTimer - delta);
    }

    if (app.descentMinigameActive) {
      if (app.descentMinigameBossIdx >= 0 && static_cast<std::size_t>(app.descentMinigameBossIdx) < app.bosses.size()) {
        const auto challenge = app.bosses[static_cast<std::size_t>(app.descentMinigameBossIdx)].challenge;
        if (challenge == ed::BossChallenge::FrenzyDig) {
          ed::updateFrenzyDig(app.frenzyDig, delta, app.rng);
          if (app.frenzyDig.finished) {
            const bool win = (app.frenzyDig.taps + app.frenzyDig.nuggetTaps * 5 >= 25);
            finishDescentVaultMinigame(app, win);
          }
        } else if (challenge == ed::BossChallenge::BoneDuel) {
          ed::updateBoneDuel(app.boneDuel, delta);
          if (app.boneDuel.finished) {
            const bool win = (app.boneDuel.cleared || app.boneDuel.perfectHits >= 3);
            finishDescentVaultMinigame(app, win);
          }
        } else if (challenge == ed::BossChallenge::CinderDance) {
          ed::updateCinderDance(app.cinderDance, delta);
          if (app.cinderDance.finished) {
            const bool win = (!app.cinderDance.failed && app.cinderDance.groove >= 20);
            finishDescentVaultMinigame(app, win);
          }
        } else if (challenge == ed::BossChallenge::HeadHunt) {
          ed::updateHeadHunt(app.headHunt, delta, app.rng, 1.0);
          if (app.headHunt.finished) {
            const bool win = (app.headHunt.realHits >= 6 && app.headHunt.heartsLost <= 3);
            finishDescentVaultMinigame(app, win);
          }
        } else if (challenge == ed::BossChallenge::ParryDuel) {
          ed::updateParryDuel(app.parryDuel, delta, app.rng);
          if (app.parryDuel.finished) {
            const bool win = (app.parryDuel.cleared || app.parryDuel.successfulParries >= 6);
            finishDescentVaultMinigame(app, win);
          }
        } else if (challenge == ed::BossChallenge::RuneRecall) {
          ed::updateRuneRecall(app.runeRecall, delta, app.rng);
          if (app.runeRecall.finished) {
            const bool win = (app.runeRecall.cleared || app.runeRecall.round >= 3);
            finishDescentVaultMinigame(app, win);
          }
        } else if (challenge == ed::BossChallenge::StarTrace) {
          ed::updateStarTrace(app.starTrace, delta, app.rng);
          if (app.starTrace.finished) {
            const bool win = (app.starTrace.cleared || app.starTrace.constellation >= 2);
            finishDescentVaultMinigame(app, win);
          }
        } else if (challenge == ed::BossChallenge::Heartpierce) {
          const bool thrust = IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_BUTTON_LEFT);
          ed::updateHeartpierce(app.heartpierce, delta, thrust);
          if (app.heartpierce.finished) {
            const bool win = (app.heartpierce.cleared || app.heartpierce.overlapTime >= 5.0F);
            finishDescentVaultMinigame(app, win);
          }
        } else if (challenge == ed::BossChallenge::SoulDodge) {
          const float moveX = (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) - (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A));
          const float moveY = (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) - (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W));
          ed::updateSoulDodge(app.soulDodge, delta, moveX, moveY,
                              IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT), 6);
          if (app.soulDodge.finished) {
            const bool win = (app.soulDodge.hits <= 5);
            finishDescentVaultMinigame(app, win);
          }
        } else {
          finishDescentVaultMinigame(app, true);
        }
      }
      return;
    }

    if (!app.descentPaused && !app.descentSim.levelUpPending && !app.descentSim.runCompleted && app.descentSim.player.currentHp > 0.0F) {
      ed::descent::DescentInput input;
      if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) input.moveVector.y -= 1.0F;
      if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) input.moveVector.y += 1.0F;
      if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) input.moveVector.x -= 1.0F;
      if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) input.moveVector.x += 1.0F;

      const Vector2 mouseScreen = GetMousePosition();
      const Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, app.descentCamera);
      input.aimPosition = {mouseWorld.x, mouseWorld.y};

      input.dashPressed = IsKeyPressed(KEY_SPACE);
      input.skillPressed = IsKeyPressed(KEY_Q) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
      input.ultimatePressed = IsKeyPressed(KEY_R);
      input.interactPressed = IsKeyPressed(KEY_E);

      app.descentAccumulator += std::min(delta, 0.1F);
      while (app.descentAccumulator >= (1.0F / 60.0F)) {
        const int prevChamber = app.descentSim.chamberIndex;
        ed::descent::stepDescentSim(app.descentSim, input, 1.0F / 60.0F);
        if (app.descentSim.chamberIndex != prevChamber) {
          app.descentChestOpened = false;
          app.descentMinigameActive = false;
          app.descentPopupTimer = 0.0F;
        }
        if (app.descentSim.isEndless) {
          if (app.descentSim.chamberIndex > app.game.player.descentEndlessBestChamber) {
            app.game.player.descentEndlessBestChamber = app.descentSim.chamberIndex;
            app.game.dirty = true;
          }
          if (app.descentSim.chamberIndex > app.descentMeta.bestEndlessChamber) {
            app.descentMeta.bestEndlessChamber = app.descentSim.chamberIndex;
          }
        } else {
          if (app.descentSim.chamberIndex > app.game.player.descentBestChamber) {
            app.game.player.descentBestChamber = app.descentSim.chamberIndex;
            app.game.dirty = true;
          }
        }
        if (app.descentSim.runLevel > app.game.player.descentMaxRunLevel) {
          app.game.player.descentMaxRunLevel = app.descentSim.runLevel;
          app.game.dirty = true;
        }
        app.descentAccumulator -= (1.0F / 60.0F);
        input.dashPressed = false;
        input.skillPressed = false;
        input.ultimatePressed = false;
      }
    }
    return;
  }

  app.blockPulse = std::max(0.0F, app.blockPulse - delta);
  app.shake = std::max(0.0F, app.shake - delta);
  app.noticeLifetime = std::max(0.0F, app.noticeLifetime - delta);

  if (app.activeMiningTimer > 0.0F) {
    app.activeMiningTimer = std::max(0.0F, app.activeMiningTimer - delta);
  }

  // Auto-Clicker (Chrono Automaton) - disabled while player is actively mining (30s grace window)
  const bool isPlayerActivelyMining = (app.activeMiningTimer > 0.0F);
  if (app.game.player.autoClickerLevel >= 1 && app.game.player.autoClickerEnabled && !isPlayerActivelyMining) {
    const float speed = static_cast<float>(ed::autoClickerSpeed(app.game.player.autoClickerLevel));
    if (speed > 0.0F) {
      const float interval = 1.0F / speed;
      app.autoClickTimer += delta;
      if (app.autoClickTimer >= interval) {
        app.autoClickTimer = 0.0F;
        mine(app, {640.0F, 335.0F}, /*isAutoMine=*/true);
      }
    }
  } else if (isPlayerActivelyMining) {
    app.autoClickTimer = 0.0F;
  }

  if (app.screen == Screen::Mine) {
    if (app.weakSpotActive) {
      app.weakSpotTimer -= delta;
      if (app.weakSpotTimer <= 0.0F) {
        app.weakSpotActive = false;
        const float cdMult = std::max(0.2F, 1.0F - app.game.player.vows.deepBreath * 0.05F);
        app.weakSpotCooldown = (4.0F + static_cast<float>(app.rng.next() % 4000) / 1000.0F) * cdMult;
      }
    } else {
      app.weakSpotCooldown -= delta;
      if (app.weakSpotCooldown <= 0.0F) {
        app.weakSpotActive = true;
        app.weakSpotTimer = 1.5F;
        const float offsetX = -70.0F + static_cast<float>(app.rng.next() % 141);
        const float offsetY = -70.0F + static_cast<float>(app.rng.next() % 141);
        app.weakSpotPos = {640.0F + offsetX, 335.0F + offsetY};
      }
    }
  }

  app.achievementNotificationTimer = std::max(0.0F, app.achievementNotificationTimer - delta);
  app.achievementCheckTimer += delta;
  if (app.achievementCheckTimer >= 0.5F) {
    app.achievementCheckTimer = 0.0F;
    const auto newlyUnlocked = ed::evaluateNewAchievements(app.game, app.inventory, app.campaign);
    for (const auto& achId : newlyUnlocked) {
      app.game.player.achievements.push_back(achId);
      app.game.dirty = true;
      std::string title = achId;
      for (const auto& a : ed::allAchievements()) {
        if (a.id == achId) {
          title = a.title;
          break;
        }
      }
      app.achievementNotification = "Achievement Unlocked: " + title + "!";
      app.achievementNotificationTimer = 4.0F;
    }
  }

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

  if (app.inBossCombat) {
    if (app.bossCombatShowResults) {
      if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER)) {
        app.inBossCombat = false;
        app.bossCombatShowResults = false;
      }
      return;
    }

    const float stepDuration = (IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_ENTER)) ? 0.06F : 0.32F;
    app.bossCombatTimer += delta;
    if (app.bossCombatTimer >= stepDuration) {
      app.bossCombatTimer = 0.0F;
      if (app.bossCombatLogIndex < app.bossCombatResult.events.size()) {
        const auto& ev = app.bossCombatResult.events[app.bossCombatLogIndex++];
        app.bossCombatPlayerHp = ev.playerHp;
        app.bossCombatBossHp = ev.bossHp;
        app.bossBattleLog.push_back(ev.description);

        if (ev.isCrit) {
          app.shake = 0.3F;
        } else if (ev.type == ed::BossCombatEventType::BossAbility) {
          app.shake = 0.25F;
        }

        if (app.bossCombatLogIndex >= app.bossCombatResult.events.size()) {
          app.bossCombatShowResults = true;
          if (!app.bossCombatRewardsClaimed) {
            app.bossCombatRewardsClaimed = true;
            if (app.bossCombatResult.victory) {
              const auto index = static_cast<std::size_t>(app.bossCombatBossIndex);
              if (index < app.bosses.size()) {
                const auto& boss = app.bosses[index];
                if (app.bossCombatIsEcho) {
                  const int oldRank = app.game.player.echoRank;
                  ed::recordEchoBossClear(app.game, static_cast<int>(index));
                  const int goldReward = ed::echoBossRewardGold(boss.firstClearGold, std::max(1, oldRank));
                  const int sigilReward = ed::echoBossRewardSigils(std::max(1, oldRank));
                  app.bossCombatRewardGold = goldReward;
                  app.bossCombatRewardSigils = sigilReward;
                  app.bossCombatRewardXp = 0;
                  app.bossCombatRewardLegendary.clear();
                  app.game.player.gold = static_cast<int>(std::min<std::int64_t>(
                      std::numeric_limits<int>::max(),
                      static_cast<std::int64_t>(app.game.player.gold) + goldReward));
                  app.game.player.goldEarnedThisRun = static_cast<int>(std::min<std::int64_t>(
                      std::numeric_limits<int>::max(),
                      static_cast<std::int64_t>(app.game.player.goldEarnedThisRun) + goldReward));
                  app.game.player.echoSigils += sigilReward;
                  app.game.dirty = true;
                } else {
                  const auto outcome = ed::evaluateBossAttempt(
                      app.game, app.campaign, app.bosses, index,
                      {app.campaignScore, app.campaignHeartsLost, true, 0.0});
                  ed::applyBossOutcome(app.game, outcome);
                  if (outcome.firstClear && !outcome.legendaryId.empty()) {
                    auto piece = ed::createLegendaryGear(outcome.legendaryId);
                    app.inventory.gear.push_back(piece);
                    app.game.dirty = true;
                  }
                  app.bossCombatRewardGold = outcome.goldReward;
                  app.bossCombatRewardXp = outcome.xpReward;
                  app.bossCombatRewardSigils = 0;
                  app.bossCombatRewardLegendary = outcome.legendaryId;
                }
              }
            }
          }
        }
      }
    }
  }

  if (app.screen == Screen::Campaign && app.campaignAttempt) {
    const float minigameDelta = (app.echoCampaignMode)
        ? delta * static_cast<float>(ed::echoDifficultyMultiplier(std::max(1, app.game.player.echoRank)))
        : delta;
    const auto challenge = app.bosses[static_cast<std::size_t>(app.selectedBoss)].challenge;
    if (challenge == ed::BossChallenge::FrenzyDig) {
      ed::updateFrenzyDig(app.frenzyDig, minigameDelta, app.rng);
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
      ed::updateSoulDodge(app.soulDodge, minigameDelta, moveX, moveY,
                          IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT), 10);
      if (app.soulDodge.finished) {
        app.campaignScore = ed::soulDodgeScore(app.soulDodge);
        app.campaignHeartsLost = app.soulDodge.hits;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::FiveTrials) {
      if (app.fiveTrials.phase == 1) {
        app.fiveTrials.burstElapsed += minigameDelta;
        if (app.fiveTrials.burstElapsed >= 6.0F) {
          const bool passed = app.fiveTrials.burstTaps >= 48;
          ed::advanceFiveTrials(app.fiveTrials, passed, passed ? 0 : 1);
          if (app.fiveTrials.phase == 2) {
            app.cinderDance = ed::startCinderDance(0xF17EULL, 30.0F, 36);
            app.cinderDance.gauntletMode = true;
          }
        }
      } else if (app.fiveTrials.phase == 2) {
        app.fiveTrials.phaseElapsed += minigameDelta;
        ed::updateCinderDance(app.cinderDance, minigameDelta);
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
        ed::updateParryDuel(app.parryDuel, minigameDelta, app.rng);
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
        ed::updateSoulDodge(app.soulDodge, minigameDelta, moveX, moveY,
                            IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT), 10);
        if (app.soulDodge.finished) {
          ed::advanceFiveTrials(app.fiveTrials, app.soulDodge.cleared,
                                app.soulDodge.hits);
          if (app.fiveTrials.phase == 5) app.crownStrike = ed::startCrownStrike();
        }
      } else {
        ed::updateCrownStrike(app.crownStrike, minigameDelta);
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
      ed::updateStarTrace(app.starTrace, minigameDelta, app.rng);
      if (app.starTrace.finished) {
        const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
        app.campaignScore = ed::starTraceScore(app.starTrace, stats.speed);
        app.campaignHeartsLost = app.starTrace.heartsLost;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::Heartpierce) {
      ed::updateHeartpierce(app.heartpierce, minigameDelta,
                            IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_BUTTON_LEFT));
      if (app.heartpierce.finished) {
        app.campaignScore = ed::heartpierceScore(app.heartpierce);
        app.campaignHeartsLost = app.heartpierce.heartsLost;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::BoneDuel) {
      ed::updateBoneDuel(app.boneDuel, minigameDelta);
      if (app.boneDuel.finished) {
        const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
        app.campaignScore = ed::boneDuelScore(app.boneDuel, stats.attack, stats.pierce);
        app.campaignHeartsLost = app.boneDuel.misses;
        if (app.boneDuel.misses >= 3) app.campaignScore = 0;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::CinderDance) {
      ed::updateCinderDance(app.cinderDance, minigameDelta);
      if (app.cinderDance.finished) {
        const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
        app.campaignScore = ed::cinderDanceScore(app.cinderDance, stats.attack) / 100;
        app.campaignHeartsLost = app.cinderDance.failed ? 3 : 0;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::HeadHunt) {
      const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
      ed::updateHeadHunt(app.headHunt, minigameDelta, app.rng,
                         static_cast<double>(stats.speed) / 100.0);
      if (app.headHunt.finished) {
        app.campaignScore = ed::headHuntScore(app.headHunt, stats.attack);
        app.campaignHeartsLost = app.headHunt.heartsLost;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::ParryDuel) {
      ed::updateParryDuel(app.parryDuel, minigameDelta, app.rng);
      if (app.parryDuel.finished) {
        const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
        app.campaignScore = ed::parryDuelScore(app.parryDuel, stats.attack);
        app.campaignHeartsLost = app.parryDuel.heartsLost;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::RuneRecall) {
      ed::updateRuneRecall(app.runeRecall, minigameDelta, app.rng);
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
  std::erase_if(app.floatingText, [](const FloatingText& text) { return text.lifetime <= 0.0F; });

  for (auto& particle : app.particles) {
    particle.lifetime -= delta;
    particle.position.x += particle.velocity.x * delta;
    particle.position.y += particle.velocity.y * delta;
    particle.velocity.y += 160.0F * delta;
  }
  std::erase_if(app.particles, [](const Particle& particle) { return particle.lifetime <= 0.0F; });
}

} // namespace ed::app
