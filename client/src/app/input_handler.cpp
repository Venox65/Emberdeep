#include "input_handler.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <utility>

#include "campaign_minigames.hpp"
#include "game_actions.hpp"
#include "save_system.hpp"
#include "ui_screens.hpp"
#include "ed/postgame.hpp"

namespace ed::app {

void handleInput(AppState& app) {
  if (app.screen == Screen::Loading) {
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      app.screen = Screen::Title;
    }
    return;
  }

  if (app.screen == Screen::Title) {
    if (app.showSaveSelect) {
      handleSaveSelectInput(app);
      return;
    }
    const Vector2 mouse = GetMousePosition();
    const Rectangle enterBtn{450, 350, 380, 68};
    const Rectangle saveBtn{490, 442, 300, 44};

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
        (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, enterBtn))) {
      app.screen = Screen::Mine;
      return;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, saveBtn)) {
      app.showSaveSelect = true;
      app.saveFilesList = scanSaveFiles(app.savePath);
      app.saveSelectScroll = 0;
      app.newSaveInputFocused = false;
      return;
    }
    return;
  }

  if (app.offlinePopupActive) {
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE)) {
      app.offlinePopupActive = false;
      return;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      const Vector2 mouse = GetMousePosition();
      if (CheckCollisionPointRec(mouse, {490, 540, 300, 46})) {
        app.offlinePopupActive = false;
        return;
      }
    }
    return;
  }
  if (app.showSaveSelect) {
    handleSaveSelectInput(app);
    return;
  }
  if (handleActiveCampaignInput(app)) return;

  if (IsKeyPressed(KEY_F5)) {
    saveNow(app);
    app.lastAutosave = std::chrono::steady_clock::now();
  }

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    const Vector2 mouse = GetMousePosition();
    if (CheckCollisionPointRec(mouse, {1010, 14, 72, 34})) {
      saveNow(app);
      app.lastAutosave = std::chrono::steady_clock::now();
      return;
    }
    if (CheckCollisionPointRec(mouse, {1090, 14, 160, 34})) {
      app.showSaveSelect = true;
      app.saveFilesList = scanSaveFiles(app.savePath);
      app.saveSelectScroll = 0;
      app.newSaveInputFocused = false;
      return;
    }
    const auto switchTab = [&](Screen s) {
      if (app.screen != s) {
        app.screen = s;
        app.gemSlotMenuOpen = false;
        app.particles.clear();
        app.floatingText.clear();
        app.shake = 0.0F;
      }
    };

    if (app.sidebarOpen) {
      if (CheckCollisionPointRec(mouse, {150, 15, 30, 30})) {
        app.sidebarOpen = false;
      } else {
        const Screen navScreens[] = {Screen::Mine, Screen::Inventory, Screen::Crafting, Screen::Campaign, Screen::Dungeon, Screen::GemHollow, Screen::Enchanting, Screen::Profile, Screen::Settings, Screen::DescentHub};
        for (int i = 0; i < 10; ++i) {
          const int y = 70 + i * 46;
          if (CheckCollisionPointRec(mouse, {10, static_cast<float>(y), 180, 38})) {
            switchTab(navScreens[i]);
            app.sidebarOpen = false;
            break;
          }
        }
      }
    } else {
      if (CheckCollisionPointRec(mouse, {1220, 100, 40, 40})) {
        app.sidebarOpen = true;
      }
    }
  }

  const auto switchTab = [&](Screen s) {
    if (app.screen != s) {
      app.screen = s;
      app.gemSlotMenuOpen = false;
      app.gemRefineModalOpen = false;
      app.particles.clear();
      app.floatingText.clear();
      app.shake = 0.0F;
    }
  };

  if (app.gemRefineModalOpen) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_R)) {
      app.gemRefineModalOpen = false;
      return;
    }
    const float wheel = GetMouseWheelMove();
    if (wheel != 0.0F) {
      app.gemRefineScroll -= static_cast<int>(wheel * 60.0F);
    }
    return;
  }

  if (app.screen != Screen::DescentRun && app.screen != Screen::Title) {
    if (IsKeyPressed(KEY_ONE)) switchTab(Screen::Mine);
    if (IsKeyPressed(KEY_TWO)) switchTab(Screen::Inventory);
    if (IsKeyPressed(KEY_THREE)) switchTab(Screen::Crafting);
    if (IsKeyPressed(KEY_FOUR)) switchTab(Screen::Campaign);
    if (IsKeyPressed(KEY_FIVE)) switchTab(Screen::Dungeon);
    if (IsKeyPressed(KEY_SIX)) switchTab(Screen::GemHollow);
    if (IsKeyPressed(KEY_SEVEN)) switchTab(Screen::Enchanting);
    if (IsKeyPressed(KEY_EIGHT)) switchTab(Screen::Profile);
    if (IsKeyPressed(KEY_NINE)) switchTab(Screen::Settings);
    if (IsKeyPressed(KEY_ZERO)) switchTab(Screen::DescentHub);
  }

  const Vector2 mouse = GetMousePosition();
  const bool leftClick = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

  if (app.screen == Screen::Mine) {
    const bool isLayer10 = (app.layers[static_cast<std::size_t>(app.layerIndex)].id == "aether-vault");
    const bool strataUnlocked = (isLayer10 && ed::isDeepStrataUnlocked(app.game));

    if (IsKeyPressed(KEY_Q)) {
      changeLayer(app, -1);
    }
    if (IsKeyPressed(KEY_E)) {
      changeLayer(app, 1);
    }

    if (strataUnlocked) {
      const int maxDepth = std::max(1, app.game.player.deepestDepth + 1);
      if (IsKeyPressed(KEY_LEFT_BRACKET) || IsKeyPressed(KEY_MINUS)) {
        if (app.deepStrataDepth > 0) {
          --app.deepStrataDepth;
          spawnBlock(app);
        }
      }
      if (IsKeyPressed(KEY_RIGHT_BRACKET) || IsKeyPressed(KEY_EQUAL)) {
        if (app.deepStrataDepth < maxDepth) {
          ++app.deepStrataDepth;
          spawnBlock(app);
        }
      }
      if (IsKeyPressed(KEY_T)) {
        app.deepStrataAutoAdvance = !app.deepStrataAutoAdvance;
        setNotice(app, app.deepStrataAutoAdvance ? "Deep Strata auto-advance enabled." : "Deep Strata auto-advance disabled.");
      }
    }

    if (IsKeyPressed(KEY_SPACE)) {
      app.activeMiningTimer = 30.0F;
      mine(app, {640, 305}, false);
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      const Vector2 mouse = GetMousePosition();
      bool buttonClicked = false;
      if (strataUnlocked) {
        const Rectangle minusBtn{410, 164, 34, 22};
        const Rectangle plusBtn{835, 164, 34, 22};
        const Rectangle autoBtn{885, 164, 95, 22};
        const int maxDepth = std::max(1, app.game.player.deepestDepth + 1);
        if (CheckCollisionPointRec(mouse, minusBtn)) {
          if (app.deepStrataDepth > 0) {
            --app.deepStrataDepth;
            spawnBlock(app);
          }
          buttonClicked = true;
        } else if (CheckCollisionPointRec(mouse, plusBtn)) {
          if (app.deepStrataDepth < maxDepth) {
            ++app.deepStrataDepth;
            spawnBlock(app);
          }
          buttonClicked = true;
        } else if (CheckCollisionPointRec(mouse, autoBtn)) {
          app.deepStrataAutoAdvance = !app.deepStrataAutoAdvance;
          setNotice(app, app.deepStrataAutoAdvance ? "Deep Strata auto-advance enabled." : "Deep Strata auto-advance disabled.");
          buttonClicked = true;
        }
      }
      if (!buttonClicked && CheckCollisionPointRec(mouse, {400, 220, 480, 260})) {
        app.activeMiningTimer = 30.0F;
        mine(app, mouse, false);
      }
    }
  } else if (app.screen == Screen::Inventory || app.screen == Screen::GemHollow ||
             app.screen == Screen::Enchanting || app.screen == Screen::Crafting) {
    handleInventoryInput(app, mouse, leftClick);
  } else if (app.screen == Screen::Campaign) {
    if (app.inBossCombat) {
      if (app.bossCombatShowResults) {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) ||
            (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), {540, 560, 200, 45}))) {
          finishBossCombat(app);
        }
      } else {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
          finishBossCombat(app);
          setNotice(app, "Retreated from boss combat.");
        }
      }
      return;
    }
    if (!app.campaignAttempt && (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_TAB))) {
      if (ed::isEchoCampaignUnlocked(app.game)) {
        app.echoCampaignMode = !app.echoCampaignMode;
        app.campaignScore = 0;
        app.campaignIntro = false;
        setNotice(app, app.echoCampaignMode ? "Echo Campaign active! Face empowered boss echoes."
                                            : "Standard Campaign active.");
      } else {
        setNotice(app, "Echo Campaign unlocks after defeating The Hollow King (Boss 10).");
      }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !app.campaignAttempt) {
      const Vector2 mouse = GetMousePosition();
      if (CheckCollisionPointRec(mouse, {380, 160, 240, 36})) {
        if (app.echoCampaignMode) {
          app.echoCampaignMode = false;
          app.campaignScore = 0;
          app.campaignIntro = false;
          setNotice(app, "Standard Campaign active.");
        }
      } else if (CheckCollisionPointRec(mouse, {660, 160, 240, 36})) {
        if (!app.echoCampaignMode) {
          if (ed::isEchoCampaignUnlocked(app.game)) {
            app.echoCampaignMode = true;
            app.campaignScore = 0;
            app.campaignIntro = false;
            setNotice(app, "Echo Campaign active! Face empowered boss echoes.");
          } else {
            setNotice(app, "Echo Campaign unlocks after defeating The Hollow King (Boss 10).");
          }
        }
      }
    }
    if (IsKeyPressed(KEY_A)) {
      const int nextBoss = std::max(0, app.selectedBoss - 1);
      if (nextBoss != app.selectedBoss &&
          !ed::isBossUnlocked(app.game, app.campaign, app.bosses,
                              static_cast<std::size_t>(nextBoss))) {
        setNotice(app, "That boss is locked. Clear the previous boss and meet its level requirement.");
      } else {
        app.selectedBoss = nextBoss;
        app.campaignScore = 0;
        app.campaignAttempt = false;
        app.campaignIntro = false;
      }
    }
    if (IsKeyPressed(KEY_D)) {
      const int nextBoss = std::min(static_cast<int>(app.bosses.size()) - 1, app.selectedBoss + 1);
      if (!ed::isBossUnlocked(app.game, app.campaign, app.bosses,
                              static_cast<std::size_t>(nextBoss))) {
        const auto& boss = app.bosses[static_cast<std::size_t>(nextBoss)];
        setNotice(app, "That boss is locked. Reach level " + std::to_string(boss.minLevel) +
                           " and clear the previous boss.");
      } else {
        app.selectedBoss = nextBoss;
        app.campaignScore = 0;
        app.campaignAttempt = false;
        app.campaignIntro = false;
      }
    }
    if (IsKeyPressed(KEY_ENTER)) {
      if (!app.campaignAttempt) {
        const auto bossIndex = static_cast<std::size_t>(app.selectedBoss);
        if (!ed::isBossUnlocked(app.game, app.campaign, app.bosses, bossIndex)) {
          const auto& lockedBoss = app.bosses[bossIndex];
          setNotice(app, "Boss locked. Reach level " + std::to_string(lockedBoss.minLevel) +
                             " and clear the previous boss.");
        } else if (app.campaignIntro) {
          app.campaignIntro = false;
          startCampaignAttempt(app);
        } else {
          app.campaignIntro = true;
        }
      }
    }
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE)) {
      if (app.campaignIntro) {
        app.campaignIntro = false;
      } else if (app.campaignAttempt) {
        app.campaignAttempt = false;
        app.campaignScore = 0;
        app.campaignHeartsLost = 0;
        setNotice(app, "Boss attempt abandoned.");
      }
    }
  } else if (app.screen == Screen::Dungeon) {
    if (app.inDungeonCombat) return;

    const bool abyssUnlocked = (app.game.player.bossesCleared >= 10 || app.game.player.ascensions > 0 || app.game.player.abyssBestFloor > 0);
    if (abyssUnlocked) {
      if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_TAB)) {
        app.abyssDungeonMode = !app.abyssDungeonMode;
        setNotice(app, app.abyssDungeonMode ? "The Abyss selected (Endless Mode)." : "Standard Expeditions selected.");
      }
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const Vector2 mouse = GetMousePosition();
        const Rectangle stdTab{400, 155, 220, 36};
        const Rectangle abyssTab{660, 155, 220, 36};
        if (CheckCollisionPointRec(mouse, stdTab)) {
          app.abyssDungeonMode = false;
          setNotice(app, "Standard Expeditions selected.");
        } else if (CheckCollisionPointRec(mouse, abyssTab)) {
          app.abyssDungeonMode = true;
          setNotice(app, "The Abyss selected (Endless Mode).");
        }
      }
    }

    if (!app.abyssDungeonMode) {
      if (IsKeyPressed(KEY_A)) {
        const int nextZone = std::max(0, app.selectedZone - 1);
        if (nextZone != app.selectedZone && !dungeonUnlocked(app, nextZone)) {
          setNotice(app, dungeonLockReason(app, nextZone));
        } else {
          app.selectedZone = nextZone;
        }
      }
      if (IsKeyPressed(KEY_D)) {
        const int nextZone = std::min(static_cast<int>(app.zones.size()) - 1, app.selectedZone + 1);
        if (!dungeonUnlocked(app, nextZone)) {
          setNotice(app, dungeonLockReason(app, nextZone));
        } else {
          app.selectedZone = nextZone;
        }
      }
    }

    if (IsKeyPressed(KEY_ENTER)) {
      if (app.abyssDungeonMode) {
        runDungeon(app);
      } else {
        if (!dungeonUnlocked(app, app.selectedZone)) {
          setNotice(app, dungeonLockReason(app, app.selectedZone));
        } else {
          runDungeon(app);
        }
      }
    }
  } else if (app.screen == Screen::Profile) {
    handleProfileInput(app, mouse, leftClick);
  } else if (app.screen == Screen::Settings) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      const Vector2 mouse = GetMousePosition();
      // Screen shake toggle: {850, 220, 100, 36}
      if (CheckCollisionPointRec(mouse, {850, 220, 100, 36})) {
        app.settings.screenShake = !app.settings.screenShake;
      }
      // Abbreviated numbers toggle: {850, 275, 100, 36}
      if (CheckCollisionPointRec(mouse, {850, 275, 100, 36})) {
        app.settings.abbreviatedNumbers = !app.settings.abbreviatedNumbers;
      }
      // Master volume: minus {760, 380, 36, 32}, plus {914, 380, 36, 32}
      if (CheckCollisionPointRec(mouse, {760, 380, 36, 32})) {
        app.settings.masterVolume = std::max(0, app.settings.masterVolume - 10);
      } else if (CheckCollisionPointRec(mouse, {914, 380, 36, 32})) {
        app.settings.masterVolume = std::min(100, app.settings.masterVolume + 10);
      }
      // SFX volume: minus {760, 425, 36, 32}, plus {914, 425, 36, 32}
      if (CheckCollisionPointRec(mouse, {760, 425, 36, 32})) {
        app.settings.sfxVolume = std::max(0, app.settings.sfxVolume - 10);
      } else if (CheckCollisionPointRec(mouse, {914, 425, 36, 32})) {
        app.settings.sfxVolume = std::min(100, app.settings.sfxVolume + 10);
      }
      // Music volume: minus {760, 470, 36, 32}, plus {914, 470, 36, 32}
      if (CheckCollisionPointRec(mouse, {760, 470, 36, 32})) {
        app.settings.musicVolume = std::max(0, app.settings.musicVolume - 10);
      } else if (CheckCollisionPointRec(mouse, {914, 470, 36, 32})) {
        app.settings.musicVolume = std::min(100, app.settings.musicVolume + 10);
      }
      // Save Now button: {320, 545, 190, 44}
      if (CheckCollisionPointRec(mouse, {320, 545, 190, 44})) {
        saveNow(app);
        setNotice(app, "Game saved successfully.");
      }
      // Select Save / Manage button: {530, 545, 210, 44}
      if (CheckCollisionPointRec(mouse, {530, 545, 210, 44})) {
        app.showSaveSelect = true;
        app.saveFilesList = scanSaveFiles(app.savePath);
        app.saveSelectScroll = 0;
        app.newSaveInputFocused = false;
      }
      // Return to Title Screen button: {760, 545, 190, 44}
      if (CheckCollisionPointRec(mouse, {760, 545, 190, 44})) {
        saveNow(app);
        app.screen = Screen::Title;
        app.particles.clear();
        app.floatingText.clear();
        app.shake = 0.0F;
      }
    }
  } else if (app.screen == Screen::DescentHub || app.screen == Screen::DescentRun) {
    handleDescentInput(app, mouse, leftClick);
  }
}

} // namespace ed::app
