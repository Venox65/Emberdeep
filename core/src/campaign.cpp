#include "ed/campaign.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ed {

namespace {

int adjustedThreshold(int threshold, double focus) {
  const double clampedFocus = std::clamp(focus, 0.0, 0.25);
  return std::max(1, static_cast<int>(std::ceil(threshold / (1.0 + clampedFocus))));
}

BossDef makeBoss(std::string id,
                 std::string name,
                 std::string layerId,
                 BossChallenge challenge,
                 int minLevel,
                 int clearScore,
                 int twoStarScore,
                 int threeStarScore,
                 std::string legendaryId) {
  BossDef boss;
  boss.id = std::move(id);
  boss.name = std::move(name);
  boss.layerId = std::move(layerId);
  boss.challenge = challenge;
  boss.minLevel = minLevel;
  boss.recommendedGearTier = minLevel == 1 ? 1 : std::min(10, (minLevel + 5) / 7);
  boss.clearScore = clearScore;
  boss.twoStarScore = twoStarScore;
  boss.threeStarScore = threeStarScore;
  boss.firstClearGold = 250 * (1 << std::min(9, static_cast<int>(boss.challenge)));
  boss.firstClearXp = 100 * (1 << std::min(9, static_cast<int>(boss.challenge)));
  boss.legendaryId = std::move(legendaryId);
  return boss;
}

} // namespace

std::vector<BossDef> defaultBosses() {
  return {
      makeBoss("grubnok", "Grubnok, the Burrow-King", "ashen-warrens",
               BossChallenge::FrenzyDig, 1, 5, 7, 9, "burrowkings-crown"),
      makeBoss("sir-rattlebone", "Sir Rattlebone", "copper-crypt",
               BossChallenge::BoneDuel, 5, 5, 7, 8, "bonecleaver-gauntlets"),
      makeBoss("morwenna", "Morwenna the Ashen Witch", "iron-hollows",
               BossChallenge::CinderDance, 10, 70, 85, 95, "cinderheart-amulet"),
      makeBoss("thessaly", "Thessaly, the Bog Hydra", "silvermire",
               BossChallenge::HeadHunt, 15, 30, 38, 45, "hydrascale-cuirass"),
      makeBoss("aldric", "Aldric, the Gilded Knight", "aurelite-keep",
               BossChallenge::ParryDuel, 20, 1, 1, 1, "sunforged-pickaxe"),
      makeBoss("isolde", "Isolde, Runewarden", "runic-archives",
               BossChallenge::RuneRecall, 27, 1, 1, 1, "runewardens-diadem"),
      makeBoss("seraphel", "Seraphel, the Fallen Star", "stargazer-spire",
               BossChallenge::StarTrace, 34, 8, 10, 12, "starfall-treads"),
      makeBoss("ignathia", "Ignathia, the Ember Wyrm", "wyrmbone-caverns",
               BossChallenge::Heartpierce, 42, 50, 70, 85, "wyrmfang-breaker"),
      makeBoss("xarmal", "Xar'Mal, the Lich Emperor", "voidglass-sanctum",
               BossChallenge::SoulDodge, 50, 80, 90, 100, "voidglass-phylactery"),
      makeBoss("hollow-king", "The Hollow King", "aether-vault",
               BossChallenge::FiveTrials, 60, 1, 2, 3, "aetherbrand"),
  };
}

CampaignState createCampaignState(std::size_t bossCount) {
  CampaignState campaign;
  campaign.bestStars.assign(bossCount, 0);
  campaign.cleared.assign(bossCount, false);
  return campaign;
}

bool isBossUnlocked(const GameState& game,
                    const CampaignState& campaign,
                    const std::vector<BossDef>& bosses,
                    std::size_t bossIndex) {
  if (bossIndex >= bosses.size()) {
    return false;
  }
  if (game.player.level < bosses[bossIndex].minLevel) {
    return false;
  }
  if (bossIndex == 0) {
    return true;
  }
  return bossIndex - 1 < campaign.cleared.size() && campaign.cleared[bossIndex - 1];
}

BossOutcome evaluateBossAttempt(const GameState& game,
                                CampaignState& campaign,
                                const std::vector<BossDef>& bosses,
                                std::size_t bossIndex,
                                const BossAttempt& attempt) {
  if (bossIndex >= bosses.size() || !isBossUnlocked(game, campaign, bosses, bossIndex)) {
    throw std::invalid_argument("boss is not unlocked");
  }

  const BossDef& boss = bosses[bossIndex];
  BossOutcome outcome;
  outcome.cleared =
      attempt.completed && attempt.score >= adjustedThreshold(boss.clearScore, attempt.focus);
  if (!outcome.cleared) {
    return outcome;
  }

  outcome.stars = 1;
  if (attempt.score >= adjustedThreshold(boss.twoStarScore, attempt.focus) && attempt.heartsLost <= 1) {
    outcome.stars = 2;
  }
  if (attempt.score >= adjustedThreshold(boss.threeStarScore, attempt.focus) &&
      attempt.heartsLost == 0) {
    outcome.stars = 3;
  }

  if (campaign.bestStars.size() < bosses.size()) {
    campaign.bestStars.resize(bosses.size(), 0);
  }
  if (campaign.cleared.size() < bosses.size()) {
    campaign.cleared.resize(bosses.size(), false);
  }
  outcome.firstClear = !campaign.cleared[bossIndex];
  campaign.cleared[bossIndex] = true;
  campaign.bestStars[bossIndex] = std::max(campaign.bestStars[bossIndex], outcome.stars);
  outcome.goldReward = outcome.firstClear ? boss.firstClearGold : boss.firstClearGold / 4;
  outcome.xpReward = outcome.firstClear ? boss.firstClearXp : boss.firstClearXp / 4;
  outcome.legendaryId = outcome.firstClear ? boss.legendaryId : "";
  return outcome;
}

void applyBossOutcome(GameState& game, const BossOutcome& outcome) {
  if (!outcome.cleared) {
    return;
  }
  game.player.gold += outcome.goldReward;
  game.player.goldEarnedThisRun += outcome.goldReward;
  applyXp(game, outcome.xpReward);
  if (outcome.firstClear) {
    ++game.player.bossesCleared;
    ++game.player.bossEssence;
  }
  game.dirty = true;
}

} // namespace ed
