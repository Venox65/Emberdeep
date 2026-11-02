#include "input_handler.hpp"

#include <algorithm>
#include <string>

#include "game_actions.hpp"
#include "save_system.hpp"
#include "ed/achievements.hpp"
#include "ed/postgame.hpp"

namespace ed::app {

void handleProfileInput(AppState& app, Vector2 mouse, bool leftClick) {
  if (app.screen == Screen::Profile) {
    const Vector2 mouse = GetMousePosition();
    const bool unlocked = (app.game.player.bossesCleared >= 10 || app.game.player.ascensions > 0);

    // Profile Right Panel Tabs
    const Rectangle vowsTabBtn{505, 160, 240, 36};
    const Rectangle perksTabBtn{750, 160, 240, 36};
    const Rectangle achievementsTabBtn{995, 160, 245, 36};

    if (IsKeyPressed(KEY_TAB)) {
      if (app.profileTab == ProfileTab::Vows) app.profileTab = ProfileTab::Perks;
      else if (app.profileTab == ProfileTab::Perks) app.profileTab = ProfileTab::Achievements;
      else app.profileTab = ProfileTab::Vows;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      if (CheckCollisionPointRec(mouse, vowsTabBtn)) {
        app.profileTab = ProfileTab::Vows;
      } else if (CheckCollisionPointRec(mouse, perksTabBtn)) {
        app.profileTab = ProfileTab::Perks;
      } else if (CheckCollisionPointRec(mouse, achievementsTabBtn)) {
        app.profileTab = ProfileTab::Achievements;
      }
    }

    if (app.profileTab == ProfileTab::Achievements) {
      const float wheel = GetMouseWheelMove();
      if (wheel != 0.0F) {
        app.achievementsScroll -= static_cast<int>(wheel * 60.0F);
      }
      if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) {
        app.achievementsScroll -= 80;
      }
      if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) {
        app.achievementsScroll += 80;
      }
      const int maxScroll = std::max(0, static_cast<int>(ed::allAchievements().size()) * 68 - 410);
      app.achievementsScroll = std::clamp(app.achievementsScroll, 0, maxScroll);
      return;
    }

