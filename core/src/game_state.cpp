#include "ed/game_state.hpp"

#include <algorithm>
#include <cctype>
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
    const int converted = std::stoi(value, &parsed);
    if (parsed != value.size()) {
      return false;
    }
    out = converted;
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

void applyXp(GameState& game, int earnedXp) {
  if (earnedXp <= 0) {
    return;
  }

  game.dirty = true;
  game.player.xp += earnedXp;

  constexpr int levelCap = 60;
  while (game.player.level < levelCap && game.player.xp >= xpToNextLevel(game.player.level)) {
    game.player.xp -= xpToNextLevel(game.player.level);
    ++game.player.level;
    ++game.player.vowPoints;
  }
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
    }
  }

  return true;
}

} // namespace ed
