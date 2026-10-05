#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ed/campaign.hpp"
#include "ed/data_model.hpp"
#include "ed/dungeon.hpp"
#include "ed/formulas.hpp"
#include "ed/game_state.hpp"
#include "ed/mining.hpp"
#include "ed/postgame.hpp"

namespace {

std::string rarityName(ed::Rarity rarity) {
  switch (rarity) {
    case ed::Rarity::Common:
      return "Common";
    case ed::Rarity::Uncommon:
      return "Uncommon";
    case ed::Rarity::Rare:
      return "Rare";
    case ed::Rarity::Epic:
      return "Epic";
  }
  return "Unknown";
}

void printStatus(const ed::GameState& game) {
  const auto& player = game.player;
  std::cout << "\nLevel " << player.level << " | XP " << player.xp << "/"
            << ed::xpToNextLevel(player.level) << " | Gold " << player.gold << '\n'
            << "Layer " << player.currentLayer << " | Pickaxe T" << player.pickaxeTier << '\n'
            << "Blocks " << player.blocksBroken << " | Clicks " << player.clicks
            << " | Bosses " << player.bossesCleared << '\n'
            << "Ascensions " << player.ascensions << " | Echo Rank " << player.echoRank
            << " | Ember Marks " << player.emberMarks << '\n'
            << "Dust " << player.arcaneDust << " | Boss Essence " << player.bossEssence
            << " | Echo Sigils " << player.echoSigils << " | Aether Shards "
            << player.aetherShards << "\n\n";
}

void printHelp() {
  std::cout
      << "Commands:\n"
      << "  help                         Show this help\n"
      << "  status                       Show player state\n"
      << "  mine [count]                 Mine blocks (default: 1)\n"
      << "  boss <1-10> <score> <hearts> Attempt a boss challenge\n"
      << "  dungeon <1-10> [seed]        Run a deterministic dungeon\n"
      << "  depth <number>               Inspect a Deep Strata block\n"
      << "  ascend                       Ascend and reset run progression\n"
      << "  quit                         Exit the playtest\n";
}

void mineBlocks(ed::GameState& game,
                const std::vector<ed::LayerDef>& layers,
                const std::vector<ed::MaterialDef>& materials,
                ed::Pcg32& rng,
                int count) {
  const int safeCount = std::clamp(count, 1, 100);
  for (int blockIndex = 0; blockIndex < safeCount; ++blockIndex) {
    const auto block =
        ed::generateBlockForLayer(layers, materials, game.player.currentLayer, 0, rng);
    auto progress = ed::createMiningProgress(block);
    const double miningPower = 3.0 + game.player.level * 0.15;
    const double damage = ed::miningDamage(miningPower, 0.0, false, ed::kCritDamageBase,
                                           progress.hardness, 0);
    const int hits = std::max(1, static_cast<int>(
                                     (progress.currentHp + static_cast<int>(damage) - 1) /
                                     static_cast<int>(damage)));
    progress.currentHp = 0;
    ++game.player.blocksBroken;
    game.player.clicks += hits;
    game.player.gold += block.sell * block.dropMultiplier;
    game.player.goldEarnedThisRun += block.sell * block.dropMultiplier;
    ed::applyXp(game, block.xp);
    std::cout << "Broke " << block.materialName << " [" << rarityName(block.rarity) << "]"
              << " in " << hits << " hits for " << block.xp << " XP and "
              << block.sell * block.dropMultiplier << " gold.\n";
  }
  game.dirty = true;
}

void attemptBoss(ed::GameState& game,
                 ed::CampaignState& campaign,
                 const std::vector<ed::BossDef>& bosses,
                 int index,
                 int score,
                 int heartsLost) {
  if (index < 1 || index > static_cast<int>(bosses.size())) {
    std::cout << "Boss must be between 1 and 10.\n";
    return;
  }
  const auto bossIndex = static_cast<std::size_t>(index - 1);
  if (!ed::isBossUnlocked(game, campaign, bosses, bossIndex)) {
    std::cout << "Boss is locked. Required level: " << bosses[bossIndex].minLevel
              << " and the previous boss must be cleared.\n";
    return;
  }
  const auto outcome =
      ed::evaluateBossAttempt(game, campaign, bosses, bossIndex, {score, heartsLost, true, 0.0});
  if (!outcome.cleared) {
    std::cout << "Defeat. Score did not reach the clear threshold.\n";
    return;
  }
  ed::applyBossOutcome(game, outcome);
  std::cout << "Cleared " << bosses[bossIndex].name << " with " << outcome.stars
            << " star(s), +" << outcome.goldReward << " gold, +" << outcome.xpReward
            << " XP";
  if (!outcome.legendaryId.empty()) {
    std::cout << ", legendary: " << outcome.legendaryId;
  }
  std::cout << ".\n";
}

void runDungeon(ed::GameState& game,
                const std::vector<ed::DungeonZoneDef>& zones,
                int index,
                std::uint64_t seed) {
  if (index < 1 || index > static_cast<int>(zones.size())) {
    std::cout << "Dungeon zone must be between 1 and 10.\n";
    return;
  }
  const auto& zone = zones[static_cast<std::size_t>(index - 1)];
  ed::DungeonPlayerStats stats;
  stats.maxHp = 100 + game.player.level * 3;
  stats.attack = 10 + game.player.level;
  stats.defense = game.player.level / 2;
  stats.speed = 10 + game.player.level / 3;
  const auto result = ed::simulateDungeonRun(stats, zone, seed);
  if (!result.cleared) {
    std::cout << "Dungeon failed after " << result.floorsCleared << " floor(s).\n";
    return;
  }
  game.player.gold += static_cast<int>(result.gold);
  game.player.goldEarnedThisRun += static_cast<int>(result.gold);
  ed::applyXp(game, result.xp);
  game.player.arcaneDust += result.arcaneDust;
  std::cout << "Dungeon cleared: +" << result.gold << " gold, +" << result.xp << " XP, +"
            << result.trophies << " trophies";
  if (result.gemFound) {
    std::cout << ", found a gem";
  }
  std::cout << ".\n";
}

void inspectDepth(const ed::GameState& game, int depth) {
  if (game.player.bossesCleared < 10) {
    std::cout << "Deep Strata requires all 10 bosses cleared.\n";
    return;
  }
  const auto block = ed::deepStrataBlock(depth);
  std::cout << "Depth " << block.depth << ": HP " << block.hp << ", XP " << block.xp
            << ", ore " << block.oreQuantity << ", Aether Shard chance "
            << std::fixed << std::setprecision(2)
            << (static_cast<double>(block.aetherShardChanceBasisPoints) / 100.0) << "%.\n";
}

} // namespace

