#include "game_actions.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

#include "ed/descent/combat_sim.hpp"

namespace ed::app {

void spawnBlock(AppState& app) {
  const std::string& layerId = app.layers[static_cast<std::size_t>(app.layerIndex)].id;
  const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
  if (layerId == "aether-vault" && app.deepStrataDepth > 0) {
    app.blockSpec = ed::generateDeepStrataBlock(app.layers, app.materials, app.deepStrataDepth, stats.luck, app.rng);
  } else {
    app.blockSpec = ed::generateBlockForLayer(app.layers, app.materials, layerId, stats.luck, app.rng);
  }
  app.block = ed::createMiningProgress(app.blockSpec);
}

std::string layerLockReason(const ed::GameState& game, const ed::LayerDef& layer) {
  std::string message = layer.name + " is locked. ";
  if (game.player.level < layer.minLevel) {
    message += "Reach level " + std::to_string(layer.minLevel) + ".";
  } else if (game.player.pickaxeTier < layer.requiredPickaxeTier) {
    message += "Equip a tier " + std::to_string(layer.requiredPickaxeTier) + " pickaxe.";
  }
  return message;
}

int layerIndexForId(const AppState& app, const std::string& id) {
  for (std::size_t i = 0; i < app.layers.size(); ++i) {
    if (app.layers[i].id == id) return static_cast<int>(i);
  }
  return -1;
}

void changeLayer(AppState& app, int direction) {
  const int currentIndex = app.layerIndex;
  const int nextIndex = std::clamp(
      currentIndex + direction, 0, static_cast<int>(app.layers.size()) - 1);
  if (nextIndex == currentIndex) {
    setNotice(app, direction < 0 ? "You are already at the first layer."
                                 : "You are already at the deepest unlocked layer.");
    return;
  }

  const auto& nextLayer = app.layers[static_cast<std::size_t>(nextIndex)];
  if (!ed::canUnlockLayer(app.game, nextLayer)) {
    setNotice(app, layerLockReason(app.game, nextLayer));
    return;
  }

  app.layerIndex = nextIndex;
  app.game.player.currentLayer = nextLayer.id;
  spawnBlock(app);
  app.game.dirty = true;
}

std::string craftFailureReason(const AppState& app, const ed::CraftRecipe& recipe) {
  const int recipeLayer = std::clamp(recipe.tier - 1, 0, static_cast<int>(app.layers.size()) - 1);
  if (!ed::canUnlockLayer(app.game, app.layers[static_cast<std::size_t>(recipeLayer)])) {
    return layerLockReason(app.game, app.layers[static_cast<std::size_t>(recipeLayer)]);
  }
  if (app.game.player.gold < recipe.gold) {
    return "Need " + ed::formatCount(recipe.gold - app.game.player.gold) + " more gold.";
  }
  const std::string materialId = materialIdForTier(recipe.tier);
  const int ingots = ed::countItem(app.inventory, "ingots", materialId);
  if (ingots < recipe.ingots) {
    return "Need " + ed::formatCount(recipe.ingots - ingots) + " more " + materialId + " ingot(s).";
  }
  const std::string trophyId = "zone-" + std::to_string(recipe.tier);
  const int trophies = ed::countItem(app.inventory, "trophies", trophyId);
  if (trophies < recipe.trophies) {
    return "Need " + ed::formatCount(recipe.trophies - trophies) + " more tier " +
           std::to_string(recipe.tier) + " dungeon trophy/trophies.";
  }
  if (recipe.requiresGem && app.inventory.gems.empty()) {
    return "Find at least one gem before crafting an amulet.";
  }
  return "Crafting requirements are not met.";
}

bool dungeonUnlocked(const AppState& app, int zoneIndex) {
  if (zoneIndex < 0 || zoneIndex >= static_cast<int>(app.zones.size())) return false;
  const int index = layerIndexForId(app, app.zones[static_cast<std::size_t>(zoneIndex)].id);
  return index >= 0 && ed::canUnlockLayer(app.game, app.layers[static_cast<std::size_t>(index)]);
}

std::string dungeonLockReason(const AppState& app, int zoneIndex) {
  if (zoneIndex < 0 || zoneIndex >= static_cast<int>(app.zones.size())) {
    return "That dungeon does not exist.";
  }
  const auto& zone = app.zones[static_cast<std::size_t>(zoneIndex)];
  const int index = layerIndexForId(app, zone.id);
  return index >= 0 ? layerLockReason(app.game, app.layers[static_cast<std::size_t>(index)])
                    : zone.name + " is not available.";
}

void spawnHitEffects(AppState& app, Vector2 center, Color color, int damage) {
  if (app.screen != Screen::Mine) return;
  app.floatingText.push_back({{center.x - 20.0F, center.y - 80.0F},
                              std::to_string(damage),
                              1.0F,
                              Color{255, 244, 190, 255}});
  for (int i = 0; i < 8; ++i) {
    const float angle = static_cast<float>(i) * 0.785398F;
    app.particles.push_back({center,
                             {std::cos(angle) * 80.0F, std::sin(angle) * 80.0F},
                             0.55F,
                             color});
  }
  app.shake = 0.15F;
}

void mine(AppState& app, Vector2 clickPosition, bool isAutoMine) {
  const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
  bool isCrit = false;
  double critMultiplier = static_cast<double>(stats.critDamageBasisPoints) / 10000.0;
  bool hitWeakSpot = false;

  if (app.weakSpotActive) {
    const float dx = clickPosition.x - app.weakSpotPos.x;
    const float dy = clickPosition.y - app.weakSpotPos.y;
    if (dx * dx + dy * dy <= 32.0F * 32.0F) {
      hitWeakSpot = true;
      isCrit = true;
      critMultiplier *= 1.5; // Guaranteed crit with +50% bonus damage
      app.weakSpotActive = false;
      const float cdMult = std::max(0.2F, 1.0F - app.game.player.vows.deepBreath * 0.05F);
      app.weakSpotCooldown = (4.0F + static_cast<float>(app.rng.next() % 4000) / 1000.0F) * cdMult;
      ++app.game.player.weakSpotsHit;
      app.shake = 0.35F;

      for (int i = 0; i < 16; ++i) {
        const float angle = static_cast<float>(i) * 0.392699F;
        const float spd = 120.0F + static_cast<float>(app.rng.next() % 60);
        app.particles.push_back({app.weakSpotPos,
                                 {std::cos(angle) * spd, std::sin(angle) * spd},
                                 0.75F,
                                 Color{255, 215, 0, 255}});
      }
      app.floatingText.push_back({{app.weakSpotPos.x - 60.0F, app.weakSpotPos.y - 40.0F},
                                  "+CRITICAL! WEAK SPOT!",
                                  1.4F,
                                  Color{255, 220, 50, 255}});
    }
  }

  if (!hitWeakSpot) {
    if (app.game.player.vows.shatterer >= 1 && (app.game.player.clicks + 1) % 25 == 0) {
      isCrit = true;
    } else {
      const int roll = static_cast<int>(app.rng.next() % 10000);
      if (roll < stats.critChanceBasisPoints) {
        isCrit = true;
      }
    }
  }

  const double power = stats.miningPower;
  const std::int64_t damage = static_cast<std::int64_t>(std::max(
      1.0, ed::miningDamage(power, stats.miningPowerBonusPercent, isCrit,
                            critMultiplier,
                            app.block.hardness, stats.pierce)));
  app.block.currentHp = std::max<std::int64_t>(0, app.block.currentHp - damage);
  ++app.game.player.clicks;
  if (app.screen == Screen::Mine) {
    app.blockPulse = 0.16F;
    spawnHitEffects(app, clickPosition, materialColor(app.blockSpec), static_cast<int>(std::min<std::int64_t>(damage, 999999999)));
    if (isCrit && !hitWeakSpot) {
      app.floatingText.push_back({{clickPosition.x - 20.0F, clickPosition.y - 100.0F},
                                  "CRITICAL!",
                                  1.0F,
                                  Color{255, 200, 60, 255}});
      app.shake = 0.2F;
    }
  }

  if (app.block.currentHp > 0) {
    return;
  }

  ++app.game.player.blocksBroken;
  const std::int64_t addedGold = static_cast<std::int64_t>(app.block.sell) * app.blockSpec.dropMultiplier;
  const int gold = static_cast<int>(std::min<std::int64_t>(std::numeric_limits<int>::max(), addedGold));
  const int ore = app.blockSpec.dropMultiplier;
  ed::addItemToInventory(app.inventory, "ore", app.block.materialId, ore);
  app.game.player.gold = static_cast<int>(std::min<std::int64_t>(
      std::numeric_limits<int>::max(),
      static_cast<std::int64_t>(app.game.player.gold) + addedGold));
  app.game.player.goldEarnedThisRun = static_cast<int>(std::min<std::int64_t>(
      std::numeric_limits<int>::max(),
      static_cast<std::int64_t>(app.game.player.goldEarnedThisRun) + addedGold));
  const std::int64_t earnedXp = isAutoMine
      ? std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(static_cast<double>(app.block.xp) * 0.001)))
      : app.block.xp;
  ed::applyXp(app.game, earnedXp);

  bool shardDropped = false;
  if (app.blockSpec.aetherShardChanceBasisPoints > 0) {
    const int roll = static_cast<int>(app.rng.next() % 10000);
    if (roll < app.blockSpec.aetherShardChanceBasisPoints) {
      ++app.game.player.aetherShards;
      shardDropped = true;
      if (app.screen == Screen::Mine) {
        app.floatingText.push_back({{640.0F - 60.0F, 220.0F}, "+1 Aether Shard!", 2.0F, Color{124, 247, 224, 255}});
      }
    }
  }

  if (app.game.player.vows.dustCollector > 0) {
    const int dustRoll = static_cast<int>(app.rng.next() % 100);
    if (dustRoll < app.game.player.vows.dustCollector * 2) {
      ++app.game.player.arcaneDust;
      if (app.screen == Screen::Mine) {
        app.floatingText.push_back({{640.0F + 40.0F, 220.0F}, "+1 Arcane Dust!", 2.0F, Color{220, 150, 255, 255}});
      }
    }
  }

  if (app.blockSpec.depth > 0) {
    if (app.blockSpec.depth > app.game.player.deepestDepth) {
      app.game.player.deepestDepth = app.blockSpec.depth;
    }
    if (app.deepStrataAutoAdvance && app.deepStrataDepth == app.blockSpec.depth) {
      app.deepStrataDepth = app.blockSpec.depth + 1;
    }
  }

  if (app.screen == Screen::Mine) {
    std::string msg = "Block shattered! +" + ed::formatCount(earnedXp) + " XP, +" +
                      ed::formatCount(gold) + " gold";
    if (ore > 1) {
      msg += ", +" + ed::formatCount(ore) + " " + app.blockSpec.materialName;
    }
    if (shardDropped) {
      msg += ", +1 Aether Shard!";
    } else {
      msg += ".";
    }
    setNotice(app, msg);
  }

  spawnBlock(app);
  app.game.dirty = true;
}

