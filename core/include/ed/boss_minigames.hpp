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
  bool gauntletMode = false;
  float duration = 75.0F;
  int feedback = 0;
  int feedbackLane = 0;
  float feedbackTimer = 0.0F;
  std::vector<CinderNote> chart;
};

CinderDanceState startCinderDance(std::uint64_t seed = 0xC1DEULL,
                                  float duration = 75.0F,
                                  int noteCount = 110);
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
  int maxAttacks = 24;
  int successfulParries = 0;
  int heartsLost = 0;
  int direction = 0;
  bool feint = false;
  float elapsed = 0.0F;
  std::uint32_t patternState = 0;
  bool finished = false;
  bool cleared = false;
};

ParryDuelState startParryDuel(std::uint64_t seed = 0xA1D1CULL,
                              int maxAttacks = 24);
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

struct HeartpierceState {
  float totalTime = 0.0F;
  float overlapTime = 0.0F;
  float lanceY = 0.5F;
  float lanceV = 0.0F;
  float heartY = 0.5F;
  float waypointY = 0.5F;
  float retargetTimer = 2.4F;
  float gauge = 0.25F;
  float invulnerable = 0.0F;
  float dartTimer = 5.0F;
  float dartTelegraph = 0.0F;
  float dartTarget = 0.5F;
  float focus = 0.0F;
  float echoModifier = 1.0F;
  int phase = 0;
  int hearts = 3;
  int heartsLost = 0;
  std::uint32_t patternState = 0;
  bool finished = false;
  bool cleared = false;
};

HeartpierceState startHeartpierce(std::uint64_t seed = 0x1A671ULL,
                                  float focus = 0.0F, int ward = 0,
                                  int echoRank = 1);
void updateHeartpierce(HeartpierceState& state, float deltaSeconds, bool rising);
int heartpierceScore(const HeartpierceState& state);

enum class SoulHazardKind { Spiral, Rain, Laser, Ring };

struct SoulHazard {
  SoulHazardKind kind = SoulHazardKind::Spiral;
  float x = 0.5F;
  float y = 0.5F;
  float angle = 0.0F;
  float radius = 0.0F;
  float lifetime = 0.0F;
  float telegraph = 0.0F;
  bool active = true;
};

struct SoulDodgeState {
  float elapsed = 0.0F;
  float duration = 60.0F;
  float playerX = 0.5F;
  float playerY = 0.5F;
  int hearts = 3;
  int hits = 0;
  float invulnerable = 0.0F;
  float patternTimer = 0.0F;
  std::uint32_t patternState = 0;
  std::vector<SoulHazard> hazards;
  bool slowMove = false;
  bool finished = false;
  bool cleared = false;
};

SoulDodgeState startSoulDodge(std::uint64_t seed = 0x50UL, int ward = 0,
                              float duration = 60.0F);
void updateSoulDodge(SoulDodgeState& state, float deltaSeconds,
                     float moveX, float moveY, bool slowMove, int speed);
int soulDodgeScore(const SoulDodgeState& state);

struct CrownStrikeState {
  int strike = 0;
  int hits = 0;
  int misses = 0;
  float needle = 0.0F;
  float direction = 1.0F;
  bool finished = false;
  bool cleared = false;
};

CrownStrikeState startCrownStrike();
void updateCrownStrike(CrownStrikeState& state, float deltaSeconds);
bool strikeCrown(CrownStrikeState& state, float focus);
int crownStrikeScore(const CrownStrikeState& state);

struct FiveTrialsState {
  int phase = 1;
  float phaseElapsed = 0.0F;
  float burstElapsed = 0.0F;
  int burstTaps = 0;
  int hearts = 3;
  int maxHearts = 3;
  int heartsLost = 0;
  bool restoredBetweenPhases = false;
  bool finished = false;
  bool cleared = false;
};

FiveTrialsState startFiveTrials(int ward = 0);
bool advanceFiveTrials(FiveTrialsState& state, bool phaseCleared,
                       int phaseHeartsLost);
int fiveTrialsScore(const FiveTrialsState& state);

} // namespace ed
