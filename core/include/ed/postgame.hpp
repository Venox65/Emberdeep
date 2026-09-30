#pragma once

#include <cstdint>

#include "ed/game_state.hpp"

namespace ed {

struct DeepStrataBlock {
  int depth = 0;
  std::int64_t hp = 0;
  std::int64_t xp = 0;
  std::int64_t oreQuantity = 0;
  int aetherShardChanceBasisPoints = 0;
};

double echoDifficultyMultiplier(int echoRank);
DeepStrataBlock deepStrataBlock(int depth, std::int64_t baseHp = 108700, std::int64_t baseXp = 13200);
int calculateAscensionEmberMarks(std::int64_t goldEarnedThisRun, int echoRank);
int applyAscension(GameState& game);

} // namespace ed
