#include "ed/boss_minigames.hpp"

#include <algorithm>
#include <cmath>

namespace ed {

namespace {

constexpr float kFrenzyDuration = 10.0F;
constexpr int kBoneRounds = 8;
constexpr float kHeadHuntDuration = 35.0F;
constexpr int kRuneRounds = 10;

float focusMultiplier(double focus) {
  return 1.0F + static_cast<float>(std::clamp(focus, 0.0, 0.25));
}

float cinderAccuracy(const CinderDanceState& state) {
  const int judged = state.perfectHits + state.goodHits + state.misses;
  if (judged == 0) return 0.0F;
  return static_cast<float>(state.perfectHits + 0.6F * state.goodHits) /
         static_cast<float>(judged);
}

void spawnHead(HeadHuntState& state, Pcg32& rng, double speed) {
  if (state.targets.size() >= 2) return;
  HeadHuntTarget target;
  target.cell = static_cast<int>(rng.nextRange(0, 8));
  target.real = rng.nextRange(0, 99) >= 30;
  const float baseLifetime = target.real ? 1.4F : 1.0F;
  target.remaining = baseLifetime + static_cast<float>(std::clamp(speed, 0.0, 0.25));
  state.targets.push_back(target);
}

} // namespace

FrenzyDigState startFrenzyDig(std::uint64_t seed) {
  Pcg32 rng(seed);
  FrenzyDigState state;
  const bool right = rng.nextRange(0, 1) != 0;
  const bool bottom = rng.nextRange(0, 1) != 0;
  state.nuggetX = static_cast<float>(rng.nextRange(right ? 75 : 5, right ? 95 : 25)) / 100.0F;
  state.nuggetY = static_cast<float>(rng.nextRange(bottom ? 70 : 5, bottom ? 95 : 25)) / 100.0F;
  state.nuggetVisible = true;
  state.nuggetSpawns = 1;
  return state;
}

void updateFrenzyDig(FrenzyDigState& state, float deltaSeconds, Pcg32& rng) {
  if (state.finished) return;
  state.elapsed = std::min(kFrenzyDuration, state.elapsed + std::max(0.0F, deltaSeconds));
  if (state.elapsed >= kFrenzyDuration) {
    state.finished = true;
    state.nuggetVisible = false;
  } else if (!state.nuggetVisible && state.nuggetSpawns < 2 &&
             rng.nextRange(0, 99) < 3) {
    const bool right = rng.nextRange(0, 1) != 0;
    const bool bottom = rng.nextRange(0, 1) != 0;
    state.nuggetX = static_cast<float>(rng.nextRange(right ? 75 : 5, right ? 95 : 25)) / 100.0F;
    state.nuggetY = static_cast<float>(rng.nextRange(bottom ? 70 : 5, bottom ? 95 : 25)) / 100.0F;
    state.nuggetVisible = true;
    ++state.nuggetSpawns;
  }
}

bool tapFrenzyDig(FrenzyDigState& state, float x, float y, Pcg32& rng) {
  if (state.finished) return false;
  ++state.taps;
  if (state.nuggetVisible && std::abs(x - state.nuggetX) <= 0.07F &&
      std::abs(y - state.nuggetY) <= 0.07F) {
    state.taps += 5;
    ++state.nuggetTaps;
    state.nuggetVisible = false;
    const bool right = rng.nextRange(0, 1) != 0;
    const bool bottom = rng.nextRange(0, 1) != 0;
    state.nuggetX = static_cast<float>(rng.nextRange(right ? 75 : 5, right ? 95 : 25)) / 100.0F;
    state.nuggetY = static_cast<float>(rng.nextRange(bottom ? 70 : 5, bottom ? 95 : 25)) / 100.0F;
    return true;
  }

  return false;
}

int frenzyDigScore(const FrenzyDigState& state, int attack) {
  const int effectiveTaps = state.taps + std::max(0, attack) / 10;
  return static_cast<int>(std::floor(effectiveTaps / kFrenzyDuration));
}

CinderDanceState startCinderDance(std::uint64_t seed, float duration, int noteCount) {
  Pcg32 rng(seed);
  CinderDanceState state;
  state.duration = std::max(1.0F, duration);
  const int count = std::max(1, noteCount);
  state.chart.reserve(static_cast<std::size_t>(count));
  for (int index = 0; index < count; ++index) {
    CinderNote note;
    note.time = 2.0F + static_cast<float>(index) *
                              ((state.duration - 5.0F) /
                               static_cast<float>(std::max(1, count - 1)));
    note.lane = static_cast<int>(rng.nextRange(0, 3));
    state.chart.push_back(note);
  }
  return state;
}

void updateCinderDance(CinderDanceState& state, float deltaSeconds) {
  if (state.finished) return;
  const float delta = std::max(0.0F, deltaSeconds);
  state.elapsed = std::min(state.duration, state.elapsed + delta);
  state.feedbackTimer = std::max(0.0F, state.feedbackTimer - delta);
  if (state.feedbackTimer <= 0.0F) state.feedback = 0;
  while (state.nextNote < state.chart.size() &&
         state.elapsed - state.chart[state.nextNote].time > 0.1F) {
    state.feedback = -1;
    state.feedbackLane = state.chart[state.nextNote].lane;
    state.feedbackTimer = 0.55F;
    state.chart[state.nextNote].judged = true;
    ++state.nextNote;
    ++state.misses;
    state.groove = std::max(0, state.groove - 6);
  }
  if ((!state.gauntletMode && state.groove == 0) ||
      state.elapsed >= state.duration) {
    state.finished = true;
    state.failed = state.groove == 0;
    while (state.nextNote < state.chart.size()) {
      if (!state.chart[state.nextNote].judged) {
        ++state.misses;
        state.chart[state.nextNote].judged = true;
      }
      ++state.nextNote;
    }
  }
}

int judgeCinderDance(CinderDanceState& state, int lane, double focus) {
  if (state.finished || lane < 0 || lane > 3) return 0;
  state.feedbackLane = lane;
  state.feedbackTimer = 0.55F;
  const float window = 0.1F * focusMultiplier(focus);
  std::size_t candidate = state.nextNote;
  while (candidate < state.chart.size() && state.chart[candidate].judged) ++candidate;
  if (candidate >= state.chart.size() || state.chart[candidate].lane != lane ||
      std::abs(state.elapsed - state.chart[candidate].time) > window) {
    state.feedback = -1;
    return 0;
  }
  const float distance = std::abs(state.elapsed - state.chart[candidate].time);
  state.chart[candidate].judged = true;
  state.nextNote = candidate + 1;
  if (distance <= 0.045F * focusMultiplier(focus)) {
    state.feedback = 2;
    ++state.perfectHits;
    state.groove = std::min(100, state.groove + 2);
    return 2;
  }
  ++state.goodHits;
  state.feedback = 1;
  state.groove = std::min(100, state.groove + 2);
  return 1;
}

int cinderDanceScore(const CinderDanceState& state, int attack) {
  const int judged = state.perfectHits + state.goodHits + state.misses;
  if (state.failed || judged == 0) return 0;
  const int accuracyBasisPoints = static_cast<int>(
      10000.0 * (state.perfectHits + 0.6 * state.goodHits) / judged);
  return accuracyBasisPoints + std::min(500, std::max(0, attack) * 5);
}

BoneDuelState startBoneDuel(std::uint64_t seed) {
  BoneDuelState state;
  state.round = 1;
  Pcg32 rng(seed);
  state.sideState = rng.next();
  state.startsFromRight = (state.sideState & 1U) != 0;
  state.needle = state.startsFromRight ? 1.0F : 0.0F;
  state.direction = state.startsFromRight ? -1.0F : 1.0F;
  return state;
}

void updateBoneDuel(BoneDuelState& state, float deltaSeconds) {
  if (state.finished) return;
  const float safeDelta = std::max(0.0F, deltaSeconds);
  const float speed = 0.65F + 0.07F * static_cast<float>(state.round - 1);
  state.needle += state.direction * speed * safeDelta;
  if (state.needle >= 1.0F) {
    state.needle = 1.0F;
    state.direction = -1.0F;
  } else if (state.needle <= 0.0F) {
    state.needle = 0.0F;
    state.direction = 1.0F;
  }
  state.elapsed += safeDelta;
}

float boneDuelZoneWidth(const BoneDuelState& state, double focus) {
  const float base = 0.30F - 0.22F * static_cast<float>(state.round - 1) / 7.0F;
  return std::min(1.0F, base * focusMultiplier(focus));
}

bool strikeBoneDuel(BoneDuelState& state, double focus) {
  if (state.finished) return false;
  const float width = boneDuelZoneWidth(state, focus);
  const float center = 0.5F;
  const bool hit = std::abs(state.needle - center) <= width / 2.0F;
  const float perfectWidth = width * 0.35F;
  if (hit) {
    if (std::abs(state.needle - center) <= perfectWidth / 2.0F) ++state.perfectHits;
  } else {
    ++state.misses;
    if (state.misses >= 3) {
      state.finished = true;
      state.cleared = false;
      return false;
    }
  }
  if (state.round == kBoneRounds) {
    state.finished = true;
    state.cleared = true;
  } else {
    ++state.round;
    state.sideState = state.sideState * 1664525U + 1013904223U;
    state.startsFromRight = (state.sideState & 1U) != 0;
    state.needle = state.startsFromRight ? 1.0F : 0.0F;
    state.direction = state.startsFromRight ? -1.0F : 1.0F;
    state.elapsed = 0.0F;
  }
  return hit;
}

int boneDuelScore(const BoneDuelState& state, int attack, int pierce) {
  const int gearBonus = std::min(2, std::max(0, attack + pierce) / 40);
  return std::max(0, (kBoneRounds - state.misses) + gearBonus);
}

HeadHuntState startHeadHunt(std::uint64_t seed) {
  HeadHuntState state;
  Pcg32 rng(seed);
  spawnHead(state, rng, 0.0);
  return state;
}

void updateHeadHunt(HeadHuntState& state, float deltaSeconds, Pcg32& rng, double speed) {
  if (state.finished) return;
  const float safeDelta = std::max(0.0F, deltaSeconds);
  state.elapsed = std::min(kHeadHuntDuration, state.elapsed + safeDelta);
  for (auto& target : state.targets) target.remaining -= safeDelta;
  for (const auto& target : state.targets) {
    if (target.remaining <= 0.0F && target.real) ++state.missedReal;
  }
  state.targets.erase(std::remove_if(state.targets.begin(), state.targets.end(),
                                     [](const HeadHuntTarget& target) {
                                       return target.remaining <= 0.0F;
                                     }),
                      state.targets.end());
  if (state.targets.size() < 2 && rng.nextRange(0, 99) < 12) {
    spawnHead(state, rng, speed);
  }
  if (state.elapsed >= kHeadHuntDuration) state.finished = true;
}

bool hitHeadHunt(HeadHuntState& state, int cell) {
  if (state.finished || cell < 0 || cell > 8) return false;
  const auto target = std::find_if(state.targets.begin(), state.targets.end(),
                                   [cell](const HeadHuntTarget& item) {
                                     return item.cell == cell;
                                   });
  if (target == state.targets.end()) return false;
  const bool real = target->real;
  if (real) {
    ++state.realHits;
  } else {
    ++state.decoyHits;
    ++state.heartsLost;
    if (state.heartsLost >= 3) state.finished = true;
  }
  state.targets.erase(target);
  return real;
}

int headHuntScore(const HeadHuntState& state, int attack) {
  if (state.realHits < 30 || state.heartsLost >= 3) return 0;
  return state.realHits + std::min(5, std::max(0, attack) / 20);
}

ParryDuelState startParryDuel(std::uint64_t seed, int maxAttacks) {
  ParryDuelState state;
  state.maxAttacks = std::max(1, maxAttacks);
  Pcg32 rng(seed);
  state.patternState = rng.next();
  state.direction = static_cast<int>(state.patternState % 3U);
  state.patternState = state.patternState * 1664525U + 1013904223U;
  state.feint = state.patternState % 100U < 15U;
  return state;
}

void advanceParryPattern(ParryDuelState& state) {
  state.patternState = state.patternState * 1664525U + 1013904223U;
  state.direction = static_cast<int>(state.patternState % 3U);
  state.patternState = state.patternState * 1664525U + 1013904223U;
  state.feint = state.attack >= 12 && state.patternState % 100U < 25U;
}

void updateParryDuel(ParryDuelState& state, float deltaSeconds, Pcg32& rng) {
  if (state.finished) return;
  state.elapsed += std::max(0.0F, deltaSeconds);
  const float window = std::max(0.35F, 0.6F - 0.25F * state.attack / 23.0F);
  if (state.elapsed >= window) {
    if (!state.feint) ++state.heartsLost;
    ++state.attack;
    state.elapsed = 0.0F;
    if (state.heartsLost >= 3 || state.attack >= state.maxAttacks) {
      state.finished = true;
      state.cleared = state.attack >= state.maxAttacks && state.heartsLost < 3;
    } else {
      static_cast<void>(rng);
      advanceParryPattern(state);
    }
  }
}

bool parryDuelInput(ParryDuelState& state, int direction) {
  if (state.finished) return false;
  const bool valid = state.feint ? direction < 0 : direction == state.direction;
  if (valid) {
    ++state.successfulParries;
  } else {
    ++state.heartsLost;
  }
  ++state.attack;
  state.elapsed = 0.0F;
  if (state.heartsLost >= 3 || state.attack >= state.maxAttacks) {
    state.finished = true;
    state.cleared = state.attack >= state.maxAttacks && state.heartsLost < 3;
  }
  if (!state.finished) advanceParryPattern(state);
  return valid;
}

int parryDuelScore(const ParryDuelState& state, int attack) {
  if (!state.cleared) return 0;
  return state.successfulParries + std::min(4, std::max(0, attack) / 30);
}

RuneRecallState startRuneRecall(std::uint64_t seed) {
  RuneRecallState state;
  Pcg32 rng(seed);
  state.sequence.resize(3);
  for (int& rune : state.sequence) rune = static_cast<int>(rng.nextRange(0, 5));
  state.showDuration = 1.25F;
  state.patternState = rng.next();
  return state;
}

void updateRuneRecall(RuneRecallState& state, float deltaSeconds, Pcg32& rng) {
  static_cast<void>(rng);
  if (state.finished || !state.showing) return;
  state.showElapsed += std::max(0.0F, deltaSeconds);
  if (state.showElapsed >= state.showDuration) state.showing = false;
}

bool inputRuneRecall(RuneRecallState& state, int rune) {
  if (state.finished || state.showing || rune < 0 || rune >= 6) return false;
  if (rune != state.sequence[static_cast<std::size_t>(state.inputIndex)]) {
    ++state.mistakes;
    state.inputIndex = 0;
    return false;
  }
  ++state.inputIndex;
  if (state.inputIndex < static_cast<int>(state.sequence.size())) return true;
  if (state.round == kRuneRounds) {
    state.finished = true;
    state.cleared = true;
    return true;
  }
  ++state.round;
  state.inputIndex = 0;
  state.patternState = state.patternState * 1664525U + 1013904223U;
  state.sequence.clear();
  const int sequenceLength = std::min(8, state.round + 2);
  for (int index = 0; index < sequenceLength; ++index) {
    state.patternState = state.patternState * 1664525U + 1013904223U;
    state.sequence.push_back(static_cast<int>(state.patternState % 6U));
  }
  state.showing = true;
  state.showElapsed = 0.0F;
  state.showDuration = 1.25F + 0.5F * static_cast<float>(state.round - 1);
  return true;
}

int runeRecallScore(const RuneRecallState& state, int focus) {
  if (!state.cleared) return 0;
  return std::max(0, 10 - state.mistakes + std::min(2, std::max(0, focus) / 20));
}

StarTraceState startStarTrace(std::uint64_t seed) {
  StarTraceState state;
  Pcg32 rng(seed);
  state.patternState = rng.next();
  state.stars.resize(4);
  state.starX.resize(4);
  state.starY.resize(4);
  for (std::size_t index = 0; index < state.stars.size(); ++index) {
    state.stars[index] = static_cast<int>(index);
    state.starX[index] = static_cast<float>(rng.nextRange(15, 85)) / 100.0F;
    state.starY[index] = static_cast<float>(rng.nextRange(20, 80)) / 100.0F;
  }
  return state;
}

void regenerateStarTrace(StarTraceState& state) {
  state.patternState = state.patternState * 1664525U + 1013904223U;
  const int minimumCount = state.constellation >= 3 ? 5 : 4;
  const int count = minimumCount + static_cast<int>(state.patternState % 2U);
  state.stars.resize(static_cast<std::size_t>(count));
  state.starX.resize(static_cast<std::size_t>(count));
  state.starY.resize(static_cast<std::size_t>(count));
  for (int index = 0; index < count; ++index) {
    state.patternState = state.patternState * 1664525U + 1013904223U;
    state.stars[static_cast<std::size_t>(index)] = index;
    bool placed = false;
    for (int attempt = 0; attempt < 20 && !placed; ++attempt) {
      state.patternState = state.patternState * 1664525U + 1013904223U;
      const float x = 0.15F + 0.70F * (state.patternState / 4294967295.0F);
      state.patternState = state.patternState * 1664525U + 1013904223U;
      const float y = 0.20F + 0.60F * (state.patternState / 4294967295.0F);
      placed = true;
      for (int prior = 0; prior < index; ++prior) {
        const float dx = x - state.starX[static_cast<std::size_t>(prior)];
        const float dy = y - state.starY[static_cast<std::size_t>(prior)];
        if (dx * dx + dy * dy < 0.045F) placed = false;
      }
      if (placed) {
        state.starX[static_cast<std::size_t>(index)] = x;
        state.starY[static_cast<std::size_t>(index)] = y;
      }
    }
  }
}

void updateStarTrace(StarTraceState& state, float deltaSeconds, Pcg32& rng) {
  static_cast<void>(rng);
  if (state.finished) return;
  state.elapsed += std::max(0.0F, deltaSeconds);
  if (state.elapsed >= 40.0F) {
    state.finished = true;
    state.cleared = state.constellation >= 8;
    return;
  }
}

bool hitStarTrace(StarTraceState& state, int star) {
  if (state.finished || star < 0 || star >= 6) return false;
  if (star != state.stars[static_cast<std::size_t>(state.targetIndex)]) {
    ++state.heartsLost;
    if (state.heartsLost >= 3) state.finished = true;
    return false;
  }
  ++state.targetIndex;
  if (state.targetIndex == static_cast<int>(state.stars.size())) {
    ++state.constellation;
    state.targetIndex = 0;
    regenerateStarTrace(state);
  }
  return true;
}

int starTraceScore(const StarTraceState& state, int speed) {
  if (!state.cleared) return 0;
  return state.constellation + std::min(2, std::max(0, speed) / 40);
}

HeartpierceState startHeartpierce(std::uint64_t seed, float focus, int ward,
                                   int echoRank) {
  HeartpierceState state;
  state.patternState = static_cast<std::uint32_t>(seed);
  state.focus = std::clamp(focus, 0.0F, 0.25F);
  state.echoModifier = std::min(2.0F, 1.0F + 0.08F *
                                               static_cast<float>(std::max(0, echoRank - 1)));
  state.hearts = 3 + std::max(0, ward);
  state.waypointY = 0.72F;
  state.heartY = state.waypointY;
  return state;
}

void updateHeartpierce(HeartpierceState& state, float deltaSeconds, bool rising) {
  if (state.finished) return;
  const float delta = std::min(0.1F, std::max(0.0F, deltaSeconds));
  state.totalTime += delta;
  state.invulnerable = std::max(0.0F, state.invulnerable - delta);

  const float thrust = 2.4F;
  const float gravity = 1.6F;
  const float maxVelocity = 1.2F;
  state.lanceV += (rising ? thrust : -gravity) * delta;
  state.lanceV = std::clamp(state.lanceV, -maxVelocity, maxVelocity);
  state.lanceY += state.lanceV * delta;
  if (state.lanceY < 0.0F || state.lanceY > 1.0F) {
    state.lanceY = std::clamp(state.lanceY, 0.0F, 1.0F);
    state.lanceV = -state.lanceV * 0.3F;
  }

  const float speed[] = {0.28F, 0.40F, 0.56F};
  const float retargetMin[] = {2.0F, 1.6F, 1.2F};
  const float retargetMax[] = {3.0F, 2.6F, 2.0F};
  const float zone[] = {0.13F, 0.11F, 0.09F};
  const float gain[] = {0.25F, 0.22F, 0.20F};
  const float drain[] = {0.12F, 0.14F, 0.16F};
  const int phase = std::clamp(state.phase, 0, 2);
  state.retargetTimer -= delta;
  if (state.retargetTimer <= 0.0F) {
    state.patternState = state.patternState * 1664525U + 1013904223U;
    float candidate = 0.1F + 0.8F *
        (static_cast<float>(state.patternState) / 4294967295.0F);
    if (std::abs(candidate - state.waypointY) < 0.2F) candidate = state.waypointY < 0.5F ? 0.9F : 0.1F;
    state.waypointY = candidate;
    state.patternState = state.patternState * 1664525U + 1013904223U;
    const float randomUnit = static_cast<float>(state.patternState) / 4294967295.0F;
    state.retargetTimer = retargetMin[phase] +
                          (retargetMax[phase] - retargetMin[phase]) * randomUnit /
                              state.echoModifier;
  }
  if (phase == 2) {
    state.dartTimer -= delta;
    if (state.dartTelegraph > 0.0F) {
      state.dartTelegraph -= delta;
      if (state.dartTelegraph <= 0.0F) state.heartY = state.dartTarget;
    } else if (state.dartTimer <= 0.0F) {
      state.patternState = state.patternState * 1664525U + 1013904223U;
      state.dartTarget = 0.1F + 0.8F *
          (static_cast<float>(state.patternState) / 4294967295.0F);
      state.dartTelegraph = 0.4F;
      state.patternState = state.patternState * 1664525U + 1013904223U;
      state.dartTimer = 5.0F + 2.0F *
          (static_cast<float>(state.patternState) / 4294967295.0F);
    }
  }
  const float heartDelta = state.waypointY - state.heartY;
  const float maxStep = speed[phase] * state.echoModifier * delta;
  state.heartY += std::clamp(heartDelta, -maxStep, maxStep);
  const float overlapHalf = 0.10F * (1.0F + state.focus) +
                            zone[phase] * (1.0F + state.focus) / state.echoModifier;
  const bool overlap = std::abs(state.lanceY - state.heartY) <= overlapHalf;
  if (overlap) {
    state.overlapTime += delta;
    state.gauge += gain[phase] * delta;
  } else {
    state.gauge -= drain[phase] * delta;
  }
  if (state.gauge >= 1.0F) {
    if (state.phase == 2) {
      state.finished = true;
      state.cleared = true;
    } else {
      ++state.phase;
      state.gauge = 0.25F;
      state.retargetTimer = 0.0F;
    }
  } else if (state.gauge <= 0.0F && state.invulnerable <= 0.0F) {
    ++state.heartsLost;
    --state.hearts;
    state.gauge = 0.25F;
    state.invulnerable = 1.0F;
    if (state.hearts <= 0) state.finished = true;
  }
}

int heartpierceScore(const HeartpierceState& state) {
  if (!state.cleared || state.totalTime <= 0.0F) return 0;
  return static_cast<int>(std::clamp(state.overlapTime / state.totalTime, 0.0F, 1.0F) * 100.0F);
}

SoulDodgeState startSoulDodge(std::uint64_t seed, int ward, float duration) {
  SoulDodgeState state;
  state.patternState = static_cast<std::uint32_t>(seed);
  state.hearts = 3 + std::max(0, ward);
  state.duration = std::max(0.1F, duration);
  state.patternTimer = 0.5F;
  return state;
}

void updateSoulDodge(SoulDodgeState& state, float deltaSeconds, float moveX,
                     float moveY, bool slowMove, int speed) {
  if (state.finished) return;
  const float delta = std::min(0.1F, std::max(0.0F, deltaSeconds));
  state.elapsed = std::min(60.0F, state.elapsed + delta);
  state.invulnerable = std::max(0.0F, state.invulnerable - delta);
  state.slowMove = slowMove;
  const float movement = (slowMove ? 0.13F : 0.25F) *
                         std::clamp(1.0F + speed / 100.0F, 0.75F, 1.5F);
  state.playerX = std::clamp(state.playerX + moveX * movement * delta, 0.03F, 0.97F);
  state.playerY = std::clamp(state.playerY + moveY * movement * delta, 0.03F, 0.97F);

  state.patternTimer -= delta;
  if (state.patternTimer <= 0.0F) {
    state.patternState = state.patternState * 1664525U + 1013904223U;
    const auto kind = static_cast<SoulHazardKind>(state.patternState % 4U);
    const int count = kind == SoulHazardKind::Laser
                          ? 1
                          : 5 + static_cast<int>((state.patternState >> 24) % 5U);
    const float centerX = 0.2F +
        static_cast<float>((state.patternState >> 8) % 600U) / 1000.0F;
    const float centerY = 0.2F +
        static_cast<float>((state.patternState >> 18) % 600U) / 1000.0F;
    for (int index = 0; index < count; ++index) {
      state.patternState = state.patternState * 1664525U + 1013904223U;
      SoulHazard hazard;
      hazard.kind = kind;
      hazard.lifetime = kind == SoulHazardKind::Laser ? 2.2F : 3.8F;
      hazard.telegraph = kind == SoulHazardKind::Laser ? 0.8F : 0.0F;
      hazard.angle = static_cast<float>(state.patternState % 628U) / 100.0F +
                     static_cast<float>(index) * 0.45F;
      const float spread = count > 1
                               ? (static_cast<float>(index) / static_cast<float>(count - 1) - 0.5F) *
                                     0.24F
                               : 0.0F;
      hazard.x = kind == SoulHazardKind::Rain
                     ? 0.08F + static_cast<float>(index) / static_cast<float>(count - 1) * 0.84F
                     : std::clamp(centerX + spread, 0.06F, 0.94F);
      hazard.y = kind == SoulHazardKind::Rain
                     ? -0.08F
                     : std::clamp(centerY + spread * 0.6F, 0.06F, 0.94F);
      hazard.radius = kind == SoulHazardKind::Ring
                          ? 0.18F
                          : (kind == SoulHazardKind::Spiral ? 0.02F : 0.0F);
      state.hazards.push_back(hazard);
    }
    state.patternTimer = 1.3F - std::min(0.35F, state.elapsed / 180.0F);
  }

  for (auto& hazard : state.hazards) {
    hazard.lifetime -= delta;
    if (hazard.telegraph > 0.0F) hazard.telegraph -= delta;
    if (hazard.kind == SoulHazardKind::Spiral || hazard.kind == SoulHazardKind::Ring) {
      hazard.angle += delta * (hazard.kind == SoulHazardKind::Ring ? 0.9F : 1.4F);
      hazard.radius += delta * (hazard.kind == SoulHazardKind::Ring ? 0.12F : 0.10F);
    } else if (hazard.kind == SoulHazardKind::Rain) {
      hazard.y += delta * 0.35F;
    }
  }
  for (const auto& hazard : state.hazards) {
    if (hazard.lifetime <= 0.0F || hazard.telegraph > 0.0F ||
        state.invulnerable > 0.0F) continue;
    float distance = 10.0F;
    if (hazard.kind == SoulHazardKind::Laser) {
      distance = std::abs(state.playerX - hazard.x);
    } else if (hazard.kind == SoulHazardKind::Rain) {
      distance = std::hypot(state.playerX - hazard.x, state.playerY - hazard.y);
    } else {
      const float hx = hazard.x + std::cos(hazard.angle) * hazard.radius;
      const float hy = hazard.y + std::sin(hazard.angle) * hazard.radius;
      distance = std::hypot(state.playerX - hx, state.playerY - hy);
    }
    if (distance <= (hazard.kind == SoulHazardKind::Laser ? 0.045F : 0.035F)) {
      ++state.hits;
      --state.hearts;
      state.invulnerable = 1.0F;
      break;
    }
  }
  state.hazards.erase(std::remove_if(state.hazards.begin(), state.hazards.end(),
                                     [](const SoulHazard& hazard) {
                                       return hazard.lifetime <= 0.0F;
                                     }),
                      state.hazards.end());
  if (state.hearts <= 0) {
    state.finished = true;
  } else if (state.elapsed >= state.duration) {
    state.finished = true;
    state.cleared = true;
  }
}

int soulDodgeScore(const SoulDodgeState& state) {
  return state.cleared ? std::max(0, 100 - state.hits * 20) : 0;
}

CrownStrikeState startCrownStrike() {
  return {};
}

void updateCrownStrike(CrownStrikeState& state, float deltaSeconds) {
  if (state.finished) return;
  state.needle += state.direction * std::max(0.0F, deltaSeconds) * 2.2F;
  if (state.needle >= 1.0F) {
    state.needle = 1.0F;
    state.direction = -1.0F;
  } else if (state.needle <= 0.0F) {
    state.needle = 0.0F;
    state.direction = 1.0F;
  }
}

bool strikeCrown(CrownStrikeState& state, float focus) {
  if (state.finished) return false;
  const float zone = 0.10F * (1.0F + std::clamp(focus, 0.0F, 0.25F));
  const bool hit = std::abs(state.needle - 0.5F) <= zone;
  if (hit) ++state.hits;
  else ++state.misses;
  ++state.strike;
  if (state.strike >= 3) {
    state.finished = true;
    state.cleared = state.hits == 3;
  }
  return hit;
}

int crownStrikeScore(const CrownStrikeState& state) {
  return state.cleared ? state.hits : 0;
}

FiveTrialsState startFiveTrials(int ward) {
  FiveTrialsState state;
  state.maxHearts = state.hearts = 3 + std::max(0, ward);
  return state;
}

bool advanceFiveTrials(FiveTrialsState& state, bool phaseCleared,
                       int phaseHeartsLost) {
  if (state.finished) return false;
  state.heartsLost += std::max(0, phaseHeartsLost);
  state.hearts -= std::max(0, phaseHeartsLost);
  if (!phaseCleared || state.hearts <= 0) {
    state.finished = true;
    return false;
  }
  if (state.phase == 2 && !state.restoredBetweenPhases) {
    state.hearts = std::min(state.maxHearts, state.hearts + 1);
    state.restoredBetweenPhases = true;
  }
  if (state.phase == 5) {
    state.finished = true;
    state.cleared = true;
    return true;
  }
  ++state.phase;
  state.phaseElapsed = 0.0F;
  return true;
}

int fiveTrialsScore(const FiveTrialsState& state) {
  return state.cleared ? std::max(0, 3 - state.heartsLost) : 0;
}

} // namespace ed
