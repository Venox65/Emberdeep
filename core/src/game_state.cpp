#include "ed/game_state.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace ed {

namespace {

std::string trim(const std::string& value) {
  const auto start = value.find_first_not_of(" \t\r\n");
  if (start == std::string::npos) {
    return "";
  }
  const auto end = value.find_last_not_of(" \t\r\n");
  return value.substr(start, end - start + 1);
}

std::pair<std::string, std::string> splitKeyValue(const std::string& line) {
  const auto pos = line.find('=');
  if (pos == std::string::npos) {
    return {trim(line), ""};
  }
  return {trim(line.substr(0, pos)), trim(line.substr(pos + 1))};
}

bool parseIntValue(const std::string& value, int& out) {
  try {
    size_t parsed = 0;
    const long long converted = std::stoll(value, &parsed);
    if (parsed != value.size()) {
      return false;
    }
    // Clamp overflowed values instead of failing
    if (converted > std::numeric_limits<int>::max()) {
      out = std::numeric_limits<int>::max();
    } else if (converted < std::numeric_limits<int>::min()) {
      out = std::numeric_limits<int>::min();
    } else {
      out = static_cast<int>(converted);
    }
    return true;
  } catch (...) {
    return false;
  }
}

} // namespace

bool canUnlockLayer(const GameState& game, const LayerDef& layer) {
  if (layer.minLevel > game.player.level) {
    return false;
  }
  if (game.player.pickaxeTier < layer.requiredPickaxeTier) {
    return false;
  }
  return true;
}

int playerLevelCap(const PlayerState& player) {
  constexpr int baseCap = 60;
  constexpr int maxCap = 100;
  const int bonus = (std::clamp(player.emberPerks.potential, 0, 24) / 3) * 5;
  return std::min(maxCap, baseCap + bonus);
}

void applyXp(GameState& game, std::int64_t earnedXp) {
  if (earnedXp <= 0) {
    return;
  }

  game.dirty = true;
  std::int64_t remainingXp = earnedXp + static_cast<std::int64_t>(game.player.xp);
  const int levelCap = playerLevelCap(game.player);
  while (game.player.level < levelCap && remainingXp >= xpToNextLevel(game.player.level)) {
    remainingXp -= xpToNextLevel(game.player.level);
    ++game.player.level;
    ++game.player.vowPoints;
  }
  game.player.xp = static_cast<int>(std::min<std::int64_t>(std::numeric_limits<int>::max(), remainingXp));
}