void startBossCombat(AppState& app, std::size_t bossIndex, int stars, bool isEcho, int echoRank) {
  if (bossIndex >= app.bosses.size()) return;
  const auto& boss = app.bosses[bossIndex];
  const auto playerStats = ed::calculatePlayerStats(app.game, app.inventory);

  app.campaignAttempt = false;
  app.inBossCombat = true;
  app.bossCombatShowResults = false;
  app.bossCombatRewardsClaimed = false;
  app.bossCombatRewardGold = 0;
  app.bossCombatRewardXp = 0;
  app.bossCombatRewardSigils = 0;
  app.bossCombatRewardLegendary.clear();
  app.bossCombatLogIndex = 0;
  app.bossCombatTimer = 0.0F;

  app.bossCombatPlayerMaxHp = std::max<std::int64_t>(1, playerStats.maxHp);
  app.bossCombatPlayerHp = app.bossCombatPlayerMaxHp;
  app.bossCombatBossIndex = static_cast<int>(bossIndex);
  app.bossCombatStars = stars;
  app.bossCombatIsEcho = isEcho;
  app.bossCombatEchoRank = echoRank;
  app.bossCombatBossName = (isEcho ? "Echo of " : "") + boss.name;
  app.bossCombatBossAbility = boss.abilityName;

  const auto effBoss = ed::getEffectiveBossDef(boss, isEcho, echoRank);
  const double bossHpStartMult = stars >= 3 ? 0.85 : (stars >= 2 ? 0.92 : 1.0);
  app.bossCombatBossMaxHp = effBoss.maxHp;
  app.bossCombatBossHp = std::max<std::int64_t>(1, static_cast<std::int64_t>(std::llround(effBoss.maxHp * bossHpStartMult)));

  const std::uint64_t combatSeed =
      (static_cast<std::uint64_t>(app.rng.next()) << 32) | app.rng.next();
  app.bossCombatResult = ed::simulateBossCombat(playerStats, boss, stars, isEcho, echoRank, combatSeed);

  app.bossBattleLog.clear();
  if (stars >= 3) {
    app.bossBattleLog.push_back("Minigame 3-Star Mastery! +30% Stats, Boss starts at 85% HP!");
  } else if (stars >= 2) {
    app.bossBattleLog.push_back("Minigame 2-Star Clear! +15% Stats, Boss starts at 92% HP!");
  } else {
    app.bossBattleLog.push_back("Minigame 1-Star Clear! Boss engaged in combat.");
  }
}

