#include "ed/campaign.hpp"
#include "ed/pcg.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ed {

namespace {

int adjustedThreshold(int threshold, double focus) {
  const double clampedFocus = std::clamp(focus, 0.0, 0.25);
  return std::max(1, static_cast<int>(std::ceil(threshold / (1.0 + clampedFocus))));
}

BossDef makeBoss(std::string id,
                 std::string name,
                 std::string layerId,
                 BossChallenge challenge,
                 int minLevel,
                 int clearScore,
                 int twoStarScore,
                 int threeStarScore,
                 std::string legendaryId,
                 std::int64_t maxHp = 100,
                 std::int64_t attack = 10,
                 std::int64_t defense = 0,
                 std::int64_t speed = 10,
                 std::string abilityName = "Strike") {
  BossDef boss;
  boss.id = std::move(id);
  boss.name = std::move(name);
  boss.layerId = std::move(layerId);
  boss.challenge = challenge;
  boss.minLevel = minLevel;
  boss.recommendedGearTier = minLevel == 1 ? 1 : std::min(10, (minLevel + 5) / 7);
  boss.clearScore = clearScore;
  boss.twoStarScore = twoStarScore;
  boss.threeStarScore = threeStarScore;
  boss.firstClearGold = 250 * (1 << std::min(9, static_cast<int>(boss.challenge)));
  boss.firstClearXp = 100 * (1 << std::min(9, static_cast<int>(boss.challenge)));
  boss.legendaryId = std::move(legendaryId);
  boss.maxHp = maxHp;
  boss.attack = attack;
  boss.defense = defense;
  boss.speed = speed;
  boss.abilityName = std::move(abilityName);
  return boss;
}

} // namespace

std::vector<BossDef> defaultBosses() {
  return {
      makeBoss("grubnok", "Grubnok, the Burrow-King", "ashen-warrens",
               BossChallenge::FrenzyDig, 1, 5, 7, 9, "burrowkings-crown",
               110, 10, 4, 8, "Burrow Strike"),
      makeBoss("sir-rattlebone", "Sir Rattlebone", "copper-crypt",
               BossChallenge::BoneDuel, 5, 5, 7, 8, "bonecleaver-gauntlets",
               260, 22, 12, 10, "Bone Guard"),
      makeBoss("morwenna", "Morwenna the Ashen Witch", "iron-hollows",
               BossChallenge::CinderDance, 10, 70, 85, 95, "cinderheart-amulet",
               620, 50, 28, 12, "Cinder Burn"),
      makeBoss("thessaly", "Thessaly, the Bog Hydra", "silvermire",
               BossChallenge::HeadHunt, 15, 30, 38, 45, "hydrascale-cuirass",
               1500, 115, 65, 14, "Hydra Venom"),
      makeBoss("aldric", "Aldric, the Gilded Knight", "aurelite-keep",
               BossChallenge::ParryDuel, 20, 1, 1, 1, "sunforged-pickaxe",
               3600, 260, 150, 16, "Aegis Bash"),
      makeBoss("isolde", "Isolde, Runewarden", "runic-archives",
               BossChallenge::RuneRecall, 27, 1, 1, 1, "runewardens-diadem",
               8500, 600, 340, 18, "Runic Burst"),
      makeBoss("seraphel", "Seraphel, the Fallen Star", "stargazer-spire",
               BossChallenge::StarTrace, 34, 8, 10, 12, "starfall-treads",
               20000, 1400, 780, 20, "Starfall Comet"),
      makeBoss("ignathia", "Ignathia, the Ember Wyrm", "wyrmbone-caverns",
               BossChallenge::Heartpierce, 42, 50, 70, 85, "wyrmfang-breaker",
               48000, 3200, 1800, 22, "Molten Breath"),
      makeBoss("xarmal", "Xar'Mal, the Lich Emperor", "voidglass-sanctum",
               BossChallenge::SoulDodge, 50, 80, 90, 100, "voidglass-phylactery",
               115000, 7200, 4000, 24, "Soul Harvest"),
      makeBoss("hollow-king", "The Hollow King", "aether-vault",
               BossChallenge::FiveTrials, 60, 1, 2, 3, "aetherbrand",
               280000, 16500, 9200, 26, "Hollow Crown Judgment"),
  };
}

CampaignState createCampaignState(std::size_t bossCount) {
  CampaignState campaign;
  campaign.bestStars.assign(bossCount, 0);
  campaign.cleared.assign(bossCount, false);
  return campaign;
}

bool isBossUnlocked(const GameState& game,
                    const CampaignState& campaign,
                    const std::vector<BossDef>& bosses,
                    std::size_t bossIndex) {
  if (bossIndex >= bosses.size()) {
    return false;
  }
  if (game.player.level < bosses[bossIndex].minLevel) {
    return false;
  }
  if (bossIndex == 0) {
    return true;
  }
  return bossIndex - 1 < campaign.cleared.size() && campaign.cleared[bossIndex - 1];
}

