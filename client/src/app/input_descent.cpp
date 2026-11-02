#include "input_handler.hpp"

#include <algorithm>
#include <string>

#include "game_actions.hpp"
#include "save_system.hpp"
#include "campaign_minigames.hpp"
#include "ed/descent/combat_sim.hpp"

namespace ed::app {

void handleDescentInput(AppState& app, Vector2 mouse, bool leftClick) {
  if (app.screen == Screen::DescentHub) {
    const Vector2 mouse = GetMousePosition();

    // Begin Run button
    const Rectangle beginRunBtn{80, 155, 280, 52};
    if ((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, beginRunBtn)) || IsKeyPressed(KEY_ENTER)) {
      app.game.player.descentRuns++;
      if (app.descentEndlessMode) {
        app.game.player.descentEndlessBestChamber = std::max(app.game.player.descentEndlessBestChamber, 1);
        app.descentMeta.bestEndlessChamber = std::max(app.descentMeta.bestEndlessChamber, 1);
      } else {
        app.game.player.descentBestChamber = std::max(app.game.player.descentBestChamber, 1);
      }
      app.game.player.descentMaxRunLevel = std::max(app.game.player.descentMaxRunLevel, 1);
      app.game.dirty = true;
      app.descentSim.isEndless = app.descentEndlessMode;
      app.descentSim.endlessLoop = 1;
      ed::descent::applyMetaToPlayer(app.descentSim, app.descentMeta);
      ed::descent::initRoom(app.descentSim, 56, 32);
      app.descentPaused = false;
      app.screen = Screen::DescentRun;
      setNotice(app, app.descentEndlessMode
        ? "Entered Endless Catacombs! Move with WASD, Dash with SPACE, Skill with Q, Ult with R."
        : "Entered The Hollow Descent! Move with WASD, Dash with SPACE, Skill with Q, Ult with R.");
      return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      // Expedition Mode Selection Cards
      const Rectangle stdCard{915, 430, 290, 106};
      const Rectangle endlessCard{915, 545, 290, 112};
      if (CheckCollisionPointRec(mouse, stdCard)) {
        app.descentEndlessMode = false;
        setNotice(app, "Mode selected: Standard Descent (20-40 Chambers)");
        return;
      }
      if (CheckCollisionPointRec(mouse, endlessCard)) {
        app.descentEndlessMode = true;
        setNotice(app, "Mode selected: Endless Catacombs (Infinite Waves & Boss Cycles)");
        return;
      }

      // Tab Buttons (5 tabs)
      for (int i = 0; i < 5; ++i) {
        const Rectangle tabBtn{80, static_cast<float>(222 + i * 50), 280, 42};
        if (CheckCollisionPointRec(mouse, tabBtn)) {
          if (i == 0) app.descentHubTab = DescentHubTab::Main;
          else if (i == 1) app.descentHubTab = DescentHubTab::Anvil;
          else if (i == 2) app.descentHubTab = DescentHubTab::Preparations;
          else if (i == 3) app.descentHubTab = DescentHubTab::Codex;
          else if (i == 4) app.descentHubTab = DescentHubTab::Pacts;
          return;
        }
      }

      // Back to Camp
      if (CheckCollisionPointRec(mouse, {80, 595, 280, 46})) {
        app.screen = Screen::Mine;
        setNotice(app, "Returned to Camp.");
        return;
      }

      // Inside Main Tab: Class Carousel
      if (app.descentHubTab == DescentHubTab::Main) {
        if (CheckCollisionPointRec(mouse, {415, 150, 36, 36})) {
          // Prev class
          int c = static_cast<int>(app.descentMeta.selectedClass);
          c = (c + 4) % 5;
          app.descentMeta.selectedClass = static_cast<ed::descent::ClassId>(c);
          saveDescentMeta(app.savePath, app.descentMeta);
        } else if (CheckCollisionPointRec(mouse, {830, 150, 36, 36})) {
          // Next class
          int c = static_cast<int>(app.descentMeta.selectedClass);
          c = (c + 1) % 5;
          app.descentMeta.selectedClass = static_cast<ed::descent::ClassId>(c);
          saveDescentMeta(app.savePath, app.descentMeta);
        } else if (CheckCollisionPointRec(mouse, {520, 605, 240, 42})) {
          // Ready / Begin
          app.game.player.descentRuns++;
          if (app.descentEndlessMode) {
            app.game.player.descentEndlessBestChamber = std::max(app.game.player.descentEndlessBestChamber, 1);
            app.descentMeta.bestEndlessChamber = std::max(app.descentMeta.bestEndlessChamber, 1);
          } else {
            app.game.player.descentBestChamber = std::max(app.game.player.descentBestChamber, 1);
          }
          app.game.player.descentMaxRunLevel = std::max(app.game.player.descentMaxRunLevel, 1);
          app.game.dirty = true;
          app.descentSim.isEndless = app.descentEndlessMode;
          app.descentSim.endlessLoop = 1;
          ed::descent::applyMetaToPlayer(app.descentSim, app.descentMeta);
          ed::descent::initRoom(app.descentSim, 56, 32);
          app.descentPaused = false;
          app.screen = Screen::DescentRun;
          setNotice(app, app.descentEndlessMode
            ? "Entered Endless Catacombs! Move with WASD, Dash with SPACE, Skill with Q, Ult with R."
            : "Entered The Hollow Descent! Move with WASD, Dash with SPACE.");
          return;
        }
      }

      // Inside Anvil Tab: Upgrades & Respec
      else if (app.descentHubTab == DescentHubTab::Anvil) {
        struct UpgradeDef {
          int* rank;
          int maxRank;
          int baseCost;
          int stepCost;
        };
        UpgradeDef uDefs[] = {
          {&app.descentMeta.anvilMight, 10, 20, 15},
          {&app.descentMeta.anvilVitality, 10, 20, 15},
          {&app.descentMeta.anvilSwiftness, 5, 25, 20},
          {&app.descentMeta.anvilMagnetism, 5, 15, 10},
          {&app.descentMeta.anvilResilience, 5, 30, 25},
          {&app.descentMeta.anvilGreed, 5, 25, 20},
          {&app.descentMeta.anvilRerolls, 3, 50, 50},
          {&app.descentMeta.anvilBanishes, 3, 50, 50}
        };

        for (int i = 0; i < 8; ++i) {
          const float x = static_cast<float>(415 + (i % 2) * 225);
          const float y = static_cast<float>(205 + (i / 2) * 88);
          const Rectangle bBtn{x + 120, y + 42, 85, 26};
          if (CheckCollisionPointRec(mouse, bBtn)) {
            const int cost = uDefs[i].baseCost + (*uDefs[i].rank) * uDefs[i].stepCost;
            if (*uDefs[i].rank < uDefs[i].maxRank && app.descentMeta.embers >= cost) {
              app.descentMeta.embers -= cost;
              (*uDefs[i].rank)++;
              saveDescentMeta(app.savePath, app.descentMeta);
              setNotice(app, "Anvil upgrade forged!");
            }
          }
        }

        // Respec All Button
        if (CheckCollisionPointRec(mouse, {500, 580, 280, 36})) {
          int refund = 0;
          for (int i = 0; i < 8; ++i) {
            for (int r = 0; r < *uDefs[i].rank; ++r) {
              refund += uDefs[i].baseCost + r * uDefs[i].stepCost;
            }
            *uDefs[i].rank = 0;
          }
          app.descentMeta.embers += refund;
          saveDescentMeta(app.savePath, app.descentMeta);
          setNotice(app, "Respec complete! Refunded " + std::to_string(refund) + " Embers.");
        }
      }

      // Inside Preparations Tab: Provisions toggles
      else if (app.descentHubTab == DescentHubTab::Preparations) {
        if (CheckCollisionPointRec(mouse, {425, 238, 430, 46})) {
          app.descentMeta.provisionStew = !app.descentMeta.provisionStew;
          saveDescentMeta(app.savePath, app.descentMeta);
        } else if (CheckCollisionPointRec(mouse, {425, 292, 430, 46})) {
          app.descentMeta.provisionBrew = !app.descentMeta.provisionBrew;
          saveDescentMeta(app.savePath, app.descentMeta);
        } else if (CheckCollisionPointRec(mouse, {425, 346, 430, 46})) {
          app.descentMeta.provisionOil = !app.descentMeta.provisionOil;
          saveDescentMeta(app.savePath, app.descentMeta);
        }
      }

      // Inside Pacts Tab: Calamity ranks
      else if (app.descentHubTab == DescentHubTab::Pacts) {
        int* pactRanks[] = {
          &app.descentMeta.pactClaws,
          &app.descentMeta.pactFrenzy,
          &app.descentMeta.pactFlesh,
          &app.descentMeta.pactShadows,
          &app.descentMeta.pactNumbers,
          &app.descentMeta.pactWounds
        };
        const int maxRanks[] = {3, 3, 3, 3, 3, 1};

        for (int i = 0; i < 6; ++i) {
          const float y = static_cast<float>(210 + i * 65);
          // Minus
          if (CheckCollisionPointRec(mouse, {750, y + 14, 28, 28})) {
            if (*pactRanks[i] > 0) {
              (*pactRanks[i])--;
              saveDescentMeta(app.savePath, app.descentMeta);
            }
          }
          // Plus
          else if (CheckCollisionPointRec(mouse, {820, y + 14, 28, 28})) {
            if (*pactRanks[i] < maxRanks[i]) {
              (*pactRanks[i])++;
              saveDescentMeta(app.savePath, app.descentMeta);
            }
          }
        }
      }
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
      if (app.descentHubTab != DescentHubTab::Main) {
        app.descentHubTab = DescentHubTab::Main;
      } else {
        app.screen = Screen::Mine;
        setNotice(app, "Returned to Camp.");
      }
      return;
    }
  } else if (app.screen == Screen::DescentRun) {
    const Vector2 mouse = GetMousePosition();

    // Pause Screen Modal (Highest priority when paused)
    if (app.descentPaused) {
      if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_SPACE)) {
        app.descentPaused = false;
        return;
      }
      const Rectangle resumeBtn{360, 605, 260, 46};
      const Rectangle abandonBtn{660, 605, 260, 46};
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(mouse, resumeBtn)) {
          app.descentPaused = false;
          return;
        }
        if (CheckCollisionPointRec(mouse, abandonBtn)) {
          if (app.descentSim.isEndless) {
            const int souls = std::max(1, app.descentSim.chamberIndex / 5);
            const int crowns = std::max(0, app.descentSim.endlessLoop - 1);
            app.descentMeta.embers += app.descentSim.embers;
            app.descentMeta.soulFragments += souls;
            app.descentMeta.crownShards += crowns;
            app.descentMeta.lifetimeKills += app.descentSim.enemiesKilled;
            app.descentMeta.lifetimeRoomsCleared += app.descentSim.chamberIndex;
            if (app.descentSim.chamberIndex > app.descentMeta.bestEndlessChamber) {
              app.descentMeta.bestEndlessChamber = app.descentSim.chamberIndex;
            }
            if (app.descentSim.chamberIndex > app.game.player.descentEndlessBestChamber) {
              app.game.player.descentEndlessBestChamber = app.descentSim.chamberIndex;
            }
            if (app.descentSim.runLevel > app.game.player.descentMaxRunLevel) {
              app.game.player.descentMaxRunLevel = app.descentSim.runLevel;
            }
            app.game.dirty = true;
            saveDescentMeta(app.savePath, app.descentMeta);
            setNotice(app, "Retired from Endless Catacombs! Chamber " + std::to_string(app.descentSim.chamberIndex) +
                           " reached. Secured " + std::to_string(app.descentSim.embers) + " Embers and " + std::to_string(souls) + " Soul Fragments.");
          } else {
            setNotice(app, "Run abandoned. Returned to Descent Hub.");
          }
          app.descentPaused = false;
          app.screen = Screen::DescentHub;
          return;
        }
      }
      return;
    }

    // Toggle pause on ESC (during active run)
    if (IsKeyPressed(KEY_ESCAPE)) {
      if (!app.descentSim.runCompleted && app.descentSim.player.currentHp > 0.0F) {
        app.descentPaused = true;
        return;
      }
    }

    // Treasure Vault Chest Interaction
    if (app.descentSim.currentChamberType == ed::descent::ChamberType::Treasure && !app.descentChestOpened && !app.descentMinigameActive) {
      const float worldW = static_cast<float>(app.descentSim.roomWidthTiles) * ed::descent::DescentSimState::kTileSize;
      const float worldH = static_cast<float>(app.descentSim.roomHeightTiles) * ed::descent::DescentSimState::kTileSize;
      const ed::descent::Vec2 chestPos{worldW * 0.5F, worldH * 0.45F};
      const float dToChest = ed::descent::Vec2::distance(app.descentSim.playerPos, chestPos);
      if (dToChest <= 65.0F) {
        if (IsKeyPressed(KEY_E) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
          startDescentVaultMinigame(app);
          return;
        }
      }
    }

    // Level-Up Modal
    if (app.descentSim.levelUpPending) {
      for (int i = 0; i < 3; ++i) {
        const Rectangle cardBtn{static_cast<float>(265 + i * 260), 210, 230, 310};
        if (CheckCollisionPointRec(mouse, cardBtn)) {
          app.descentCardHovered = i;
          if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            ed::descent::selectLevelUpChoice(app.descentSim, i);
            break;
          }
        }
      }
      if (IsKeyPressed(KEY_ONE)) ed::descent::selectLevelUpChoice(app.descentSim, 0);
      if (IsKeyPressed(KEY_TWO)) ed::descent::selectLevelUpChoice(app.descentSim, 1);
      if (IsKeyPressed(KEY_THREE)) ed::descent::selectLevelUpChoice(app.descentSim, 2);

      const Rectangle rerollBtn{380, 550, 140, 40};
      if (IsKeyPressed(KEY_R) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, rerollBtn))) {
        ed::descent::rerollLevelUpChoices(app.descentSim);
      }

      const Rectangle skipBtn{560, 550, 140, 40};
      if (IsKeyPressed(KEY_S) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, skipBtn))) {
        ed::descent::skipLevelUpChoice(app.descentSim);
      }

      const Rectangle banishBtn{740, 550, 140, 40};
      if (IsKeyPressed(KEY_B) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, banishBtn))) {
        ed::descent::banishLevelUpChoice(app.descentSim, 0);
      }
      return;
    }

    // Optional in-world door click or hotkey selection when chamber cleared
    if (app.descentSim.doorChoicesPending && !app.descentSim.runCompleted && app.descentSim.player.currentHp > 0.0F) {
      auto onDoorSelected = [&](int choiceIdx) {
        app.descentChestOpened = false;
        app.descentMinigameActive = false;
        app.descentPopupTimer = 0.0F;
        ed::descent::selectChamberChoice(app.descentSim, choiceIdx);
        setNotice(app, "Entered Chamber " + std::to_string(app.descentSim.chamberIndex) + "!");
      };

      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const Vector2 mouseWorld = GetScreenToWorld2D(mouse, app.descentCamera);
        for (std::size_t i = 0; i < app.descentSim.doorChoices.size(); ++i) {
          const auto& d = app.descentSim.doorChoices[i];
          const Rectangle doorRec{d.doorPos.x - d.doorWidth * 0.5F - 10.0F, d.doorPos.y - d.doorHeight * 0.5F - 20.0F, d.doorWidth + 20.0F, d.doorHeight + 40.0F};
          if (CheckCollisionPointRec(mouseWorld, doorRec)) {
            onDoorSelected(static_cast<int>(i));
            return;
          }
        }
      }
      if (IsKeyPressed(KEY_ONE) && app.descentSim.doorChoices.size() >= 1) {
        onDoorSelected(0);
        return;
      }
      if (IsKeyPressed(KEY_TWO) && app.descentSim.doorChoices.size() >= 2) {
        onDoorSelected(1);
        return;
      }
      if (IsKeyPressed(KEY_THREE) && app.descentSim.doorChoices.size() >= 3) {
        onDoorSelected(2);
        return;
      }
    }

    // Run Summary Modal on Final Run Completion (Victory) or Death (Defeat)
    if (app.descentSim.runCompleted || app.descentSim.player.currentHp <= 0.0F) {
      const bool victory = (app.descentSim.runCompleted && app.descentSim.bossDefeated && !app.descentSim.isEndless);

      if (victory) {
        // Dual Buttons on Standard Victory: Claim & Return (Left) vs Enter Endless (Right)
        const Rectangle retBtn{350, 520, 280, 48};
        const Rectangle contBtn{650, 520, 280, 48};

        if (IsKeyPressed(KEY_SPACE) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, contBtn))) {
          // Bank standard victory rewards
          app.descentMeta.crownShards += 1;
          app.descentMeta.soulFragments += 3;
          app.descentMeta.victories++;
          app.game.player.descentVictories++;
          if (app.descentMeta.calculateTotalHeat() > app.descentMeta.bestHeat) {
            app.descentMeta.bestHeat = app.descentMeta.calculateTotalHeat();
          }
          if (app.descentSim.chamberIndex > app.game.player.descentBestChamber) {
            app.game.player.descentBestChamber = app.descentSim.chamberIndex;
          }
          app.game.dirty = true;
          saveDescentMeta(app.savePath, app.descentMeta);

          // Transition seamlessly into Endless Catacombs Loop 1
          app.descentSim.isEndless = true;
          app.descentSim.endlessLoop = 1;
          app.descentSim.totalChambers = -1;
          app.descentSim.runCompleted = false;
          app.descentSim.bossDefeated = false;
          app.descentSim.bossName.clear();
          app.descentSim.bossCurrentHp = 0.0F;
          app.descentSim.bossMaxHp = 0.0F;
          app.descentSim.player.currentHp = std::min(app.descentSim.player.maxHp, app.descentSim.player.currentHp + app.descentSim.player.maxHp * 0.5F);
          app.descentSim.doorChoicesPending = true;
          ed::descent::generateChamberChoices(app.descentSim);
          setNotice(app, "Ventured into Endless Catacombs! Chamber " + std::to_string(app.descentSim.chamberIndex + 1) + " awaits...");
          return;
        }

        if (IsKeyPressed(KEY_ENTER) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, retBtn))) {
          app.descentMeta.embers += app.descentSim.embers;
          app.descentMeta.soulFragments += 3;
          app.descentMeta.crownShards += 1;
          app.descentMeta.lifetimeKills += app.descentSim.enemiesKilled;
          app.descentMeta.lifetimeRoomsCleared += app.descentSim.chamberIndex;
          app.descentMeta.victories++;
          if (app.descentMeta.calculateTotalHeat() > app.descentMeta.bestHeat) {
            app.descentMeta.bestHeat = app.descentMeta.calculateTotalHeat();
          }
          app.game.player.descentVictories++;
          if (app.descentSim.chamberIndex > app.game.player.descentBestChamber) {
            app.game.player.descentBestChamber = app.descentSim.chamberIndex;
          }
          if (app.descentSim.runLevel > app.game.player.descentMaxRunLevel) {
            app.game.player.descentMaxRunLevel = app.descentSim.runLevel;
          }
          app.game.dirty = true;
          saveDescentMeta(app.savePath, app.descentMeta);
          app.screen = Screen::DescentHub;
          setNotice(app, "Descent Conquered! Earned " + std::to_string(app.descentSim.embers) + " Embers, 3 Soul Fragments, and 1 Crown Shard!");
          return;
        }
      } else {
        // Defeat or Endless Completion
        const Rectangle retBtn{460, 520, 360, 48};
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, retBtn))) {
          if (app.descentSim.isEndless) {
            const int souls = std::max(1, app.descentSim.chamberIndex / 5);
            const int crowns = std::max(0, app.descentSim.endlessLoop - 1);
            app.descentMeta.embers += app.descentSim.embers;
            app.descentMeta.soulFragments += souls;
            app.descentMeta.crownShards += crowns;
            app.descentMeta.lifetimeKills += app.descentSim.enemiesKilled;
            app.descentMeta.lifetimeRoomsCleared += app.descentSim.chamberIndex;
            if (app.descentSim.chamberIndex > app.descentMeta.bestEndlessChamber) {
              app.descentMeta.bestEndlessChamber = app.descentSim.chamberIndex;
            }
            if (app.descentSim.chamberIndex > app.game.player.descentEndlessBestChamber) {
              app.game.player.descentEndlessBestChamber = app.descentSim.chamberIndex;
            }
            if (app.descentSim.runLevel > app.game.player.descentMaxRunLevel) {
              app.game.player.descentMaxRunLevel = app.descentSim.runLevel;
            }
            app.game.dirty = true;
            saveDescentMeta(app.savePath, app.descentMeta);
            app.screen = Screen::DescentHub;
            setNotice(app, "Endless Catacombs expedition ended at Chamber " + std::to_string(app.descentSim.chamberIndex) +
                           "! Earned " + std::to_string(app.descentSim.embers) + " Embers and " + std::to_string(souls) + " Soul Fragments.");
            return;
          } else {
            app.descentMeta.embers += app.descentSim.embers;
            app.descentMeta.soulFragments += 1;
            app.descentMeta.lifetimeKills += app.descentSim.enemiesKilled;
            app.descentMeta.lifetimeRoomsCleared += app.descentSim.chamberIndex;
            if (app.descentSim.chamberIndex > app.game.player.descentBestChamber) {
              app.game.player.descentBestChamber = app.descentSim.chamberIndex;
            }
            if (app.descentSim.runLevel > app.game.player.descentMaxRunLevel) {
              app.game.player.descentMaxRunLevel = app.descentSim.runLevel;
            }
            app.game.dirty = true;
            saveDescentMeta(app.savePath, app.descentMeta);
            app.screen = Screen::DescentHub;
            setNotice(app, "Expedition ended. Earned " + std::to_string(app.descentSim.embers) + " Embers and 1 Soul Fragment.");
            return;
          }
        }
      }
    }
  }
}

} // namespace ed::app