void finishBossCombat(AppState& app) {
  if (!app.inBossCombat) return;
  app.inBossCombat = false;
  app.bossCombatShowResults = false;
  app.bossCombatRewardsClaimed = false;
}

void attemptCampaign(AppState& app) {
  const auto index = static_cast<std::size_t>(app.selectedBoss);
  if (!ed::isBossUnlocked(app.game, app.campaign, app.bosses, index)) {
    setNotice(app, "Boss locked: level or previous clear required.");
    return;
  }

  const auto& boss = app.bosses[index];

  if (app.echoCampaignMode) {
    const int effectiveEchoRank = std::max(1, app.game.player.echoRank);
    const double echoMod = ed::echoDifficultyMultiplier(effectiveEchoRank);
    const int scaledClearScore = static_cast<int>(std::llround(boss.clearScore * echoMod));
    const bool cleared = (app.campaignScore >= scaledClearScore && app.campaignHeartsLost < 3);
    if (!cleared) {
      setNotice(app, "Echo Minigame Defeat. Needed " + std::to_string(scaledClearScore) + " score.");
      app.campaignAttempt = false;
      return;
    }
    int stars = 1;
    if (app.campaignHeartsLost == 0 && app.campaignScore >= static_cast<int>(scaledClearScore * 1.2)) {
      stars = 3;
    } else if (app.campaignHeartsLost <= 1) {
      stars = 2;
    }
    startBossCombat(app, index, stars, true, effectiveEchoRank);
    return;
  }

  const auto outcome = ed::evaluateBossAttempt(
      app.game, app.campaign, app.bosses, index,
      {app.campaignScore, app.campaignHeartsLost, true, 0.0});
  if (!outcome.cleared) {
    setNotice(app, "Minigame Defeat. Raise score or preserve hearts.");
    app.campaignAttempt = false;
    return;
  }

  startBossCombat(app, index, outcome.stars, false, 1);
}