std::string serializeGameState(const GameState& game) {
  std::ostringstream out;
  out << "schema_version=" << game.schemaVersion << '\n';
  out << "save_id=" << game.saveId << '\n';
  out << "dirty=" << (game.dirty ? 1 : 0) << '\n';
  out << "level=" << game.player.level << '\n';
  out << "xp=" << game.player.xp << '\n';
  out << "gold=" << game.player.gold << '\n';
  out << "arcane_dust=" << game.player.arcaneDust << '\n';
  out << "boss_essence=" << game.player.bossEssence << '\n';
  out << "echo_sigils=" << game.player.echoSigils << '\n';
  out << "aether_shards=" << game.player.aetherShards << '\n';
  out << "ember_marks=" << game.player.emberMarks << '\n';
  out << "vow_points=" << game.player.vowPoints << '\n';
  out << "blocks_broken=" << game.player.blocksBroken << '\n';
  out << "clicks=" << game.player.clicks << '\n';
  out << "bosses_cleared=" << game.player.bossesCleared << '\n';
  out << "ascensions=" << game.player.ascensions << '\n';
  out << "echo_rank=" << game.player.echoRank << '\n';
  out << "deepest_depth=" << game.player.deepestDepth << '\n';
  out << "gold_earned_this_run=" << game.player.goldEarnedThisRun << '\n';
  out << "pickaxe_tier=" << game.player.pickaxeTier << '\n';
  out << "current_layer=" << game.player.currentLayer << '\n';
  out << "abyss_best_floor=" << game.player.abyssBestFloor << '\n';
  out << "perk_power=" << game.player.emberPerks.power << '\n';
  out << "perk_wealth=" << game.player.emberPerks.wealth << '\n';
  out << "perk_wisdom=" << game.player.emberPerks.wisdom << '\n';
  out << "perk_potential=" << game.player.emberPerks.potential << '\n';
  out << "perk_armory=" << game.player.emberPerks.armory << '\n';
  out << "perk_smelting=" << game.player.emberPerks.smelting << '\n';
  out << "vow_strong_arms=" << game.player.vows.strongArms << '\n';
  out << "vow_keen_eye=" << game.player.vows.keenEye << '\n';
  out << "vow_prospector=" << game.player.vows.prospector << '\n';
  out << "vow_deep_breath=" << game.player.vows.deepBreath << '\n';
  out << "vow_shatterer=" << game.player.vows.shatterer << '\n';
  out << "vow_stout_heart=" << game.player.vows.stoutHeart << '\n';
  out << "vow_whetstone=" << game.player.vows.whetstone << '\n';
  out << "vow_ironskin=" << game.player.vows.ironskin << '\n';
  out << "vow_swift_boots=" << game.player.vows.swiftBoots << '\n';
  out << "vow_second_wind=" << game.player.vows.secondWind << '\n';
  out << "vow_quick_study=" << game.player.vows.quickStudy << '\n';
  out << "vow_haggler=" << game.player.vows.haggler << '\n';
  out << "vow_steady_hand=" << game.player.vows.steadyHand << '\n';
  out << "vow_dust_collector=" << game.player.vows.dustCollector << '\n';
  out << "vow_gem_whisperer=" << game.player.vows.gemWhisperer << '\n';
  out << "echo_cleared=";
  for (std::size_t i = 0; i < game.player.echoCleared.size(); ++i) {
    if (i > 0) out << ",";
    out << (game.player.echoCleared[i] ? 1 : 0);
  }
  out << '\n';
  out << "last_gem_hollow_date=" << game.player.lastGemHollowDate << '\n';
  out << "gem_hollow_streak=" << game.player.gemHollowStreak << '\n';
  out << "gem_hollow_swings_remaining=" << game.player.gemHollowSwingsRemaining << '\n';
  out << "gem_hollow_today_gems=" << game.player.gemHollowTodayGems << '\n';
  out << "gem_hollow_today_dust=" << game.player.gemHollowTodayDust << '\n';
  out << "gem_hollow_today_gold=" << game.player.gemHollowTodayGold << '\n';
  out << "gem_hollow_active=" << (game.player.gemHollowActive ? 1 : 0) << '\n';
  out << "gem_hollow_veins=";
  for (std::size_t i = 0; i < game.player.gemHollowVeins.size(); ++i) {
    if (i > 0) out << ",";
    out << game.player.gemHollowVeins[i];
  }
  out << '\n';
  out << "weak_spots_hit=" << game.player.weakSpotsHit << '\n';
  out << "achievements=";
  for (std::size_t i = 0; i < game.player.achievements.size(); ++i) {
    if (i > 0) out << ",";
    out << game.player.achievements[i];
  }
  out << '\n';
  out << "chrono_cores=" << game.player.chronoCores << '\n';
  out << "auto_clicker_level=" << game.player.autoClickerLevel << '\n';
  out << "auto_clicker_enabled=" << (game.player.autoClickerEnabled ? 1 : 0) << '\n';
  out << "last_save_time=" << game.player.lastSaveTime << '\n';
  out << "descent_runs=" << game.player.descentRuns << '\n';
  out << "descent_victories=" << game.player.descentVictories << '\n';
  out << "descent_best_chamber=" << game.player.descentBestChamber << '\n';
  out << "descent_vaults_cracked=" << game.player.descentVaultsCracked << '\n';
  out << "descent_max_run_level=" << game.player.descentMaxRunLevel << '\n';
  out << "descent_endless_best_chamber=" << game.player.descentEndlessBestChamber << '\n';
  return out.str();
}