BossOutcome evaluateBossAttempt(const GameState& game,
                                CampaignState& campaign,
                                const std::vector<BossDef>& bosses,
                                std::size_t bossIndex,
                                const BossAttempt& attempt) {
  if (bossIndex >= bosses.size() || !isBossUnlocked(game, campaign, bosses, bossIndex)) {
    throw std::invalid_argument("boss is not unlocked");
  }

  const BossDef& boss = bosses[bossIndex];
  BossOutcome outcome;
  outcome.cleared =
      attempt.completed && attempt.score >= adjustedThreshold(boss.clearScore, attempt.focus);
  if (!outcome.cleared) {
    return outcome;
  }

  outcome.stars = 1;
  if (attempt.score >= adjustedThreshold(boss.twoStarScore, attempt.focus) && attempt.heartsLost <= 1) {
    outcome.stars = 2;
  }
  if (attempt.score >= adjustedThreshold(boss.threeStarScore, attempt.focus) &&
      attempt.heartsLost == 0) {
    outcome.stars = 3;
  }

  if (campaign.bestStars.size() < bosses.size()) {
    campaign.bestStars.resize(bosses.size(), 0);
  }
  if (campaign.cleared.size() < bosses.size()) {
    campaign.cleared.resize(bosses.size(), false);
  }
  outcome.firstClear = !campaign.cleared[bossIndex];
  campaign.cleared[bossIndex] = true;
  campaign.bestStars[bossIndex] = std::max(campaign.bestStars[bossIndex], outcome.stars);
  outcome.goldReward = outcome.firstClear ? boss.firstClearGold : boss.firstClearGold / 4;
  outcome.xpReward = outcome.firstClear ? boss.firstClearXp : boss.firstClearXp / 4;
  outcome.legendaryId = outcome.firstClear ? boss.legendaryId : "";
  return outcome;
}

void applyBossOutcome(GameState& game, const BossOutcome& outcome) {
  if (!outcome.cleared) {
    return;
  }
  game.player.gold += outcome.goldReward;
  game.player.goldEarnedThisRun += outcome.goldReward;
  applyXp(game, outcome.xpReward);
  if (outcome.firstClear) {
    ++game.player.bossesCleared;
    ++game.player.bossEssence;
  }
  game.dirty = true;
}

BossDef getEffectiveBossDef(const BossDef& boss, bool isEcho, int echoRank) {
  if (!isEcho) {
    return boss;
  }
  BossDef scaled = boss;
  const int rank = std::max(1, echoRank);
  const int bossStep = std::clamp(static_cast<int>(boss.challenge), 0, 9);

  // In postgame Echo Campaign, player has conquered Hollow King and possesses Tier 10 / Mythic gear.
  // Boss 1 (Echo Grubnok) starts at a Tier 10 baseline (~350k HP, 15k Atk, 8.5k Def).
  // Boss 10 scales up to ~1.1M HP, 28.5k Atk, 16.9k Def at Rank 1.
  // Each Echo Rank adds +20% stats across all parameters.
  const double rankMultiplier = 1.0 + 0.20 * (rank - 1);
  const double stepProgression = 1.0 + 0.24 * bossStep;

  const double baseHp = 350000.0 * stepProgression * rankMultiplier;
  const double baseAtk = 15000.0 * (1.0 + 0.10 * bossStep) * rankMultiplier;
  const double baseDef = 8500.0 * (1.0 + 0.11 * bossStep) * rankMultiplier;
  const double baseSpd = 26.0 + 1.2 * bossStep + 2.0 * (rank - 1);

  scaled.maxHp = std::max<std::int64_t>(1000, static_cast<std::int64_t>(std::llround(baseHp)));
  scaled.attack = std::max<std::int64_t>(100, static_cast<std::int64_t>(std::llround(baseAtk)));
  scaled.defense = std::max<std::int64_t>(0, static_cast<std::int64_t>(std::llround(baseDef)));
  scaled.speed = std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(baseSpd)));

  return scaled;
}