void startCampaignAttempt(AppState& app) {
  const auto bossIndex = static_cast<std::size_t>(app.selectedBoss);
  if (!ed::isBossUnlocked(app.game, app.campaign, app.bosses, bossIndex)) {
    setNotice(app, "Boss locked: level or previous boss clear required.");
    return;
  }
  app.campaignAttempt = true;
  app.campaignScore = 0;
  app.campaignHeartsLost = 0;
  if (app.echoCampaignMode) {
    setNotice(app, "Echo Challenge: " + app.bosses[bossIndex].name + " (Rank " +
                      std::to_string(std::max(1, app.game.player.echoRank)) + ")");
  }
  const auto challenge = app.bosses[bossIndex].challenge;
  if (challenge == ed::BossChallenge::FrenzyDig) {
    app.frenzyDig = ed::startFrenzyDig(
        0xB055ULL + static_cast<std::uint64_t>(app.game.player.clicks));
    setNotice(app, "Frenzy Dig! Click the throne rock or press Space for 10 seconds.");
  } else if (challenge == ed::BossChallenge::BoneDuel) {
    app.boneDuel = ed::startBoneDuel(
        0xB00EULL + static_cast<std::uint64_t>(app.game.player.clicks));
    setNotice(app, "Bone Duel! Stop the needle inside the green zone.");
  } else if (challenge == ed::BossChallenge::CinderDance) {
    app.cinderDance = ed::startCinderDance(
        0xC1DEULL + static_cast<std::uint64_t>(app.game.player.clicks));
    setNotice(app, "Cinder Dance! Hit D, F, J, or K on the rhythm line.");
  } else if (challenge == ed::BossChallenge::HeadHunt) {
    app.headHunt = ed::startHeadHunt(
        0x1EADULL + static_cast<std::uint64_t>(app.game.player.clicks));
    setNotice(app, "Head Hunt! Strike green heads and avoid blue decoys.");
  } else if (challenge == ed::BossChallenge::ParryDuel) {
    app.parryDuel = ed::startParryDuel(
        0xA1D1CULL + static_cast<std::uint64_t>(app.game.player.clicks));
    setNotice(app, "Parry Duel! Match the telegraphed attack direction.");
  } else if (challenge == ed::BossChallenge::RuneRecall) {
    app.runeRecall = ed::startRuneRecall(
        0x7A11ULL + static_cast<std::uint64_t>(app.game.player.clicks));
    setNotice(app, "Rune Recall! Memorize the sequence and repeat it.");
  } else if (challenge == ed::BossChallenge::StarTrace) {
    app.starTrace = ed::startStarTrace(
        0x57A2ULL + static_cast<std::uint64_t>(app.game.player.clicks));
    setNotice(app, "Star Trace! Tap the numbered stars in order.");
  } else if (challenge == ed::BossChallenge::Heartpierce) {
    const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
    app.heartpierce = ed::startHeartpierce(
        0x1A671ULL + static_cast<std::uint64_t>(app.game.player.clicks),
        static_cast<float>(stats.focusBasisPoints) / 10000.0F, 0,
        app.game.player.echoRank);
    setNotice(app, "Heartpierce! Hold Space or mouse to raise the lance and track Ignathia's heart.");
  } else if (challenge == ed::BossChallenge::SoulDodge) {
    app.soulDodge = ed::startSoulDodge(
        0x50ULL + static_cast<std::uint64_t>(app.game.player.clicks));
    setNotice(app, "Soul Dodge! Survive the pattern waves.");
  } else {
    app.fiveTrials = ed::startFiveTrials();
    app.cinderDance = ed::startCinderDance(0xF17EULL, 30.0F, 36);
    app.cinderDance.gauntletMode = true;
    setNotice(app, "The Five Trials begin. Complete every phase.");
  }
}

