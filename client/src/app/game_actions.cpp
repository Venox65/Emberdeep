#include "game_actions.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

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
    return "Need " + std::to_string(recipe.gold - app.game.player.gold) + " more gold.";
  }
  const std::string materialId = materialIdForTier(recipe.tier);
  const int ingots = ed::countItem(app.inventory, "ingots", materialId);
  if (ingots < recipe.ingots) {
    return "Need " + std::to_string(recipe.ingots - ingots) + " more " + materialId + " ingot(s).";
  }
  const std::string trophyId = "zone-" + std::to_string(recipe.tier);
  const int trophies = ed::countItem(app.inventory, "trophies", trophyId);
  if (trophies < recipe.trophies) {
    return "Need " + std::to_string(recipe.trophies - trophies) + " more tier " +
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

void mine(AppState& app, Vector2 clickPosition) {
  const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
  const double power = stats.miningPower;
  const std::int64_t damage = static_cast<std::int64_t>(std::max(
      1.0, ed::miningDamage(power, stats.miningPowerBonusPercent, false,
                            static_cast<double>(stats.critDamageBasisPoints) / 10000.0,
                            app.block.hardness, stats.pierce)));
  app.block.currentHp = std::max<std::int64_t>(0, app.block.currentHp - damage);
  ++app.game.player.clicks;
  app.blockPulse = 0.16F;
  spawnHitEffects(app, clickPosition, materialColor(app.blockSpec), static_cast<int>(std::min<std::int64_t>(damage, 999999999)));

  if (app.block.currentHp > 0) {
    return;
  }

  ++app.game.player.blocksBroken;
  const int gold = app.block.sell * app.blockSpec.dropMultiplier;
  const int ore = app.blockSpec.dropMultiplier;
  ed::addItemToInventory(app.inventory, "ore", app.block.materialId, ore);
  app.game.player.gold += gold;
  app.game.player.goldEarnedThisRun += gold;
  ed::applyXp(app.game, app.block.xp);

  bool shardDropped = false;
  if (app.blockSpec.aetherShardChanceBasisPoints > 0) {
    const int roll = static_cast<int>(app.rng.next() % 10000);
    if (roll < app.blockSpec.aetherShardChanceBasisPoints) {
      ++app.game.player.aetherShards;
      shardDropped = true;
      app.floatingText.push_back({{640.0F - 60.0F, 220.0F}, "+1 Aether Shard!", 2.0F, Color{124, 247, 224, 255}});
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

  std::string msg = "Block shattered! +" + std::to_string(app.block.xp) + " XP, +" +
                    std::to_string(gold) + " gold";
  if (ore > 1) {
    msg += ", +" + std::to_string(ore) + " " + app.blockSpec.materialName;
  }
  if (shardDropped) {
    msg += ", +1 Aether Shard!";
  } else {
    msg += ".";
  }
  setNotice(app, msg);

  spawnBlock(app);
  app.game.dirty = true;
}

void attemptCampaign(AppState& app) {
  const auto index = static_cast<std::size_t>(app.selectedBoss);
  if (!ed::isBossUnlocked(app.game, app.campaign, app.bosses, index)) {
    setNotice(app, "Boss locked: level or previous clear required.");
    return;
  }

  const auto& boss = app.bosses[index];
  const auto outcome = ed::evaluateBossAttempt(
      app.game, app.campaign, app.bosses, index,
      {app.campaignScore, app.campaignHeartsLost, true, 0.0});
  if (!outcome.cleared) {
    setNotice(app, "Defeat. Raise the score with Space or mouse clicks.");
    app.campaignAttempt = false;
    return;
  }
  ed::applyBossOutcome(app.game, outcome);
  std::string message = "Cleared " + boss.name + " with " + std::to_string(outcome.stars) +
                        " star(s)!";
  if (!outcome.legendaryId.empty()) {
    message += " Legendary earned.";
  }
  setNotice(app, message);
  app.campaignAttempt = false;
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
  const auto& zone = app.zones[static_cast<std::size_t>(app.selectedZone)];
  const auto playerStats = ed::calculatePlayerStats(app.game, app.inventory);
  ed::DungeonPlayerStats stats;
  stats.maxHp = playerStats.maxHp;
  stats.attack = playerStats.attack;
  stats.defense = playerStats.defense;
  stats.speed = playerStats.speed;
  stats.critChanceBasisPoints = playerStats.critChanceBasisPoints;
  stats.critDamageBasisPoints = playerStats.critDamageBasisPoints;

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
    app.game.player.gold += static_cast<int>(result.gold);
    app.game.player.goldEarnedThisRun += static_cast<int>(result.gold);
    ed::applyXp(app.game, result.xp);
    app.game.player.arcaneDust += result.arcaneDust;
    ed::addItemToInventory(app.inventory, "trophies",
                           "zone-" + std::to_string(zone.tier), result.trophies);
  }
  app.game.dirty = true;
}

} // namespace ed::app
