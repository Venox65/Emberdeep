#pragma once

#include <string>
#include <raylib.h>
#include "app_state.hpp"

namespace ed::app {

Texture2D getTexture(const std::string& path);
void drawTopBar(const AppState& app);
void drawNavigation(const AppState& app);
void drawLoadingScreen(const AppState& app);
void drawTitleScreen(const AppState& app);
void drawMineScreen(const AppState& app);
void drawCampaignScreen(const AppState& app);
void drawDungeonScreen(AppState& app);
void drawGemHollowScreen(AppState& app);
void drawEnchantingScreen(AppState& app);
void drawProfileScreen(const AppState& app);
void drawInventoryScreen(const AppState& app);
void drawCraftingScreen(const AppState& app);
void drawSettingsScreen(AppState& app);
void drawDescentHubScreen(AppState& app);
void drawDescentRunScreen(AppState& app);
void drawOfflineProgressModal(const AppState& app);
void drawGemRadialMenu(AppState& app);
void drawGemRefiningModal(AppState& app);

void clampInventoryScroll(AppState& app);
void syncGearScroll(AppState& app);
int inventoryGearStartY(const AppState& app);

inline void drawTextWithOutline(const char* text, int posX, int posY, int fontSize, Color color, Color outlineColor = BLACK) {
  DrawText(text, posX - 1, posY, fontSize, outlineColor);
  DrawText(text, posX + 1, posY, fontSize, outlineColor);
  DrawText(text, posX, posY - 1, fontSize, outlineColor);
  DrawText(text, posX, posY + 1, fontSize, outlineColor);
  DrawText(text, posX, posY, fontSize, color);
}

} // namespace ed::app