void runDungeon(AppState& app) {
  const auto playerStats = ed::calculatePlayerStats(app.game, app.inventory);
  ed::DungeonPlayerStats stats;
  stats.maxHp = playerStats.maxHp;
  stats.attack = playerStats.attack;
  stats.defense = playerStats.defense;
  stats.speed = playerStats.speed;
  stats.critChanceBasisPoints = playerStats.critChanceBasisPoints;
  stats.critDamageBasisPoints = playerStats.critDamageBasisPoints;
  stats.secondWind = (app.game.player.vows.secondWind >= 1);

  if (app.abyssDungeonMode) {
    const auto result = ed::simulateAbyssRun(stats, 0xAB755ULL + app.game.player.clicks);
    app.dungeonResult = result;
    app.inDungeonCombat = true;
    app.dungeonShowResults = false;
    app.dungeonLogIndex = 0;
    app.dungeonCombatTimer = 0.0F;
    app.dungeonPlayerMaxHp = stats.maxHp;
    app.dungeonPlayerHp = stats.maxHp;
    app.dungeonEnemies.clear();
    app.dungeonBattleLog.clear();
    app.dungeonBattleLog.push_back("Descended into The Abyss!");

    // Apply Abyss rewards earned across cleared floors
    app.game.player.gold = static_cast<int>(std::min<std::int64_t>(
        std::numeric_limits<int>::max(),
        static_cast<std::int64_t>(app.game.player.gold) + static_cast<std::int64_t>(result.gold)));
    app.game.player.goldEarnedThisRun = static_cast<int>(std::min<std::int64_t>(
        std::numeric_limits<int>::max(),
        static_cast<std::int64_t>(app.game.player.goldEarnedThisRun) + static_cast<std::int64_t>(result.gold)));
    ed::applyXp(app.game, result.xp);
    app.game.player.arcaneDust += result.arcaneDust;
    app.game.player.echoSigils += result.echoSigils;
    app.game.player.aetherShards += result.aetherShards;
    if (result.floorsCleared > app.game.player.abyssBestFloor) {
      app.game.player.abyssBestFloor = result.floorsCleared;
    }
    app.game.dirty = true;
    return;
  }

  const auto& zone = app.zones[static_cast<std::size_t>(app.selectedZone)];
  const auto result =
      ed::simulateDungeonRun(stats, zone, 0xD00DULL + app.game.player.clicks);

  app.dungeonResult = result;
  app.inDungeonCombat = true;
  app.dungeonShowResults = false;
  app.dungeonLogIndex = 0;
  app.dungeonCombatTimer = 0.0F;
  app.dungeonPlayerMaxHp = stats.maxHp;
  app.dungeonPlayerHp = stats.maxHp;
  app.dungeonEnemies.clear();
  app.dungeonBattleLog.clear();
  app.dungeonBattleLog.push_back("Entered " + zone.name + "!");

  if (result.cleared) {
    app.game.player.gold = static_cast<int>(std::min<std::int64_t>(
        std::numeric_limits<int>::max(),
        static_cast<std::int64_t>(app.game.player.gold) + static_cast<std::int64_t>(result.gold)));
    app.game.player.goldEarnedThisRun = static_cast<int>(std::min<std::int64_t>(
        std::numeric_limits<int>::max(),
        static_cast<std::int64_t>(app.game.player.goldEarnedThisRun) + static_cast<std::int64_t>(result.gold)));
    ed::applyXp(app.game, result.xp);
    app.game.player.arcaneDust += result.arcaneDust;
    ed::addItemToInventory(app.inventory, "trophies",
                           "zone-" + std::to_string(zone.tier), result.trophies);
  }
  app.game.dirty = true;
}

