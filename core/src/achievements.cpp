#include "ed/achievements.hpp"

#include <algorithm>

namespace ed {

const std::vector<AchievementDef>& allAchievements() {
  static const std::vector<AchievementDef> s_achievements = {
    // Mining
    {"first_strike", "First Strike", "Break your first block in the mines.", "Mining", "[*]"},
    {"apprentice_miner", "Apprentice Miner", "Break 100 blocks in the mines.", "Mining", "[*]"},
    {"deep_quarryman", "Deep Quarryman", "Break 1,000 blocks in the mines.", "Mining", "[*]"},
    {"master_excavator", "Master Excavator", "Break 10,000 blocks in the mines.", "Mining", "[*]"},
    {"precision_striker", "Precision Striker", "Hit your first mining weak spot.", "Mining", "(+)"},
    {"deadeye_miner", "Deadeye Miner", "Hit 25 mining weak spots.", "Mining", "(+)"},
    {"bullseye_master", "Bullseye Master", "Hit 100 mining weak spots.", "Mining", "(+)"},
    {"flawless_focus", "Flawless Focus", "Hit 250 mining weak spots.", "Mining", "(+)"},

    // Progression
    {"depth_explorer", "Depth Explorer", "Delve to Depth 5 or deeper.", "Progression", "[v]"},
    {"abyss_diver", "Abyss Diver", "Delve to Depth 10 or deeper.", "Progression", "[v]"},
    {"core_delver", "Core Delver", "Delve to Depth 15 or deeper.", "Progression", "[v]"},
    {"novice_adventurer", "Novice Adventurer", "Reach Player Level 10.", "Progression", "[^]"},
    {"seasoned_delver", "Seasoned Delver", "Reach Player Level 30.", "Progression", "[^]"},
    {"veteran_pioneer", "Veteran Pioneer", "Reach Player Level 60.", "Progression", "[^]"},
    {"mythic_hero", "Mythic Hero", "Reach Player Level 100.", "Progression", "[^]"},
    {"pocket_change", "Pocket Change", "Accumulate 1,000 Gold.", "Progression", "[$]"},
    {"gold_hoarder", "Gold Hoarder", "Accumulate 100,000 Gold.", "Progression", "[$]"},
    {"dragon_treasury", "Dragon's Treasury", "Accumulate 1,000,000 Gold.", "Progression", "[$]"},
    {"dust_collector", "Arcane Collector", "Gather 50 Arcane Dust.", "Progression", "[~]"},
    {"essence_harvester", "Essence Harvester", "Gather 20 Boss Essence.", "Progression", "[~]"},

    // Crafting & Gear
    {"blacksmith_hand", "Blacksmith's Hand", "Own at least 3 pieces of equipment.", "Crafting", "[#]"},
    {"full_arsenal", "Full Arsenal", "Own at least 6 pieces of equipment.", "Crafting", "[#]"},
    {"gemologist", "Gemologist", "Socket a gem into a piece of gear.", "Crafting", "<*>"},
    {"jeweled_knight", "Jeweled Knight", "Socket 3 or more gems across all gear.", "Crafting", "<*>"},
    {"mythic_ascendant", "Mythic Ascendant", "Own a piece of Tier 5 or Legendary gear.", "Crafting", "<!>"},

    // Combat
    {"first_boss_slain", "Boss Slayer", "Defeat your first campaign boss.", "Combat", "[X]"},
    {"tomb_raider", "Dungeon Crawler", "Clear Floor 5 of the Abyssal Spire.", "Combat", "[D]"},
    {"spire_conqueror", "Spire Conqueror", "Clear Floor 20 of the Abyssal Spire.", "Combat", "[D]"},
    {"perfect_strike", "Grand Maestro", "Achieve a 3-star rating on any boss minigame.", "Combat", "[*]"},
    {"campaign_victor", "Campaign Victor", "Defeat all 10 campaign bosses.", "Combat", "[W]"},

    // Endgame
    {"rebirth", "Rebirth", "Perform your first Ascension / Prestige.", "Endgame", "[A]"},
    {"eternal_flame", "Eternal Flame", "Ascend 5 or more times.", "Endgame", "[A]"},
    {"echo_slayer", "Echo Vanquisher", "Defeat an Echo Boss in the post-game.", "Endgame", "[E]"},
    {"hollow_seeker", "Hollow Seeker", "Complete a run in the Gem Hollow.", "Endgame", "[G]"},

    // Hollow Descent
    {"descent_initiate", "Hollow Initiate", "Brave the threshold and step into The Hollow Descent.", "Descent", "[D]"},
    {"descent_depth_5", "Descent Delver", "Conquer Chamber 5 in the Hollow Descent.", "Descent", "[5]"},
    {"descent_depth_10", "Abyssal March", "Conquer Chamber 10 in the Hollow Descent.", "Descent", "[10]"},
    {"descent_conqueror", "Crowned Conqueror", "Vanquish Grave Warden Osric and conquer the Hollow Descent.", "Descent", "[W]"},
    {"descent_vault_cracker", "Vault Breaker", "Crack the lock and claim the riches of a secret Treasure Chamber.", "Descent", "[$]"},
    {"descent_level_15", "Soul Ascendant", "Reach Run Level 15 within a single Descent expedition.", "Descent", "[^]"},
    {"endless_voyager", "Endless Delver", "Conquer Chamber 25 in Endless Catacombs.", "Descent", "[X]"}
  };
  return s_achievements;
}

bool isAchievementUnlocked(const PlayerState& player, const std::string& achievementId) {
  return std::find(player.achievements.begin(), player.achievements.end(), achievementId) != player.achievements.end();
}

std::vector<std::string> evaluateNewAchievements(const GameState& game,
                                                 const InventoryState& inventory,
                                                 const CampaignState& campaign) {
  const auto& p = game.player;
  std::vector<std::string> newlyUnlocked;

  auto check = [&](const std::string& id, bool condition) {
    if (condition && !isAchievementUnlocked(p, id)) {
      newlyUnlocked.push_back(id);
    }
  };

  // Mining
  check("first_strike", p.blocksBroken >= 1);
  check("apprentice_miner", p.blocksBroken >= 100);
  check("deep_quarryman", p.blocksBroken >= 1000);
  check("master_excavator", p.blocksBroken >= 10000);
  check("precision_striker", p.weakSpotsHit >= 1);
  check("deadeye_miner", p.weakSpotsHit >= 25);
  check("bullseye_master", p.weakSpotsHit >= 100);
  check("flawless_focus", p.weakSpotsHit >= 250);

  // Progression
  check("depth_explorer", p.deepestDepth >= 5);
  check("abyss_diver", p.deepestDepth >= 10);
  check("core_delver", p.deepestDepth >= 15);
  check("novice_adventurer", p.level >= 10);
  check("seasoned_delver", p.level >= 30);
  check("veteran_pioneer", p.level >= 60);
  check("mythic_hero", p.level >= 100);
  check("pocket_change", p.gold >= 1000);
  check("gold_hoarder", p.gold >= 100000);
  check("dragon_treasury", p.gold >= 1000000);
  check("dust_collector", p.arcaneDust >= 50);
  check("essence_harvester", p.bossEssence >= 20);

  // Crafting
  check("blacksmith_hand", inventory.gear.size() >= 3);
  check("full_arsenal", inventory.gear.size() >= 6);

  int totalSocketedGems = 0;
  bool hasAnySocketed = false;
  bool hasMythicOrLegendary = false;
  for (const auto& g : inventory.gear) {
    totalSocketedGems += static_cast<int>(g.socketedGems.size());
    if (!g.socketedGems.empty()) hasAnySocketed = true;
    if (g.tier >= 5 || g.legendary) hasMythicOrLegendary = true;
  }
  check("gemologist", hasAnySocketed);
  check("jeweled_knight", totalSocketedGems >= 3);
  check("mythic_ascendant", hasMythicOrLegendary);

  // Combat
  int clearedBossCount = 0;
  bool hasThreeStar = false;
  for (std::size_t i = 0; i < campaign.cleared.size(); ++i) {
    if (campaign.cleared[i]) clearedBossCount++;
    if (i < campaign.bestStars.size() && campaign.bestStars[i] >= 3) {
      hasThreeStar = true;
    }
  }
  check("first_boss_slain", p.bossesCleared >= 1 || clearedBossCount >= 1);
  check("tomb_raider", p.abyssBestFloor >= 5);
  check("spire_conqueror", p.abyssBestFloor >= 20);
  check("perfect_strike", hasThreeStar);
  check("campaign_victor", clearedBossCount >= 10);

  // Endgame
  check("rebirth", p.ascensions >= 1);
  check("eternal_flame", p.ascensions >= 5);

  bool echoClearedAny = p.echoRank >= 1;
  for (bool c : p.echoCleared) {
    if (c) { echoClearedAny = true; break; }
  }
  check("echo_slayer", echoClearedAny);
  check("hollow_seeker", p.gemHollowStreak >= 1 || p.gemHollowTodayGems > 0);

  // Hollow Descent
  check("descent_initiate", p.descentRuns >= 1);
  check("descent_depth_5", p.descentBestChamber >= 5);
  check("descent_depth_10", p.descentBestChamber >= 10);
  check("descent_conqueror", p.descentVictories >= 1);
  check("descent_vault_cracker", p.descentVaultsCracked >= 1);
  check("descent_level_15", p.descentMaxRunLevel >= 15);
  check("endless_voyager", p.descentEndlessBestChamber >= 25);

  return newlyUnlocked;
}

} // namespace ed
