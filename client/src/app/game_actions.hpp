#pragma once

#include <string>
#include <raylib.h>
#include "app_state.hpp"

namespace ed::app {

void spawnBlock(AppState& app);
int layerIndexForId(const AppState& app, const std::string& id);
std::string layerLockReason(const ed::GameState& game, const ed::LayerDef& layer);
void changeLayer(AppState& app, int direction);
std::string craftFailureReason(const AppState& app, const ed::CraftRecipe& recipe);
bool dungeonUnlocked(const AppState& app, int zoneIndex);
std::string dungeonLockReason(const AppState& app, int zoneIndex);

void spawnHitEffects(AppState& app, Vector2 center, Color color, int damage);
void mine(AppState& app, Vector2 clickPosition);
void attemptCampaign(AppState& app);
void startCampaignAttempt(AppState& app);
void startBossCombat(AppState& app, std::size_t bossIndex, int stars, bool isEcho, int echoRank);
void finishBossCombat(AppState& app);
void runDungeon(AppState& app);

} // namespace ed::app