void startDescentVaultMinigame(AppState& app) {
  if (app.bosses.empty()) return;
  const int bossIdx = rand() % std::min(static_cast<int>(app.bosses.size()), 9);
  app.descentMinigameBossIdx = bossIdx;
  app.descentMinigameActive = true;
  app.campaignAttempt = true;
  app.campaignIntro = false;
  app.campaignScore = 0;
  app.campaignHeartsLost = 0;

  const auto challenge = app.bosses[static_cast<std::size_t>(bossIdx)].challenge;
  if (challenge == ed::BossChallenge::FrenzyDig) {
    app.frenzyDig = ed::startFrenzyDig(0xB055ULL + static_cast<std::uint64_t>(app.game.player.clicks + rand()));
    setNotice(app, "Vault Lock: Frenzy Dig! Click the rock or press Space repeatedly.");
  } else if (challenge == ed::BossChallenge::BoneDuel) {
    app.boneDuel = ed::startBoneDuel(0xB00EULL + static_cast<std::uint64_t>(app.game.player.clicks + rand()));
    setNotice(app, "Vault Lock: Bone Duel! Press Space inside the green zone.");
  } else if (challenge == ed::BossChallenge::CinderDance) {
    app.cinderDance = ed::startCinderDance(0xC1DEULL + static_cast<std::uint64_t>(app.game.player.clicks + rand()), 25.0F, 35);
    setNotice(app, "Vault Lock: Cinder Dance! Hit D, F, J, K on the rhythm lines.");
  } else if (challenge == ed::BossChallenge::HeadHunt) {
    app.headHunt = ed::startHeadHunt(0x1EADULL + static_cast<std::uint64_t>(app.game.player.clicks + rand()));
    setNotice(app, "Vault Lock: Head Hunt! Click green targets and avoid blue decoys.");
  } else if (challenge == ed::BossChallenge::ParryDuel) {
    app.parryDuel = ed::startParryDuel(0xA1D1CULL + static_cast<std::uint64_t>(app.game.player.clicks + rand()), 12);
    setNotice(app, "Vault Lock: Parry Duel! Match the telegraphed directions.");
  } else if (challenge == ed::BossChallenge::RuneRecall) {
    app.runeRecall = ed::startRuneRecall(0x7A11ULL + static_cast<std::uint64_t>(app.game.player.clicks + rand()));
    setNotice(app, "Vault Lock: Rune Recall! Repeat the sequence.");
  } else if (challenge == ed::BossChallenge::StarTrace) {
    app.starTrace = ed::startStarTrace(0x57A2ULL + static_cast<std::uint64_t>(app.game.player.clicks + rand()));
    setNotice(app, "Vault Lock: Star Trace! Connect the numbered stars in order.");
  } else if (challenge == ed::BossChallenge::Heartpierce) {
    const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
    app.heartpierce = ed::startHeartpierce(0x1A671ULL + static_cast<std::uint64_t>(app.game.player.clicks + rand()),
                                           static_cast<float>(stats.focusBasisPoints) / 10000.0F, 0, 1);
    setNotice(app, "Vault Lock: Heartpierce! Track the moving target with your lance.");
  } else if (challenge == ed::BossChallenge::SoulDodge) {
    app.soulDodge = ed::startSoulDodge(0x50ULL + static_cast<std::uint64_t>(app.game.player.clicks + rand()));
    setNotice(app, "Vault Lock: Soul Dodge! Dodge the incoming projectile patterns.");
  } else {
    app.boneDuel = ed::startBoneDuel(0xB00EULL + static_cast<std::uint64_t>(app.game.player.clicks + rand()));
    setNotice(app, "Vault Lock: Bone Duel! Press Space inside the green zone.");
  }
}