    if (app.profileTab == ProfileTab::Vows) {
      const Rectangle respecBtn{1040, 202, 180, 32};
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, respecBtn)) {
        const int cost = ed::respecVowsCost(app.game);
        if (ed::respecVows(app.game)) {
          requestSave(app);
          setNotice(app, "Miner's Vows reset! Refunded all spent Vow Points for " + ed::formatCount(cost) + " gold.");
        } else {
          if (app.game.player.gold < cost) {
            setNotice(app, "Need " + ed::formatCount(cost) + " gold to respec vows.");
          } else {
            setNotice(app, "No Vow Points have been spent yet.");
          }
        }
      }

      const std::vector<std::vector<std::pair<std::string, std::string>>> vowGrid = {
        {{"strong_arms", "Strong Arms"}, {"keen_eye", "Keen Eye"}, {"prospector", "Prospector"}, {"deep_breath", "Deep Breath"}, {"shatterer", "Shatterer"}},
        {{"stout_heart", "Stout Heart"}, {"whetstone", "Whetstone"}, {"ironskin", "Ironskin"}, {"swift_boots", "Swift Boots"}, {"second_wind", "Second Wind"}},
        {{"quick_study", "Quick Study"}, {"haggler", "Haggler"}, {"steady_hand", "Steady Hand"}, {"dust_collector", "Dust Collector"}, {"gem_whisperer", "Gem Whisperer"}}
      };

      for (std::size_t c = 0; c < vowGrid.size(); ++c) {
        const int colX = 520 + static_cast<int>(c) * 236;
        for (std::size_t r = 0; r < vowGrid[c].size(); ++r) {
          const auto& [vowId, vowName] = vowGrid[c][r];
          const int rowY = 268 + static_cast<int>(r) * 78;
          const Rectangle upBtn{static_cast<float>(colX + 182), static_cast<float>(rowY + 20), 34, 32};
          if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, upBtn)) {
            if (ed::upgradeVow(app.game, vowId)) {
              requestSave(app);
              setNotice(app, "Upgraded " + vowName + " to Rank " + std::to_string(ed::currentVowRank(app.game, vowId)) + "!");
            } else {
              if (app.game.player.vowPoints <= 0) {
                setNotice(app, "No Vow Points available. Level up to gain Vow Points!");
              } else if (ed::isVowCapstone(vowId) && app.game.player.level < 30) {
                setNotice(app, "Capstone vow requires Player Level 30.");
              } else {
                setNotice(app, vowName + " is already at maximum rank.");
              }
            }
          }
        }
      }
      return;
    }

    if (app.ascensionConfirm) {
      const Rectangle confirmBtn{410, 430, 210, 48};
      const Rectangle cancelBtn{670, 430, 210, 48};
      if (IsKeyPressed(KEY_Y) ||
          (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, confirmBtn))) {
        const int marks = ed::applyAscension(app.game, &app.inventory);
        app.ascensionConfirm = false;
        app.layerIndex = 0;
        app.deepStrataDepth = 0;
        app.game.player.currentLayer = "ashen-warrens";
        spawnBlock(app);
        requestSave(app);
        setNotice(app, "Ascended! Gained " + std::to_string(marks) + " Ember Marks.");
      } else if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE) ||
                 (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, cancelBtn))) {
        app.ascensionConfirm = false;
      }
      return;
    }

    if (unlocked) {
      const Rectangle ascendBtn{1060, 203, 160, 40};
      if ((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, ascendBtn)) ||
          IsKeyPressed(KEY_ENTER)) {
        app.ascensionConfirm = true;
      }

      // Auto-Clicker (Chrono Automaton) inputs
      if (app.game.player.autoClickerLevel == 0) {
        const Rectangle unlockBtn{1040, 260, 175, 38};
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, unlockBtn)) {
          if (ed::upgradeAutoClicker(app.game)) {
            requestSave(app);
            setNotice(app, "Unlocked Chrono Automaton (1.0 clicks/sec)!");
          } else {
            setNotice(app, "Need 1 Chrono Core to unlock Chrono Automaton.");
          }
        }
      } else {
        // Toggle button: {925, 260, 105, 38}
        const Rectangle toggleBtn{925, 260, 105, 38};
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, toggleBtn)) {
          app.game.player.autoClickerEnabled = !app.game.player.autoClickerEnabled;
          requestSave(app);
          setNotice(app, app.game.player.autoClickerEnabled ? "Chrono Automaton ENABLED" : "Chrono Automaton PAUSED");
        }

        // Upgrade button: {1040, 260, 175, 38}
        const Rectangle upBtn{1040, 260, 175, 38};
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, upBtn)) {
          if (ed::upgradeAutoClicker(app.game)) {
            requestSave(app);
            setNotice(app, "Upgraded Chrono Automaton to Lv." + std::to_string(app.game.player.autoClickerLevel) + "!");
          } else {
            setNotice(app, "Cannot upgrade Chrono Automaton (insufficient cores or max level).");
          }
        }
      }

      const char* perkIds[] = {"potential", "power", "wealth", "wisdom", "armory", "smelting"};
      int py = 316;
      for (int i = 0; i < 6; ++i) {
        const Rectangle upgradeBtn{1040, static_cast<float>(py + 7), 175, 36};
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, upgradeBtn)) {
          if (ed::upgradeEmberPerk(app.game, perkIds[i])) {
            requestSave(app);
            setNotice(app, "Upgraded " + std::string(perkIds[i]) + " perk!");
          } else {
            setNotice(app, "Cannot upgrade " + std::string(perkIds[i]) + " (not enough marks or max rank).");
          }
        }
        py += 56;
      }
    }
  }
}

} // namespace ed::app
