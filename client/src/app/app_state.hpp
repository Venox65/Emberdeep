#pragma once

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <raylib.h>

#include "ed/achievements.hpp"
#include "ed/boss_minigames.hpp"
#include "ed/campaign.hpp"
#include "ed/crafting.hpp"
#include "ed/data_loader.hpp"
#include "ed/data_model.hpp"
#include "ed/dungeon.hpp"
#include "ed/formulas.hpp"
#include "ed/game_state.hpp"
#include "ed/inventory.hpp"
#include "ed/mining.hpp"
#include "ed/postgame.hpp"
#include "ed/save.hpp"
#include "ed/stats.hpp"
#include "ed/descent/combat_sim.hpp"

namespace ed::app {

constexpr int kWidth = 1280;
constexpr int kHeight = 720;

enum class Screen {
  Loading,
  Title,
  Mine,
  Inventory,
  Crafting,
  Campaign,
  Dungeon,
  Profile,
  GemHollow,
  Enchanting,
  Settings,
  DescentHub,
  DescentRun,
};

enum class ProfileTab {
  Vows,
  Perks,
  Achievements,
};

enum class DescentHubTab {
  Main,
  Anvil,
  Preparations,
  Codex,
  Pacts,
};

struct GameSettings {
  bool screenShake = true;
  int masterVolume = 100;
  int sfxVolume = 100;
  int musicVolume = 80;
  bool abbreviatedNumbers = true;
};

struct FloatingText {
  Vector2 position{};
  std::string text;
  float lifetime = 1.0F;
  Color color = WHITE;
};

struct Particle {
  Vector2 position{};
  Vector2 velocity{};
  float lifetime = 0.5F;
  Color color = WHITE;
};

struct SaveFileInfo {
  std::filesystem::path path;
  std::string filename;
  bool isActive = false;
  bool valid = false;
  int level = 1;
  int gold = 0;
  int bossesCleared = 0;
  int ascensions = 0;
  int deepestDepth = 0;
  int blocksBroken = 0;
};

struct EnemyUiState {
  int id = 0;
  std::string name;
  std::int64_t hp = 0;
  std::int64_t maxHp = 0;
};

struct AppState {
  bool sidebarOpen = false;
  ed::GameState game;
  ed::InventoryState inventory;
  ed::MiningProgress block;
  ed::BlockSpec blockSpec;
  ed::Pcg32 rng{0xC0FFEEULL};
  std::vector<ed::LayerDef> layers;
  std::vector<ed::MaterialDef> materials;
  std::vector<ed::BossDef> bosses;
  std::vector<ed::DungeonZoneDef> zones;
  std::vector<ed::GemDef> gems;
  std::vector<ed::CraftRecipe> recipes;
  ed::CampaignState campaign = ed::createCampaignState(10);
  std::filesystem::path savePath = "emberdeep.sav";
  std::chrono::steady_clock::time_point lastAutosave = std::chrono::steady_clock::now();
  std::mutex saveMutex;
  std::condition_variable saveCondition;
  std::optional<ed::SaveSnapshot> pendingSave;
  bool saveWorkerRunning = true;
  std::thread saveWorker;
  std::vector<FloatingText> floatingText;
  std::vector<Particle> particles;
  Screen screen = Screen::Loading;
  float loadingTimer = 0.0F;
  ProfileTab profileTab = ProfileTab::Vows;
  int achievementsScroll = 0;
  bool weakSpotActive = false;
  Vector2 weakSpotPos{640.0F, 335.0F};
  float weakSpotTimer = 0.0F;
  float weakSpotCooldown = 5.0F;
  float autoClickTimer = 0.0F;
  float activeMiningTimer = 0.0F;
  bool offlinePopupActive = false;
  ed::OfflineProgressResult offlineResult;
  float achievementCheckTimer = 0.0F;
  std::string achievementNotification;
  float achievementNotificationTimer = 0.0F;
  GameSettings settings;
  int layerIndex = 0;
  int selectedBoss = 0;
  int selectedZone = 0;
  int selectedRecipe = 0;
  int craftingScroll = 0;
  int selectedGear = 0;
  int inventoryScroll = 0;
  int enchantingScroll = 0;
  int campaignScore = 0;

  AppState() {
    auto data = ed::loadGameData("data");
    layers = std::move(data.layers);
    materials = std::move(data.materials);
    bosses = std::move(data.bosses);
    zones = std::move(data.zones);
    gems = std::move(data.gems);
    recipes = ed::defaultCraftRecipes(static_cast<int>(layers.size()));
    campaign = ed::createCampaignState(bosses.size());
  }
  int campaignHeartsLost = 0;
  bool campaignAttempt = false;
  bool campaignIntro = false;
  bool echoCampaignMode = false;
  bool abyssDungeonMode = false;
  ed::FrenzyDigState frenzyDig;
  ed::BoneDuelState boneDuel;
  ed::CinderDanceState cinderDance;
  ed::HeadHuntState headHunt;
  ed::ParryDuelState parryDuel;
  ed::RuneRecallState runeRecall;
  ed::StarTraceState starTrace;
  ed::HeartpierceState heartpierce;
  ed::SoulDodgeState soulDodge;
  ed::CrownStrikeState crownStrike;
  ed::FiveTrialsState fiveTrials;
  float blockPulse = 0.0F;
  float shake = 0.0F;
  std::string notice = "Click the block to mine.";
  float noticeLifetime = 4.0F;
  bool gemHollowShowSummary = false;
  std::vector<std::string> gemHollowSummaryItems;

