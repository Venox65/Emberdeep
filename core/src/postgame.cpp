#include "ed/postgame.hpp"

#include <algorithm>
#include <cmath>

namespace ed {

double echoDifficultyMultiplier(int echoRank) {
  return std::min(2.0, 1.0 + 0.08 * std::max(0, echoRank - 1));
}

DeepStrataBlock deepStrataBlock(int depth, std::int64_t baseHp, std::int64_t baseXp) {
  const int safeDepth = std::max(0, depth);
  const double depthValue = static_cast<double>(safeDepth);
  DeepStrataBlock block;
  block.depth = safeDepth;
  block.hp = static_cast<std::int64_t>(std::llround(baseHp * std::pow(2.6, depthValue)));
  block.xp = static_cast<std::int64_t>(std::llround(baseXp * std::pow(2.4, depthValue)));
  block.oreQuantity = std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(1.0 + 0.5 * depthValue)));
  block.aetherShardChanceBasisPoints = std::min(10000, safeDepth * 250);
  return block;
}

int calculateAscensionEmberMarks(std::int64_t goldEarnedThisRun, int echoRank) {
  const auto safeGold = std::max<std::int64_t>(0, goldEarnedThisRun);
  const int goldMarks = static_cast<int>(std::floor(std::sqrt(static_cast<double>(safeGold) / 50000.0)));
  return goldMarks + 2 * std::max(0, echoRank);
}

int applyAscension(GameState& game) {
  const int earnedMarks = calculateAscensionEmberMarks(game.player.goldEarnedThisRun, game.player.echoRank);
  ++game.player.ascensions;
  game.player.emberMarks += earnedMarks;
  game.player.level = 1;
  game.player.xp = 0;
  game.player.gold = 0;
  game.player.arcaneDust = 0;
  game.player.vowPoints = 0;
  game.player.blocksBroken = 0;
  game.player.clicks = 0;
  game.player.pickaxeTier = 1;
  game.player.currentLayer = "ashen-warrens";
  game.player.goldEarnedThisRun = 0;
  game.dirty = true;
  return earnedMarks;
}

} // namespace ed
