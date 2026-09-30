#include "ed/campaign.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("campaign bosses unlock in sequence and reward a first clear") {
  const auto bosses = ed::defaultBosses();
  auto campaign = ed::createCampaignState(bosses.size());
  ed::GameState game;
  game.player.level = 1;

  REQUIRE(ed::isBossUnlocked(game, campaign, bosses, 0));
  REQUIRE_FALSE(ed::isBossUnlocked(game, campaign, bosses, 1));

  const ed::BossAttempt attempt{9, 0, true};
  const auto outcome = ed::evaluateBossAttempt(game, campaign, bosses, 0, attempt);
  REQUIRE(outcome.cleared);
  REQUIRE(outcome.stars == 3);
  REQUIRE(outcome.firstClear);
  REQUIRE_FALSE(outcome.legendaryId.empty());
  REQUIRE(campaign.cleared[0]);
  REQUIRE(campaign.bestStars[0] == 3);

  ed::applyBossOutcome(game, outcome);
  REQUIRE(game.player.bossesCleared == 1);
  REQUIRE(game.player.bossEssence == 1);
  REQUIRE(game.player.gold == outcome.goldReward);
}

TEST_CASE("repeat campaign clears award reduced rewards and preserve best stars") {
  const auto bosses = ed::defaultBosses();
  auto campaign = ed::createCampaignState(bosses.size());
  campaign.cleared[0] = true;
  campaign.bestStars[0] = 3;
  ed::GameState game;

  const auto outcome = ed::evaluateBossAttempt(game, campaign, bosses, 0, {5, 2, true});
  REQUIRE(outcome.cleared);
  REQUIRE(outcome.stars == 1);
  REQUIRE_FALSE(outcome.firstClear);
  REQUIRE(outcome.legendaryId.empty());
  REQUIRE(campaign.bestStars[0] == 3);
}