void finishDescentVaultMinigame(AppState& app, bool success) {
  app.descentMinigameActive = false;
  app.campaignAttempt = false;
  app.descentChestOpened = true;
  app.descentPopupSuccess = success;
  app.descentPopupTimer = 2.0F;

  if (success) {
    app.game.player.descentVaultsCracked++;
    app.game.dirty = true;
    app.inventory.gems["emerald"] += 2;
    app.inventory.gems["ruby"] += 2;
    app.inventory.gems["sapphire"] += 2;
    app.inventory.gems["diamond"] += 1;
    app.inventory.gems["amethyst"] += 2;
    app.descentPopupMsg = "VAULT UNLOCKED! Gained: 2 Emeralds, 2 Rubies, 2 Sapphires, 2 Amethysts, 1 Diamond!";
    setNotice(app, "Treasure Vault Unlocked! Rare gems added to inventory.");
  } else {
    app.descentPopupMsg = "CHALLENGE FAILED! The vault lock remained sealed.";
    setNotice(app, "Vault challenge failed. No penalties.");
  }

  // Generate 3 new doors to the next rooms
  app.descentSim.roomCleared = true;
  app.descentSim.doorChoicesPending = true;
  ed::descent::generateChamberChoices(app.descentSim);
}

bool socketGemIntoGear(AppState& app, int gearIndex, int socketIndex, const std::string& gemId) {
  if (gearIndex < 0 || gearIndex >= static_cast<int>(app.inventory.gear.size())) return false;
  auto& piece = app.inventory.gear[static_cast<std::size_t>(gearIndex)];
  if (socketIndex < 0 || socketIndex >= piece.sockets) return false;

  std::string invKey = gemId;
  if (ed::countItem(app.inventory, "gems", invKey) <= 0) {
    if (invKey.rfind("rough-", 0) == 0) {
      std::string unprefixed = invKey.substr(6);
      if (ed::countItem(app.inventory, "gems", unprefixed) > 0) {
        invKey = unprefixed;
      }
    } else if (invKey.find('-') == std::string::npos) {
      std::string prefixed = "rough-" + invKey;
      if (ed::countItem(app.inventory, "gems", prefixed) > 0) {
        invKey = prefixed;
      }
    }
  }
  if (ed::countItem(app.inventory, "gems", invKey) <= 0) {
    setNotice(app, "You do not have that gem in inventory.");
    return false;
  }

  if (socketIndex < static_cast<int>(piece.socketedGems.size())) {
    const std::string oldGem = piece.socketedGems[static_cast<std::size_t>(socketIndex)];
    ed::removeGem(app.inventory, piece, static_cast<std::size_t>(socketIndex));
    if (ed::removeItemFromInventory(app.inventory, "gems", invKey, 1)) {
      piece.socketedGems.insert(piece.socketedGems.begin() + socketIndex, gemId);
      app.game.dirty = true;
      setNotice(app, "Replaced " + ed::gemDisplayName(oldGem) + " with " + ed::gemDisplayName(gemId) + ".");
      return true;
    } else {
      piece.socketedGems.insert(piece.socketedGems.begin() + socketIndex, oldGem);
      ed::removeItemFromInventory(app.inventory, "gems", oldGem, 1);
      return false;
    }
  } else {
    if (static_cast<int>(piece.socketedGems.size()) < piece.sockets) {
      if (ed::removeItemFromInventory(app.inventory, "gems", invKey, 1)) {
        piece.socketedGems.push_back(gemId);
        app.game.dirty = true;
        setNotice(app, "Socketed " + ed::gemDisplayName(gemId) + ".");
        return true;
      }
    }
  }
  return false;
}

bool unsocketGemFromGear(AppState& app, int gearIndex, int socketIndex) {
  if (gearIndex < 0 || gearIndex >= static_cast<int>(app.inventory.gear.size())) return false;
  auto& piece = app.inventory.gear[static_cast<std::size_t>(gearIndex)];
  if (socketIndex < 0 || socketIndex >= static_cast<int>(piece.socketedGems.size())) return false;
  const std::string oldGem = piece.socketedGems[static_cast<std::size_t>(socketIndex)];
  if (ed::removeGem(app.inventory, piece, static_cast<std::size_t>(socketIndex))) {
    app.game.dirty = true;
    setNotice(app, "Unsocketed " + ed::gemDisplayName(oldGem) + ".");
    return true;
  }
  return false;
}

} // namespace ed::app
