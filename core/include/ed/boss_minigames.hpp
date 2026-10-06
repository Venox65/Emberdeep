#pragma once

#include <cstdint>
#include <vector>

#include "ed/pcg.hpp"

namespace ed {

struct FrenzyDigState {
  float elapsed = 0.0F;
  int taps = 0;
  int nuggetTaps = 0;
  float nuggetX = 0.5F;
  float nuggetY = 0.5F;
  bool nuggetVisible = false;
  int nuggetSpawns = 0;
  bool finished = false;
};

FrenzyDigState startFrenzyDig(std::uint64_t seed);
void updateFrenzyDig(FrenzyDigState& state, float deltaSeconds, Pcg32& rng);
bool tapFrenzyDig(FrenzyDigState& state, float x, float y, Pcg32& rng);
int frenzyDigScore(const FrenzyDigState& state, int attack);

struct CinderNote {
  float time = 0.0F;
  int lane = 0;
  bool judged = false;
};

struct CinderDanceState {
  float elapsed = 0.0F;
  int groove = 50;
  int perfectHits = 0;
  int goodHits = 0;
  int misses = 0;
  std::size_t nextNote = 0;
  bool finished = false;
  bool failed = false;
  std::vector<CinderNote> chart;
};

CinderDanceState startCinderDance(std::uint64_t seed = 0xC1DEULL);
void updateCinderDance(CinderDanceState& state, float deltaSeconds);
int judgeCinderDance(CinderDanceState& state, int lane, double focus);
int cinderDanceScore(const CinderDanceState& state, int attack);

struct BoneDuelState {
  int round = 0;
  int misses = 0;
  int perfectHits = 0;
  float needle = 0.0F;
  float direction = 1.0F;
  float elapsed = 0.0F;
  std::uint32_t sideState = 0;
  bool startsFromRight = false;
  bool finished = false;
  bool cleared = false;
};

BoneDuelState startBoneDuel(std::uint64_t seed = 0xB00EULL);
void updateBoneDuel(BoneDuelState& state, float deltaSeconds);
bool strikeBoneDuel(BoneDuelState& state, double focus);
float boneDuelZoneWidth(const BoneDuelState& state, double focus);
int boneDuelScore(const BoneDuelState& state, int attack, int pierce);

struct HeadHuntTarget {
  int cell = 0;
  bool real = true;
  float remaining = 0.0F;
};

struct HeadHuntState {
  float elapsed = 0.0F;
  int realHits = 0;
  int decoyHits = 0;
  int missedReal = 0;
  int heartsLost = 0;
  bool finished = false;
  std::vector<HeadHuntTarget> targets;
};

HeadHuntState startHeadHunt(std::uint64_t seed = 0x1EADULL);
void updateHeadHunt(HeadHuntState& state, float deltaSeconds, Pcg32& rng, double speed);
bool hitHeadHunt(HeadHuntState& state, int cell);
int headHuntScore(const HeadHuntState& state, int attack);

struct ParryDuelState {
  int attack = 0;
  int successfulParries = 0;
  int heartsLost = 0;
  int direction = 0;
  bool feint = false;
  float elapsed = 0.0F;
  std::uint32_t patternState = 0;
  bool finished = false;
  bool cleared = false;
};

ParryDuelState startParryDuel(std::uint64_t seed = 0xA1D1CULL);
void updateParryDuel(ParryDuelState& state, float deltaSeconds, Pcg32& rng);
bool parryDuelInput(ParryDuelState& state, int direction);
int parryDuelScore(const ParryDuelState& state, int attack);

struct RuneRecallState {
  int round = 1;
  int mistakes = 0;
  int inputIndex = 0;
  float showElapsed = 0.0F;
  float showDuration = 1.25F;
  std::uint32_t patternState = 0;
  std::vector<int> sequence;
  bool showing = true;
  bool finished = false;
  bool cleared = false;
};

RuneRecallState startRuneRecall(std::uint64_t seed = 0x7A11ULL);
void updateRuneRecall(RuneRecallState& state, float deltaSeconds, Pcg32& rng);
bool inputRuneRecall(RuneRecallState& state, int rune);
int runeRecallScore(const RuneRecallState& state, int focus);

struct StarTraceState {
  float elapsed = 0.0F;
  int constellation = 0;
  int targetIndex = 0;
  int heartsLost = 0;
  std::vector<int> stars;
  std::vector<float> starX;
  std::vector<float> starY;
  std::uint32_t patternState = 0;
  bool finished = false;
  bool cleared = false;
};

StarTraceState startStarTrace(std::uint64_t seed = 0x57A2ULL);
void updateStarTrace(StarTraceState& state, float deltaSeconds, Pcg32& rng);
bool hitStarTrace(StarTraceState& state, int star);
int starTraceScore(const StarTraceState& state, int speed);

struct WyrmfireRunState {
  float elapsed = 0.0F;
  int lane = 1;
  int heartsLost = 0;
  int embers = 0;
  int burstTaps = 0;
  int hazardLane = 1;
  float hazardY = 0.0F;
  bool hazardActive = true;
  float hazardRespawn = 0.0F;
  int emberLane = 0;
  float emberY = 0.0F;
  bool emberActive = true;
  float emberRespawn = 0.0F;
  float nextScaleBreak = 15.0F;
  bool scaleBreak = false;
  float scaleElapsed = 0.0F;
  bool finished = false;
  bool cleared = false;
};

WyrmfireRunState startWyrmfireRun();
void updateWyrmfireRun(WyrmfireRunState& state, float deltaSeconds);
void moveWyrmfireRun(WyrmfireRunState& state, int direction);
void tapWyrmfireRun(WyrmfireRunState& state);
int wyrmfireRunScore(const WyrmfireRunState& state, int speed);

} // namespace ed
