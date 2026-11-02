#pragma once

#include <string>
#include <vector>

#include "ed/campaign.hpp"
#include "ed/game_state.hpp"
#include "ed/inventory.hpp"

namespace ed {

struct AchievementDef {
  std::string id;
  std::string title;
  std::string description;
  std::string category;
  std::string icon;
};

const std::vector<AchievementDef>& allAchievements();

std::vector<std::string> evaluateNewAchievements(const GameState& game,
                                                 const InventoryState& inventory,
                                                 const CampaignState& campaign);

bool isAchievementUnlocked(const PlayerState& player, const std::string& achievementId);

} // namespace ed