  bool inDungeonCombat = false;
  bool dungeonShowResults = false;
  ed::DungeonRunResult dungeonResult;
  std::size_t dungeonLogIndex = 0;
  float dungeonCombatTimer = 0.0F;
  std::int64_t dungeonPlayerHp = 0;
  std::int64_t dungeonPlayerMaxHp = 0;
  std::vector<EnemyUiState> dungeonEnemies;
  std::vector<std::string> dungeonBattleLog;

  bool inBossCombat = false;
  bool bossCombatShowResults = false;
  ed::BossCombatResult bossCombatResult;
  std::size_t bossCombatLogIndex = 0;
  float bossCombatTimer = 0.0F;
  std::int64_t bossCombatPlayerHp = 0;
  std::int64_t bossCombatPlayerMaxHp = 0;
  std::int64_t bossCombatBossHp = 0;
  std::int64_t bossCombatBossMaxHp = 0;
  std::string bossCombatBossName;
  std::string bossCombatBossAbility;
  int bossCombatStars = 1;
  bool bossCombatIsEcho = false;
  int bossCombatEchoRank = 1;
  int bossCombatBossIndex = 0;
  std::vector<std::string> bossBattleLog;
  bool bossCombatRewardsClaimed = false;
  int bossCombatRewardGold = 0;
  int bossCombatRewardXp = 0;
  int bossCombatRewardSigils = 0;
  std::string bossCombatRewardLegendary;

  // Gem socket radial menu state
  bool gemSlotMenuOpen = false;
  bool gemSlotMenuJustOpened = false;
  int gemSlotMenuGearIndex = -1;
  int gemSlotMenuSocketIndex = -1;
  Vector2 gemSlotMenuPos{};
  int gemRadialFilterTier = 0;         // 0: All, 1: Rough, 2: Cut, 3: Flawless
  int gemRadialSelectedBaseIndex = -1; // -1: none, >= 0: base gem selected
  int gemRadialHoveredTier = -1;       // hovered tier

  // Gem refining workshop modal state
  bool gemRefineModalOpen = false;
  int gemRefineScroll = 0;

  bool ascensionConfirm = false;
  int deepStrataDepth = 0;
  bool deepStrataAutoAdvance = true;

  bool showSaveSelect = false;
  std::vector<SaveFileInfo> saveFilesList;
  int saveSelectScroll = 0;
  std::string newSaveInputName;
  bool newSaveInputFocused = false;

  // The Hollow Descent state
  ed::descent::DescentSimState descentSim;
  Camera2D descentCamera{};
  bool descentActive = false;
  float descentAccumulator = 0.0F;
  int descentCardHovered = -1;
  DescentHubTab descentHubTab = DescentHubTab::Main;
  ed::descent::DescentSave descentMeta;
  bool descentEndlessMode = false;
  bool descentRunSummaryOpen = false;
  bool descentPaused = false;

  // Hollow Descent Treasure Room & Minigame State
  bool descentMinigameActive = false;
  int descentMinigameBossIdx = 0;
  bool descentChestOpened = false;
  float descentPopupTimer = 0.0F;
  std::string descentPopupMsg;
  bool descentPopupSuccess = false;
};

inline ed::SaveSnapshot makeSnapshot(const AppState& app) {
  return {app.game, app.inventory, app.campaign};
}

inline void setNotice(AppState& app, std::string message) {
  app.notice = std::move(message);
  app.noticeLifetime = 4.0F;
}

inline const char* rarityName(ed::Rarity rarity) {
  switch (rarity) {
    case ed::Rarity::Common: return "COMMON";
    case ed::Rarity::Uncommon: return "UNCOMMON";
    case ed::Rarity::Rare: return "RARE";
    case ed::Rarity::Epic: return "EPIC";
  }
  return "UNKNOWN";
}

inline const char* materialIdForTier(int tier) {
  static constexpr const char* ids[] = {"ashen-stone", "copper", "iron", "silvervein",
                                        "aurelite", "runic-slate", "starmetal", "wyrmite",
                                        "voidglass", "aetherium"};
  return ids[std::clamp(tier, 1, 10) - 1];
}

inline const char* screenName(Screen screen) {
  switch (screen) {
    case Screen::Loading: return "LOADING";
    case Screen::Title: return "TITLE";
    case Screen::Mine: return "MINE";
    case Screen::Inventory: return "INVENTORY";
    case Screen::Crafting: return "CRAFTING";
    case Screen::Campaign: return "CAMPAIGN";
    case Screen::Dungeon: return "DUNGEON";
    case Screen::GemHollow: return "GEM HOLLOW";
    case Screen::Enchanting: return "ENCHANTING";
    case Screen::Profile: return "PROFILE";
    case Screen::Settings: return "SETTINGS";
    case Screen::DescentHub: return "THE HOLLOW DESCENT";
    case Screen::DescentRun: return "THE DESCENT";
  }
  return "";
}

inline Color materialColor(const ed::BlockSpec& block) {
  if (block.materialId == "copper") return Color{201, 119, 63, 255};
  if (block.materialId == "iron") return Color{123, 135, 148, 255};
  if (block.materialId == "silvervein") return Color{215, 226, 236, 255};
  if (block.materialId == "aurelite") return Color{242, 193, 78, 255};
  if (block.materialId == "runic-slate") return Color{90, 79, 207, 255};
  if (block.materialId == "starmetal") return Color{111, 211, 255, 255};
  if (block.materialId == "wyrmite") return Color{217, 79, 48, 255};
  if (block.materialId == "voidglass") return Color{80, 28, 100, 255};
  if (block.materialId == "aetherium") return Color{124, 247, 224, 255};
  return Color{138, 138, 148, 255};
}

inline void drawTextCentered(const std::string& text, int y, int size, Color color) {
  const int width = MeasureText(text.c_str(), size);
  DrawText(text.c_str(), (kWidth - width) / 2, y, size, color);
}

} // namespace ed::app
