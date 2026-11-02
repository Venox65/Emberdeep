#pragma once

#include "app_state.hpp"

namespace ed::app {

void handleInput(AppState& app);

void handleInventoryInput(AppState& app, Vector2 mouse, bool leftClick);
void handleProfileInput(AppState& app, Vector2 mouse, bool leftClick);
void handleDescentInput(AppState& app, Vector2 mouse, bool leftClick);

} // namespace ed::app
