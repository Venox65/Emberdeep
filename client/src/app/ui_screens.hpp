#pragma once

#include <string>
#include <raylib.h>
#include "app_state.hpp"

namespace ed::app {

Texture2D getTexture(const std::string& path);
void drawTopBar(const AppState& app);
void drawNavigation(const AppState& app);
void drawMineScreen(const AppState& app);
void drawCampaignScreen(const AppState& app);
void drawDungeonScreen(AppState& app);
void drawGemHollowScreen(AppState& app);
void drawEnchantingScreen(AppState& app);
void drawProfileScreen(const AppState& app);
void drawInventoryScreen(const AppState& app);
void drawCraftingScreen(const AppState& app);

void clampInventoryScroll(AppState& app);
void syncGearScroll(AppState& app);
int inventoryGearStartY(const AppState& app);

} // namespace ed::app
