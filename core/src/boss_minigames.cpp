#include "ed/boss_minigames.hpp"

#include <algorithm>
#include <cmath>

namespace ed {

namespace {

constexpr float kFrenzyDuration = 10.0F;
constexpr int kBoneRounds = 8;
constexpr float kCinderDuration = 75.0F;
constexpr float kHeadHuntDuration = 35.0F;
constexpr int kParryAttacks = 24;
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

CinderDanceState startCinderDance(std::uint64_t seed) {
  Pcg32 rng(seed);
  CinderDanceState state;
  state.chart.reserve(110);
  for (int index = 0; index < 110; ++index) {
    CinderNote note;
    note.time = 2.0F + static_cast<float>(index) * (70.0F / 109.0F);
    note.lane = static_cast<int>(rng.nextRange(0, 3));
    state.chart.push_back(note);
  }
  return state;
}

void updateCinderDance(CinderDanceState& state, float deltaSeconds) {
  if (state.finished) return;
  state.elapsed = std::min(kCinderDuration, state.elapsed + std::max(0.0F, deltaSeconds));
  while (state.nextNote < state.chart.size() &&
         state.elapsed - state.chart[state.nextNote].time > 0.1F) {
    state.chart[state.nextNote].judged = true;
    ++state.nextNote;
    ++state.misses;
    state.groove = std::max(0, state.groove - 6);
  }
  if (state.groove == 0 || state.elapsed >= kCinderDuration) {
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
  const float window = 0.1F * focusMultiplier(focus);
  std::size_t candidate = state.nextNote;
  while (candidate < state.chart.size() && state.chart[candidate].judged) ++candidate;
  if (candidate >= state.chart.size() || state.chart[candidate].lane != lane ||
      std::abs(state.elapsed - state.chart[candidate].time) > window) {
    return 0;
  }
  const float distance = std::abs(state.elapsed - state.chart[candidate].time);
  state.chart[candidate].judged = true;
  state.nextNote = candidate + 1;
  if (distance <= 0.045F * focusMultiplier(focus)) {
    ++state.perfectHits;
    state.groove = std::min(100, state.groove + 2);
    return 2;
  }
  ++state.goodHits;
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

ParryDuelState startParryDuel(std::uint64_t seed) {
  ParryDuelState state;
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
    ++state.heartsLost;
    ++state.attack;
    state.elapsed = 0.0F;
    if (state.heartsLost >= 3 || state.attack >= kParryAttacks) {
      state.finished = true;
      state.cleared = state.attack >= kParryAttacks && state.heartsLost < 3;
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
  if (state.heartsLost >= 3 || state.attack >= kParryAttacks) {
    state.finished = true;
    state.cleared = state.attack >= kParryAttacks && state.heartsLost < 3;
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

WyrmfireRunState startWyrmfireRun() {
  WyrmfireRunState state;
  state.hazardLane = 1;
  state.emberLane = 0;
  state.hazardY = 0.0F;
  state.emberY = 0.45F;
  return state;
}

void updateWyrmfireRun(WyrmfireRunState& state, float deltaSeconds) {
  if (state.finished) return;
  const float delta = std::max(0.0F, deltaSeconds);
  state.elapsed = std::min(45.0F, state.elapsed + delta);
  if (state.hazardActive) {
    state.hazardY += delta * 0.24F;
    if (state.hazardY >= 0.82F) {
      if (state.hazardLane == state.lane) ++state.heartsLost;
      state.hazardActive = false;
      state.hazardRespawn = 1.5F;
    }
  } else {
    state.hazardRespawn -= delta;
    if (state.hazardRespawn <= 0.0F) {
      state.hazardActive = true;
      state.hazardY = 0.0F;
      state.hazardLane = (state.hazardLane + 1) % 3;
    }
  }
  if (state.emberActive) {
    state.emberY += delta * 0.18F;
    if (state.emberY >= 0.82F) {
      if (state.emberLane == state.lane) ++state.embers;
      state.emberActive = false;
      state.emberRespawn = 1.0F;
    }
  } else {
    state.emberRespawn -= delta;
    if (state.emberRespawn <= 0.0F) {
      state.emberActive = true;
      state.emberY = 0.0F;
      state.emberLane = (state.emberLane + 2) % 3;
    }
  }
  if (state.scaleBreak) {
    state.scaleElapsed += delta;
    if (state.scaleElapsed >= 3.0F && state.burstTaps < 25) ++state.heartsLost;
    if (state.scaleElapsed >= 3.0F) {
      state.scaleBreak = false;
      state.scaleElapsed = 0.0F;
      state.burstTaps = 0;
    }
  } else if (state.elapsed >= state.nextScaleBreak) {
    state.scaleBreak = true;
    state.scaleElapsed = 0.0F;
    state.nextScaleBreak += 15.0F;
  }
  if (state.heartsLost >= 3 || state.elapsed >= 45.0F) {
    state.finished = true;
    state.cleared = state.elapsed >= 45.0F && state.heartsLost < 3;
  }
}

void moveWyrmfireRun(WyrmfireRunState& state, int direction) {
  if (!state.finished && direction != 0) {
    state.lane = (state.lane + direction) % 3;
    if (state.lane < 0) state.lane += 3;
  }
}

void tapWyrmfireRun(WyrmfireRunState& state) {
  if (!state.finished && state.scaleBreak) ++state.burstTaps;
}

int wyrmfireRunScore(const WyrmfireRunState& state, int speed) {
  if (!state.cleared) return 0;
  return 1 + state.embers + std::min(2, std::max(0, speed) / 40);
}

} // namespace ed