BossCombatResult simulateBossCombat(const PlayerStats& playerStats,
                                    const BossDef& boss,
                                    int minigameStars,
                                    bool isEcho,
                                    int echoRank,
                                    std::uint64_t seed) {
  Pcg32 rng(seed);
  BossCombatResult result;

  const BossDef effBoss = getEffectiveBossDef(boss, isEcho, echoRank);
  const int rank = std::max(1, echoRank);

  const std::int64_t bossMaxHp = effBoss.maxHp;
  const std::int64_t bossAttack = effBoss.attack;
  const std::int64_t bossDefense = effBoss.defense;
  const std::int64_t bossSpeed = effBoss.speed;

  const double starStatMult = minigameStars >= 3 ? 1.30 : (minigameStars >= 2 ? 1.15 : 1.0);
  const double bossHpStartMult = minigameStars >= 3 ? 0.85 : (minigameStars >= 2 ? 0.92 : 1.0);

  std::int64_t playerHp = std::max<std::int64_t>(1, playerStats.maxHp);
  const std::int64_t playerMaxHp = playerHp;
  std::int64_t bossHp = std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(bossMaxHp * bossHpStartMult)));

  const std::int64_t playerAttack = std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(playerStats.attack * starStatMult)));
  const std::int64_t playerDefense = std::max<std::int64_t>(0, static_cast<std::int64_t>(std::llround(playerStats.defense * starStatMult)));
  const std::int64_t playerSpeed = std::max<std::int64_t>(1, playerStats.speed);

  std::string startDesc = isEcho
      ? ("Echo of " + boss.name + " (Rank " + std::to_string(rank) + ") appears!")
      : (boss.name + " enters the arena!");
  result.events.push_back({BossCombatEventType::CombatStart, startDesc, 0, playerHp, bossHp, false});

  int round = 0;
  int bossTurnCount = 0;
  const bool playerFirst = playerSpeed >= bossSpeed;

  while (playerHp > 0 && bossHp > 0 && round < 100) {
    ++round;

    auto executePlayerTurn = [&]() {
      if (playerHp <= 0 || bossHp <= 0) return;
      const int variance = static_cast<int>(rng.nextRange(95, 105));
      std::int64_t rawDamage = std::max<std::int64_t>(1, playerAttack * variance / 100);

      bool isCrit = false;
      const std::uint32_t critChance = static_cast<std::uint32_t>(std::clamp(playerStats.critChanceBasisPoints, 0, 10000));
      if (rng.nextRange(0, 9999) < critChance) {
        rawDamage = rawDamage * std::max(10000, playerStats.critDamageBasisPoints) / 10000;
        isCrit = true;
      }

      const std::int64_t effectiveBossDef = std::max<std::int64_t>(0, bossDefense - playerStats.pierce);
      std::int64_t damage = std::max<std::int64_t>(1, (rawDamage * rawDamage) / (rawDamage + effectiveBossDef));

      if (playerStats.lifestealBasisPoints > 0) {
        const std::int64_t healed = damage * playerStats.lifestealBasisPoints / 10000;
        if (healed > 0) {
          playerHp = std::min(playerMaxHp, playerHp + healed);
        }
      }

      bossHp = std::max<std::int64_t>(0, bossHp - damage);
      std::string desc = "You strike " + boss.name + " for " + std::to_string(damage) + " damage" +
                         (isCrit ? " [CRITICAL!]" : "") + ".";
      result.events.push_back({BossCombatEventType::PlayerAttack, desc, damage, playerHp, bossHp, isCrit});

      if (bossHp <= 0) {
        std::string winDesc = isEcho
            ? ("Echo of " + boss.name + " has been banished!")
            : (boss.name + " has been defeated!");
        result.events.push_back({BossCombatEventType::BossDefeated, winDesc, 0, playerHp, 0, false});
      }
    };

    auto executeBossTurn = [&]() {
      if (playerHp <= 0 || bossHp <= 0) return;
      ++bossTurnCount;
      const bool useAbility = (bossTurnCount % 3 == 0);

      const int variance = static_cast<int>(rng.nextRange(95, 105));
      std::int64_t rawAtk = bossAttack * variance / 100;
      if (useAbility) {
        rawAtk = rawAtk * 150 / 100;
      }

      std::int64_t damage = std::max<std::int64_t>(1, (rawAtk * rawAtk) / (rawAtk + playerDefense));
      playerHp = std::max<std::int64_t>(0, playerHp - damage);

      if (useAbility) {
        std::string desc = boss.name + " unleashes [" + boss.abilityName + "] for " +
                           std::to_string(damage) + " devastating damage!";
        result.events.push_back({BossCombatEventType::BossAbility, desc, damage, playerHp, bossHp, false});
      } else {
        std::string desc = boss.name + " attacks for " + std::to_string(damage) + " damage.";
        result.events.push_back({BossCombatEventType::BossAttack, desc, damage, playerHp, bossHp, false});
      }

      if (playerHp <= 0) {
        std::string loseDesc = "You were struck down by " + boss.name + "!";
        result.events.push_back({BossCombatEventType::PlayerDefeated, loseDesc, 0, 0, bossHp, false});
      }
    };

    if (playerFirst) {
      executePlayerTurn();
      executeBossTurn();
    } else {
      executeBossTurn();
      executePlayerTurn();
    }
  }

  result.victory = (bossHp <= 0 && playerHp > 0);
  result.finalPlayerHp = playerHp;
  result.finalBossHp = bossHp;
  result.rounds = round;
  return result;
}

} // namespace ed
