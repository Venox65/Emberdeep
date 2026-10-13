#pragma once

#include "app_state.hpp"

namespace ed::app {

void drawCampaignMinigames(const AppState& app, const ed::BossDef& boss);
bool handleActiveCampaignInput(AppState& app);

} // namespace ed::app
