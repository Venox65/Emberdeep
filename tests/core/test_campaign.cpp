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

TEST_CASE("boss combat enforces gear check: undergeared player fails, geared player wins") {
  const auto bosses = ed::defaultBosses();
  const auto& aldric = bosses[4]; // Boss 5: Aldric, HP 3600, Atk 260, Def 150

  ed::PlayerStats starterStats;
  starterStats.maxHp = 120;
  starterStats.attack = 15;
  starterStats.defense = 4;
  starterStats.speed = 10;

  const auto undergearedResult = ed::simulateBossCombat(starterStats, aldric, 1, false, 1, 0x1234ULL);
  REQUIRE_FALSE(undergearedResult.victory);
  REQUIRE(undergearedResult.finalPlayerHp <= 0);
  REQUIRE(undergearedResult.finalBossHp > 0);

  ed::PlayerStats gearedStats;
  gearedStats.maxHp = 6000;
  gearedStats.attack = 500;
  gearedStats.defense = 250;
  gearedStats.speed = 30;
  gearedStats.critChanceBasisPoints = 1500; // 15%
  gearedStats.critDamageBasisPoints = 15000;

  const auto gearedResult = ed::simulateBossCombat(gearedStats, aldric, 1, false, 1, 0x1234ULL);
  REQUIRE(gearedResult.victory);
  REQUIRE(gearedResult.finalPlayerHp > 0);
  REQUIRE(gearedResult.finalBossHp == 0);
}

TEST_CASE("boss combat minigame stars provide combat advantages") {
  const auto bosses = ed::defaultBosses();
  const auto& morwenna = bosses[2]; // Boss 3: Morwenna, HP 620, Atk 50, Def 28

  ed::PlayerStats midStats;
  midStats.maxHp = 800;
  midStats.attack = 65;
  midStats.defense = 35;
  midStats.speed = 15;

  const auto oneStar = ed::simulateBossCombat(midStats, morwenna, 1, false, 1, 0x9999ULL);
  const auto threeStar = ed::simulateBossCombat(midStats, morwenna, 3, false, 1, 0x9999ULL);

  REQUIRE(threeStar.victory);
  REQUIRE(threeStar.rounds <= oneStar.rounds);
  REQUIRE(threeStar.finalPlayerHp >= oneStar.finalPlayerHp);
}

TEST_CASE("echo boss combat scales difficulty and requires enhanced stats") {
  const auto bosses = ed::defaultBosses();
  const auto& hollowKing = bosses[9]; // Boss 10: Hollow King

  // Standard endgame stats can defeat standard Hollow King
  ed::PlayerStats t10Stats;
  t10Stats.maxHp = 300000;
  t10Stats.attack = 36000;
  t10Stats.defense = 18000;
  t10Stats.speed = 35;

  const auto standardResult = ed::simulateBossCombat(t10Stats, hollowKing, 2, false, 1, 0xABCDULL);
  REQUIRE(standardResult.victory);

  // But the same stats struggle or fail against Echo Rank 3
  const auto echo3Result = ed::simulateBossCombat(t10Stats, hollowKing, 2, true, 3, 0xABCDULL);
  REQUIRE_FALSE(echo3Result.victory);

  // Mythic-upgraded stats (+15/+20 with gems) defeat Echo Rank 3
  ed::PlayerStats mythicStats;
  mythicStats.maxHp = 700000;
  mythicStats.attack = 80000;
  mythicStats.defense = 45000;
  mythicStats.speed = 50;
  mythicStats.critChanceBasisPoints = 2500;
  mythicStats.critDamageBasisPoints = 18000;

  const auto mythicResult = ed::simulateBossCombat(mythicStats, hollowKing, 3, true, 3, 0xABCDULL);
  REQUIRE(mythicResult.victory);
}

TEST_CASE("getEffectiveBossDef scales echo bosses to endgame stats while keeping standard bosses unchanged") {
  const auto bosses = ed::defaultBosses();
  const auto& grubnok = bosses[0];
  const auto& hollowKing = bosses[9];

  // Standard bosses retain untouched baseline stats
  const auto stdGrubnok = ed::getEffectiveBossDef(grubnok, false, 1);
  CHECK(stdGrubnok.maxHp == 110);
  CHECK(stdGrubnok.attack == 10);
  CHECK(stdGrubnok.defense == 4);

  const auto stdHollowKing = ed::getEffectiveBossDef(hollowKing, false, 1);
  CHECK(stdHollowKing.maxHp == 280000);
  CHECK(stdHollowKing.attack == 16500);
  CHECK(stdHollowKing.defense == 9200);

  // Echo Boss 1 starts at endgame Tier 10 baseline
  const auto echoGrubnokR1 = ed::getEffectiveBossDef(grubnok, true, 1);
  CHECK(echoGrubnokR1.maxHp >= 300000);
  CHECK(echoGrubnokR1.attack >= 14000);
  CHECK(echoGrubnokR1.defense >= 8000);

  // Echo Boss 10 scales higher than Echo Boss 1
  const auto echoHollowKingR1 = ed::getEffectiveBossDef(hollowKing, true, 1);
  CHECK(echoHollowKingR1.maxHp > echoGrubnokR1.maxHp);
  CHECK(echoHollowKingR1.attack > echoGrubnokR1.attack);
  CHECK(echoHollowKingR1.defense > echoGrubnokR1.defense);

  // Higher echo ranks scale stats up
  const auto echoGrubnokR3 = ed::getEffectiveBossDef(grubnok, true, 3);
  CHECK(echoGrubnokR3.maxHp > echoGrubnokR1.maxHp);
  CHECK(echoGrubnokR3.attack > echoGrubnokR1.attack);
}
