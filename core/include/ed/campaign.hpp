#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "ed/game_state.hpp"
#include "ed/stats.hpp"

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
  std::int64_t maxHp = 100;
  std::int64_t attack = 10;
  std::int64_t defense = 0;
  std::int64_t speed = 10;
  std::string abilityName = "Strike";
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

enum class BossCombatEventType {
  CombatStart,
  PlayerAttack,
  BossAttack,
  BossAbility,
  PlayerDefeated,
  BossDefeated
};

struct BossCombatEvent {
  BossCombatEventType type = BossCombatEventType::CombatStart;
  std::string description;
  std::int64_t damage = 0;
  std::int64_t playerHp = 0;
  std::int64_t bossHp = 0;
  bool isCrit = false;
};

struct BossCombatResult {
  bool victory = false;
  std::int64_t finalPlayerHp = 0;
  std::int64_t finalBossHp = 0;
  int rounds = 0;
  std::vector<BossCombatEvent> events;
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

BossDef getEffectiveBossDef(const BossDef& boss, bool isEcho = false, int echoRank = 1);

BossCombatResult simulateBossCombat(const PlayerStats& playerStats,
                                    const BossDef& boss,
                                    int minigameStars = 1,
                                    bool isEcho = false,
                                    int echoRank = 1,
                                    std::uint64_t seed = 0xB055ULL);

} // namespace ed
