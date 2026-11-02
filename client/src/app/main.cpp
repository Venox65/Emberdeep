#include <algorithm>
#include <cstddef>
#include <string>

#include <raylib.h>

#include "app_state.hpp"
#include "game_actions.hpp"
#include "game_updates.hpp"
#include "input_handler.hpp"
#include "save_system.hpp"
#include "ui_screens.hpp"

using namespace ed::app;

int main() {
  InitWindow(kWidth, kHeight, "Emberdeep: The Hollow Crown");
  SetTargetFPS(60);
  SetExitKey(KEY_NULL);

  AppState app;
  configureSaveStorage(app);
  setLastPlayedSave(app.savePath);
  app.saveFilesList = scanSaveFiles(app.savePath);

  loadSave(app);
  for (std::size_t i = 0; i < app.layers.size(); ++i) {
    if (app.layers[i].id == app.game.player.currentLayer &&
        ed::canUnlockLayer(app.game, app.layers[i])) {
      app.layerIndex = static_cast<int>(i);
      break;
    }
  }
  app.game.player.currentLayer = app.layers[static_cast<std::size_t>(app.layerIndex)].id;
  spawnBlock(app);
  startSaveWorker(app);

  while (!WindowShouldClose()) {
    const float delta = GetFrameTime();
    handleInput(app);
    updateEffects(app, delta);

    BeginDrawing();
    ClearBackground(Color{20, 19, 29, 255});
    const bool canShakeScreen = (app.screen == Screen::Mine || app.screen == Screen::Campaign || app.screen == Screen::Dungeon);
    const bool enableShake = app.settings.screenShake && app.shake > 0.0F && canShakeScreen;
    const float offsetX = enableShake ? static_cast<float>(GetRandomValue(-3, 3)) : 0.0F;
    const float offsetY = enableShake ? static_cast<float>(GetRandomValue(-3, 3)) : 0.0F;
    BeginMode2D(Camera2D{{0.0F, 0.0F}, {offsetX, offsetY}, 0.0F, 1.0F});
    if (app.screen == Screen::Loading) {
      drawLoadingScreen(app);
    } else if (app.screen == Screen::Title) {
      drawTitleScreen(app);
    } else if (app.screen == Screen::DescentRun) {
      drawDescentRunScreen(app);
    } else if (app.screen == Screen::DescentHub) {
      drawDescentHubScreen(app);
      drawNavigation(app);
    } else {
      drawTopBar(app);
      switch (app.screen) {
        case Screen::Mine: drawMineScreen(app); break;
        case Screen::Inventory: drawInventoryScreen(app); break;
        case Screen::Crafting: drawCraftingScreen(app); break;
        case Screen::Campaign: drawCampaignScreen(app); break;
        case Screen::Dungeon: drawDungeonScreen(app); break;
        case Screen::Profile: drawProfileScreen(app); break;
        case Screen::GemHollow: drawGemHollowScreen(app); break;
        case Screen::Enchanting: drawEnchantingScreen(app); break;
        case Screen::Settings: drawSettingsScreen(app); break;
        default: break;
      }
      drawNavigation(app);
      if (app.screen == Screen::Mine) {
        for (const auto& particle : app.particles) {
          DrawCircleV(particle.position, 4.0F, Fade(particle.color, particle.lifetime * 2.0F));
        }
        for (const auto& text : app.floatingText) {
          DrawText(text.text.c_str(), static_cast<int>(text.position.x), static_cast<int>(text.position.y),
                   22, Fade(text.color, text.lifetime));
        }
      }
    }
    EndMode2D();
    if (app.noticeLifetime > 0.0F) {
      DrawRectangle(300, 625, 680, 30, Color{35, 34, 45, 230});
      drawTextCentered(app.notice, 632, 16, Color{234, 229, 202, 255});
    }
    if (app.achievementNotificationTimer > 0.0F) {
      const float alpha = std::min(1.0F, app.achievementNotificationTimer * 2.0F);
      DrawRectangle(320, 90, 640, 40, Fade(Color{28, 26, 42, 240}, alpha));
      DrawRectangleLinesEx({320, 90, 640, 40}, 2.0F, Fade(Color{255, 215, 0, 255}, alpha));
      drawTextCentered(app.achievementNotification, 100, 18, Fade(Color{255, 235, 120, 255}, alpha));
    }
    if (app.screen == Screen::Inventory) {
      DrawRectangle(0, 685, 1280, 35, Color{28, 27, 39, 200});
      DrawText(("Gear " + std::to_string(app.selectedGear + 1) + "/" +
                std::to_string(app.inventory.gear.size()) + "   Scroll: W/S").c_str(),
               70, 695, 14, Color{174, 174, 187, 255});
      DrawText("A/D select   U upgrade   E equip   R refine gems   Click gem slot to socket   Right-click remove   N enchant", 310, 695, 14,
               Color{174, 174, 187, 255});
    }
    drawSaveSelectModal(app);
    if (app.screen != Screen::Loading && app.screen != Screen::Title) {
      drawOfflineProgressModal(app);
    }
    if (app.screen == Screen::Inventory) {
      drawGemRadialMenu(app);
    }
    drawGemRefiningModal(app);
    EndDrawing();
  }

  stopSaveWorker(app);
  CloseWindow();
  return 0;
}
