#pragma once

#include <string>
#include <vector>

#include "ed/data_model.hpp"
#include "ed/formulas.hpp"

namespace ed {

struct PlayerState {
  int level = 1;
  int xp = 0;
  int gold = 0;
  int arcaneDust = 0;
  int bossEssence = 0;
  int echoSigils = 0;
  int aetherShards = 0;
  int emberMarks = 0;
  int vowPoints = 0;
  int blocksBroken = 0;
  int clicks = 0;
  int bossesCleared = 0;
  int ascensions = 0;
  int echoRank = 0;
  int deepestDepth = 0;
  int goldEarnedThisRun = 0;
  int pickaxeTier = 1;
  std::string currentLayer = "ashen-warrens";
  std::string lastGemHollowDate = "";
  int gemHollowStreak = 0;
  int gemHollowSwingsRemaining = 0;
  int gemHollowTodayGems = 0;
  int gemHollowTodayDust = 0;
  int gemHollowTodayGold = 0;
  bool gemHollowActive = false;
  std::vector<int> gemHollowVeins;


};

struct GameState {
  int schemaVersion = 1;
  PlayerState player;
  bool dirty = false;
  std::string saveId = "emberdeep-save";
};

bool canUnlockLayer(const GameState& game, const LayerDef& layer);
void applyXp(GameState& game, int earnedXp);
std::string serializeGameState(const GameState& game);
bool deserializeGameState(const std::string& text, GameState& game);

} // namespace ed
