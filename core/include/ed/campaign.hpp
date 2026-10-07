#pragma once

#include <string>
#include <vector>

#include "ed/game_state.hpp"

namespace ed {

enum class BossChallenge {
  FrenzyDig,
  BoneDuel,
  CinderDance,
  HeadHunt,
  ParryDuel,
  RuneRecall,
  StarTrace,
  Heartpierce,
  SoulDodge,
  FiveTrials,
};

struct BossDef {
  std::string id;
  std::string name;
  std::string layerId;
  BossChallenge challenge = BossChallenge::FrenzyDig;
  int minLevel = 1;
  int recommendedGearTier = 1;
  int clearScore = 0;
  int twoStarScore = 0;
  int threeStarScore = 0;
  int firstClearGold = 0;
  int firstClearXp = 0;
  std::string legendaryId;
};

struct BossAttempt {
  int score = 0;
  int heartsLost = 0;
  bool completed = false;
  double focus = 0.0;
};

struct BossOutcome {
  bool cleared = false;
  int stars = 0;
  int goldReward = 0;
  int xpReward = 0;
  bool firstClear = false;
  std::string legendaryId;
};

struct CampaignState {
  std::vector<int> bestStars;
  std::vector<bool> cleared;
};

std::vector<BossDef> defaultBosses();
CampaignState createCampaignState(std::size_t bossCount = 10);
bool isBossUnlocked(const GameState& game,
                    const CampaignState& campaign,
                    const std::vector<BossDef>& bosses,
                    std::size_t bossIndex);
BossOutcome evaluateBossAttempt(const GameState& game,
                                CampaignState& campaign,
                                const std::vector<BossDef>& bosses,
                                std::size_t bossIndex,
                                const BossAttempt& attempt);
void applyBossOutcome(GameState& game, const BossOutcome& outcome);

} // namespace ed
