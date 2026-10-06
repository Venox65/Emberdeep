#include "ed/boss_minigames.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Frenzy Dig is deterministic and rewards a golden nugget") {
  auto first = ed::startFrenzyDig(42);
  auto second = ed::startFrenzyDig(42);
  REQUIRE(first.nuggetX == second.nuggetX);
  REQUIRE(first.nuggetY == second.nuggetY);

  ed::Pcg32 rng(42);
  REQUIRE(ed::tapFrenzyDig(first, first.nuggetX, first.nuggetY, rng));
  REQUIRE(first.taps == 6);
  ed::updateFrenzyDig(first, 10.0F, rng);
  REQUIRE(first.finished);
  REQUIRE(ed::frenzyDigScore(first, 10) == 0);
}

TEST_CASE("Bone Duel advances eight rounds and counts misses and perfect hits") {
  auto duel = ed::startBoneDuel(1);
  const auto initialSideState = duel.sideState;
  REQUIRE(duel.needle == (duel.startsFromRight ? 1.0F : 0.0F));
  REQUIRE(duel.round == 1);
  for (int round = 0; round < 8; ++round) {
    duel.needle = 0.5F;
    REQUIRE(ed::strikeBoneDuel(duel, 0.0));
    if (round == 0) REQUIRE(duel.sideState != initialSideState);
  }
  REQUIRE(duel.finished);
  REQUIRE(duel.cleared);
  REQUIRE(duel.misses == 0);
  REQUIRE(duel.perfectHits == 8);
  REQUIRE(ed::boneDuelScore(duel, 10, 0) == 8);
}

TEST_CASE("Cinder Dance uses a deterministic 110-note chart and groove") {
  auto first = ed::startCinderDance(77);
  auto second = ed::startCinderDance(77);
  REQUIRE(first.chart.size() == 110);
  for (std::size_t index = 0; index < first.chart.size(); ++index) {
    REQUIRE(first.chart[index].time == second.chart[index].time);
    REQUIRE(first.chart[index].lane == second.chart[index].lane);
  }

  first.elapsed = first.chart.front().time;
  REQUIRE(ed::judgeCinderDance(first, first.chart.front().lane, 0.0) == 2);
  REQUIRE(first.perfectHits == 1);
  REQUIRE(first.groove == 52);

  ed::updateCinderDance(first, 75.0F);
  REQUIRE(first.finished);
  REQUIRE(first.failed);
}

TEST_CASE("Head Hunt distinguishes real heads from decoys") {
  auto hunt = ed::startHeadHunt(12);
  REQUIRE(hunt.targets.size() == 1);
  const int cell = hunt.targets.front().cell;
  const bool real = hunt.targets.front().real;
  REQUIRE(ed::hitHeadHunt(hunt, cell) == real);
  if (real) {
    REQUIRE(hunt.realHits == 1);
  } else {
    REQUIRE(hunt.heartsLost == 1);
  }
}

TEST_CASE("Parry Duel ends immediately after three failed parries") {
  auto duel = ed::startParryDuel(9);
  REQUIRE_FALSE(duel.finished);
  const int firstDirection = duel.direction;
  REQUIRE(ed::parryDuelInput(duel, firstDirection));
  REQUIRE(duel.direction != firstDirection);
  while (duel.heartsLost < 3) {
    const int wrongDirection = duel.feint ? 0 : (duel.direction + 1) % 3;
    ed::parryDuelInput(duel, wrongDirection);
  }
  REQUIRE(duel.finished);
  REQUIRE_FALSE(duel.cleared);
  REQUIRE(duel.heartsLost == 3);
}

TEST_CASE("Rune Recall randomizes sequences and caps them at eight nodes") {
  auto runes = ed::startRuneRecall(9);
  ed::Pcg32 rng(9);
  REQUIRE(runes.showDuration == 1.25F);
  ed::updateRuneRecall(runes, runes.showDuration, rng);
  for (int round = 1; round <= 10; ++round) {
    const auto sequence = runes.sequence;
    REQUIRE(sequence.size() <= 8);
    for (const int rune : sequence) REQUIRE(ed::inputRuneRecall(runes, rune));
    if (round < 10) {
      REQUIRE(runes.showDuration == 1.25F + 0.5F * static_cast<float>(round));
      ed::updateRuneRecall(runes, runes.showDuration, rng);
    }
  }
  REQUIRE(runes.finished);
  REQUIRE(runes.cleared);
  REQUIRE(runes.round == 10);
}

TEST_CASE("Star Trace and Wyrmfire Run expose deterministic playable state") {
  auto stars = ed::startStarTrace(4);
  for (int index = 0; index < 8; ++index) {
    const auto current = stars.stars;
    if (index >= 3) REQUIRE(current.size() >= 5);
    for (const int star : current) REQUIRE(ed::hitStarTrace(stars, star));
  }
  REQUIRE(stars.constellation >= 8);

  auto run = ed::startWyrmfireRun();
  ed::moveWyrmfireRun(run, -1);
  REQUIRE(run.lane == 0);
  ed::moveWyrmfireRun(run, 1);
  REQUIRE(run.lane == 1);
  ed::moveWyrmfireRun(run, 1);
  REQUIRE(run.lane == 2);
  ed::moveWyrmfireRun(run, 1);
  REQUIRE(run.lane == 0);
  ed::moveWyrmfireRun(run, 1);
  REQUIRE(run.lane == 1);
  ed::updateWyrmfireRun(run, 4.0F);
  REQUIRE(run.heartsLost == 1);
  REQUIRE_FALSE(run.hazardActive);
  for (int index = 0; index < 25; ++index) ed::tapWyrmfireRun(run);
  REQUIRE(run.burstTaps == 0);
  ed::updateWyrmfireRun(run, 45.0F);
  REQUIRE(run.finished);
}