int main() {
  ed::GameState game;
  const auto layers = ed::defaultLayers();
  const auto materials = ed::defaultMaterials();
  const auto bosses = ed::defaultBosses();
  const auto zones = ed::defaultDungeonZones();
  auto campaign = ed::createCampaignState(bosses.size());
  ed::Pcg32 rng(0xC0FFEEULL);

  std::cout << "Emberdeep terminal playtest\n";
  std::cout << "Type 'help' for commands. This is a deterministic core-system playtest.\n";

  std::string line;
  while (std::cout << "> " && std::getline(std::cin, line)) {
    std::istringstream input(line);
    std::string command;
    input >> command;
    if (command == "help") {
      printHelp();
    } else if (command == "status") {
      printStatus(game);
    } else if (command == "mine") {
      int count = 1;
      input >> count;
      mineBlocks(game, layers, materials, rng, count);
    } else if (command == "boss") {
      int index = 0;
      int score = 0;
      int heartsLost = 0;
      if (input >> index >> score >> heartsLost) {
        attemptBoss(game, campaign, bosses, index, score, heartsLost);
      } else {
        std::cout << "Usage: boss <1-10> <score> <hearts-lost>\n";
      }
    } else if (command == "dungeon") {
      int index = 0;
      std::uint64_t seed = 1;
      if (input >> index) {
        input >> seed;
        runDungeon(game, zones, index, seed);
      } else {
        std::cout << "Usage: dungeon <1-10> [seed]\n";
      }
    } else if (command == "depth") {
      int depth = 0;
      if (input >> depth) {
        inspectDepth(game, depth);
      } else {
        std::cout << "Usage: depth <number>\n";
      }
    } else if (command == "ascend") {
      if (game.player.bossesCleared < 10) {
        std::cout << "Ascension requires all 10 bosses cleared.\n";
      } else {
        const int marks = ed::applyAscension(game);
        std::cout << "Ascended for " << marks << " Ember Mark(s).\n";
      }
    } else if (command == "quit" || command == "exit") {
      break;
    } else if (!command.empty()) {
      std::cout << "Unknown command. Type 'help'.\n";
    }
  }

  std::cout << "Playtest ended.\n";
  return 0;
}