bool deserializeGameState(const std::string& text, GameState& game) {
  std::istringstream input(text);
  std::string line;

  game = GameState{};
  while (std::getline(input, line)) {
    const auto trimmed = trim(line);
    if (trimmed.empty()) {
      continue;
    }
    const auto [key, value] = splitKeyValue(trimmed);
    if (key.empty()) {
      continue;
    }

    if (key == "schema_version") {
      if (!parseIntValue(value, game.schemaVersion)) {
        return false;
      }
    } else if (key == "save_id") {
      game.saveId = value;
    } else if (key == "dirty") {
      int parsed = 0;
      if (!parseIntValue(value, parsed)) {
        return false;
      }
      game.dirty = parsed != 0;
    } else if (key == "level") {
      if (!parseIntValue(value, game.player.level)) {
        return false;
      }
    } else if (key == "xp") {
      if (!parseIntValue(value, game.player.xp)) {
        return false;
      }
    } else if (key == "gold") {
      if (!parseIntValue(value, game.player.gold)) {
        return false;
      }
    } else if (key == "arcane_dust") {
      if (!parseIntValue(value, game.player.arcaneDust)) {
        return false;
      }
    } else if (key == "boss_essence") {
      if (!parseIntValue(value, game.player.bossEssence)) {
        return false;
      }
    } else if (key == "echo_sigils") {
      if (!parseIntValue(value, game.player.echoSigils)) {
        return false;
      }
    } else if (key == "aether_shards") {
      if (!parseIntValue(value, game.player.aetherShards)) {
        return false;
      }
    } else if (key == "ember_marks") {
      if (!parseIntValue(value, game.player.emberMarks)) {
        return false;
      }
    } else if (key == "vow_points") {
      if (!parseIntValue(value, game.player.vowPoints)) {
        return false;
      }
    } else if (key == "blocks_broken") {
      if (!parseIntValue(value, game.player.blocksBroken)) {
        return false;
      }
    } else if (key == "clicks") {
      if (!parseIntValue(value, game.player.clicks)) {
        return false;
      }
    } else if (key == "bosses_cleared") {
      if (!parseIntValue(value, game.player.bossesCleared)) {
        return false;
      }
    } else if (key == "ascensions") {
      if (!parseIntValue(value, game.player.ascensions)) {
        return false;
      }
    } else if (key == "echo_rank") {
      if (!parseIntValue(value, game.player.echoRank)) {
        return false;
      }
    } else if (key == "deepest_depth") {
      if (!parseIntValue(value, game.player.deepestDepth)) {
        return false;
      }
    } else if (key == "gold_earned_this_run") {
      if (!parseIntValue(value, game.player.goldEarnedThisRun)) {
        return false;
      }
    } else if (key == "pickaxe_tier") {
      if (!parseIntValue(value, game.player.pickaxeTier)) {
        return false;
      }
    } else if (key == "current_layer") {
      game.player.currentLayer = value;
    } else if (key == "abyss_best_floor") {
      if (!parseIntValue(value, game.player.abyssBestFloor)) return false;
    } else if (key == "perk_power") {
      if (!parseIntValue(value, game.player.emberPerks.power)) return false;
    } else if (key == "perk_wealth") {
      if (!parseIntValue(value, game.player.emberPerks.wealth)) return false;
    } else if (key == "perk_wisdom") {
      if (!parseIntValue(value, game.player.emberPerks.wisdom)) return false;
    } else if (key == "perk_potential") {
      if (!parseIntValue(value, game.player.emberPerks.potential)) return false;
    } else if (key == "perk_armory") {
      if (!parseIntValue(value, game.player.emberPerks.armory)) return false;
    } else if (key == "perk_smelting") {
      if (!parseIntValue(value, game.player.emberPerks.smelting)) return false;
    } else if (key == "vow_strong_arms") {
      if (!parseIntValue(value, game.player.vows.strongArms)) return false;
    } else if (key == "vow_keen_eye") {
      if (!parseIntValue(value, game.player.vows.keenEye)) return false;
    } else if (key == "vow_prospector") {
      if (!parseIntValue(value, game.player.vows.prospector)) return false;
    } else if (key == "vow_deep_breath") {
      if (!parseIntValue(value, game.player.vows.deepBreath)) return false;
    } else if (key == "vow_shatterer") {
      if (!parseIntValue(value, game.player.vows.shatterer)) return false;
    } else if (key == "vow_stout_heart") {
      if (!parseIntValue(value, game.player.vows.stoutHeart)) return false;
    } else if (key == "vow_whetstone") {
      if (!parseIntValue(value, game.player.vows.whetstone)) return false;
    } else if (key == "vow_ironskin") {
      if (!parseIntValue(value, game.player.vows.ironskin)) return false;
    } else if (key == "vow_swift_boots") {
      if (!parseIntValue(value, game.player.vows.swiftBoots)) return false;
    } else if (key == "vow_second_wind") {
      if (!parseIntValue(value, game.player.vows.secondWind)) return false;
    } else if (key == "vow_quick_study") {
      if (!parseIntValue(value, game.player.vows.quickStudy)) return false;
    } else if (key == "vow_haggler") {
      if (!parseIntValue(value, game.player.vows.haggler)) return false;
    } else if (key == "vow_steady_hand") {
      if (!parseIntValue(value, game.player.vows.steadyHand)) return false;
    } else if (key == "vow_dust_collector") {
      if (!parseIntValue(value, game.player.vows.dustCollector)) return false;
    } else if (key == "vow_gem_whisperer") {
      if (!parseIntValue(value, game.player.vows.gemWhisperer)) return false;
    } else if (key == "echo_cleared") {
      game.player.echoCleared.clear();
      std::istringstream stream(value);
      std::string field;
      while (std::getline(stream, field, ',')) {
        if (!field.empty()) {
          try {
            game.player.echoCleared.push_back(std::stoi(field) != 0);
          } catch (...) {}
        }
      }
    } else if (key == "last_gem_hollow_date") {
      game.player.lastGemHollowDate = value;
    } else if (key == "gem_hollow_streak") {
      if (!parseIntValue(value, game.player.gemHollowStreak)) return false;
    } else if (key == "gem_hollow_swings_remaining") {
      if (!parseIntValue(value, game.player.gemHollowSwingsRemaining)) return false;
    } else if (key == "gem_hollow_today_gems") {
      if (!parseIntValue(value, game.player.gemHollowTodayGems)) return false;
    } else if (key == "gem_hollow_today_dust") {
      if (!parseIntValue(value, game.player.gemHollowTodayDust)) return false;
    } else if (key == "gem_hollow_today_gold") {
      if (!parseIntValue(value, game.player.gemHollowTodayGold)) return false;
    } else if (key == "gem_hollow_active") {
      int active = 0;
      if (!parseIntValue(value, active)) return false;
      game.player.gemHollowActive = (active != 0);
    } else if (key == "gem_hollow_veins") {
      std::istringstream veinsInput(value);
      std::string field;
      while (std::getline(veinsInput, field, ',')) {
        if (!field.empty()) {
          game.player.gemHollowVeins.push_back(std::stoi(field));
        }
      }
    } else if (key == "weak_spots_hit") {
      if (!parseIntValue(value, game.player.weakSpotsHit)) return false;
    } else if (key == "chrono_cores") {
      if (!parseIntValue(value, game.player.chronoCores)) return false;
    } else if (key == "auto_clicker_level") {
      if (!parseIntValue(value, game.player.autoClickerLevel)) return false;
    } else if (key == "auto_clicker_enabled") {
      int active = 1;
      if (!parseIntValue(value, active)) return false;
      game.player.autoClickerEnabled = (active != 0);
    } else if (key == "last_save_time") {
      try {
        game.player.lastSaveTime = std::stoll(value);
      } catch (...) {
        game.player.lastSaveTime = 0;
      }
    } else if (key == "descent_runs") {
      if (!parseIntValue(value, game.player.descentRuns)) return false;
    } else if (key == "descent_victories") {
      if (!parseIntValue(value, game.player.descentVictories)) return false;
    } else if (key == "descent_best_chamber") {
      if (!parseIntValue(value, game.player.descentBestChamber)) return false;
    } else if (key == "descent_vaults_cracked") {
      if (!parseIntValue(value, game.player.descentVaultsCracked)) return false;
    } else if (key == "descent_max_run_level") {
      if (!parseIntValue(value, game.player.descentMaxRunLevel)) return false;
    } else if (key == "descent_endless_best_chamber") {
      if (!parseIntValue(value, game.player.descentEndlessBestChamber)) return false;
    } else if (key == "achievements") {
      game.player.achievements.clear();
      std::istringstream achInput(value);
      std::string field;
      while (std::getline(achInput, field, ',')) {
        if (!field.empty()) {
          game.player.achievements.push_back(field);
        }
      }
    }
  }

  return true;
}

} // namespace ed
