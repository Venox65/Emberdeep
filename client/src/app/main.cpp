#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <filesystem>
#include <chrono>
#include <fstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <cctype>

#include <raylib.h>

#include "ed/campaign.hpp"
#include "ed/boss_minigames.hpp"
#include "ed/cloud.hpp"
#include "ed/crafting.hpp"
#include "ed/data_model.hpp"
#include "ed/dungeon.hpp"
#include "ed/formulas.hpp"
#include "ed/game_state.hpp"
#include "ed/mining.hpp"
#include "ed/inventory.hpp"
#include "ed/postgame.hpp"
#include "ed/save.hpp"
#include "ed/stats.hpp"

namespace {

constexpr int kWidth = 1280;
constexpr int kHeight = 720;

enum class Screen {
  Mine,
  Inventory,
  Crafting,
  Campaign,
  Dungeon,
  Profile,
  GemHollow,
  Enchanting,
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

struct AppState {
  bool sidebarOpen = false;
  ed::GameState game;
  ed::InventoryState inventory;
  ed::MiningProgress block;
  ed::BlockSpec blockSpec;
  ed::Pcg32 rng{0xC0FFEEULL};
  std::vector<ed::LayerDef> layers = ed::defaultLayers();
  std::vector<ed::MaterialDef> materials = ed::defaultMaterials();
  std::vector<ed::BossDef> bosses = ed::defaultBosses();
  std::vector<ed::DungeonZoneDef> zones = ed::defaultDungeonZones();
  std::vector<ed::CraftRecipe> recipes = ed::defaultCraftRecipes(10);
  ed::CampaignState campaign = ed::createCampaignState(10);
  std::filesystem::path savePath = "emberdeep.sav";
  std::filesystem::path profilePath = "emberdeep.profile";
  ed::AccountProfile account;
  bool needsAccountChoice = true;
  bool accountForm = false;
  bool authUsernameActive = true;
  bool authSubmitting = false;
  std::string authUsername;
  std::string authPassword;
  std::string backendHost = "127.0.0.1";
  int backendPort = 8080;
  std::chrono::steady_clock::time_point lastAutosave = std::chrono::steady_clock::now();
  std::mutex saveMutex;
  std::condition_variable saveCondition;
  std::optional<ed::SaveSnapshot> pendingSave;
  bool saveWorkerRunning = true;
  std::thread saveWorker;
  std::vector<FloatingText> floatingText;
  std::vector<Particle> particles;
  Screen screen = Screen::Mine;
  int layerIndex = 0;
  int selectedBoss = 0;
  int selectedZone = 0;
  int selectedRecipe = 0;
  int craftingScroll = 0;
  int selectedGear = 0;
  int inventoryScroll = 0;
  int campaignScore = 0;
  int campaignHeartsLost = 0;
  bool campaignAttempt = false;
  bool campaignIntro = false;
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
};

ed::SaveSnapshot makeSnapshot(const AppState& app) {
  return {app.game, app.inventory, app.campaign};
}

void setNotice(AppState& app, std::string message);
void spawnBlock(AppState& app);
void stopSaveWorker(AppState& app);
void loadSave(AppState& app);

std::string safeAccountId(const std::string& value) {
  std::string result;
  for (const unsigned char character : value) {
    if (std::isalnum(character) || character == '-' || character == '_') {
      result.push_back(static_cast<char>(character));
    } else {
      result.push_back('_');
    }
  }
  return result.empty() ? "guest" : result;
}

void configureAccountStorage(AppState& app) {
  const std::string accountId = app.account.mode == "account"
                                    ? app.account.username
                                    : "guest";
  app.savePath = "emberdeep-" + safeAccountId(accountId) + ".sav";
}

void requestSave(AppState& app) {
  {
    std::lock_guard lock(app.saveMutex);
    app.pendingSave = makeSnapshot(app);
  }
  app.saveCondition.notify_one();
  app.game.dirty = false;
}

void saveNow(AppState& app) {
  const auto snapshot = makeSnapshot(app);
  if (!ed::writeSaveAtomic(app.savePath, snapshot)) {
    setNotice(app, "Save failed: local save could not be written.");
    return;
  }
  if (app.account.mode == "account" && !app.account.token.empty()) {
    const auto result = ed::cloudSave(app.backendHost, app.backendPort, app.account.token, snapshot);
    if (!result.ok) {
      setNotice(app, "Local save stored; cloud sync failed: " + result.error);
      return;
    }
    setNotice(app, "Save stored locally and synced to the cloud.");
    app.game.dirty = false;
    return;
  }
  setNotice(app, "Local save stored.");
  app.game.dirty = false;
}

void startSaveWorker(AppState& app) {
  app.saveWorkerRunning = true;
  app.saveWorker = std::thread([&app] {
    while (true) {
      std::optional<ed::SaveSnapshot> snapshot;
      {
        std::unique_lock lock(app.saveMutex);
        app.saveCondition.wait(lock, [&] {
          return !app.saveWorkerRunning || app.pendingSave.has_value();
        });
        if (!app.saveWorkerRunning && !app.pendingSave.has_value()) return;
        snapshot = std::move(app.pendingSave);
        app.pendingSave.reset();
      }
      if (snapshot.has_value()) {
        ed::writeSaveAtomic(app.savePath, *snapshot);
        if (app.account.mode == "account" && !app.account.token.empty()) {
          const auto result = ed::cloudSave(app.backendHost, app.backendPort, app.account.token, *snapshot);
          if (!result.ok) app.notice = "Local save is safe; cloud sync failed.";
        }
      }
    }
  });
}

void loadCloudSave(AppState& app) {
      if (app.account.mode != "account" || app.account.token.empty()) return;
      const auto result = ed::cloudLoad(app.backendHost, app.backendPort, app.account.token);
      ed::SaveSnapshot snapshot;
      if (result.ok && ed::decodeCloudSnapshot(result.body, snapshot)) {
        app.game = snapshot.game;
        app.inventory = snapshot.inventory;
        app.campaign = snapshot.campaign;
        ed::writeSaveAtomic(app.savePath, snapshot);
        app.notice = "Cloud save loaded.";
      } else if (result.status == 404) {
        app.notice = "No cloud save yet; local progress will be uploaded.";
      } else if (!result.ok) {
        app.notice = "Backend unavailable; continuing with local save.";
      }
    }

void initializeAccount(AppState& app) {
      if (ed::readAccountProfile(app.profilePath, app.account)) {
        app.needsAccountChoice = false;
        configureAccountStorage(app);
        return;
      }
      configureAccountStorage(app);
      app.needsAccountChoice = true;
    }

void drawAccountPrompt(const AppState& app) {
      DrawText("EMBERDEEP", 475, 105, 42, Color{224, 187, 106, 255});
      DrawText("Choose how to keep your progress", 410, 170, 22, Color{220, 214, 190, 255});
      DrawRectangleRounded({245, 245, 350, 150}, 0.08F, 8, Color{52, 52, 67, 255});
      DrawRectangleRounded({685, 245, 350, 150}, 0.08F, 8, Color{52, 52, 67, 255});
      DrawText("PLAY AS GUEST", 315, 280, 25, Color{129, 214, 241, 255});
      DrawText("Local save only", 330, 325, 18, RAYWHITE);
      DrawText("Press G", 385, 355, 18, Color{174, 174, 187, 255});
      DrawText("SIGN IN / CREATE", 730, 280, 25, Color{224, 187, 106, 255});
      DrawText("Cloud + local save", 765, 325, 18, RAYWHITE);
      DrawText("Press S", 825, 355, 18, Color{174, 174, 187, 255});
    }

void drawAuthForm(const AppState& app) {
      DrawText("SIGN IN OR CREATE ACCOUNT", 350, 105, 32, Color{224, 187, 106, 255});
      DrawText("Existing usernames sign in; new usernames are created automatically.", 270, 160, 17,
               Color{174, 174, 187, 255});
      DrawText("Username (3+ characters)", 300, 235, 19, Color{220, 214, 190, 255});
      DrawRectangleLines(300, 265, 680, 48, app.authUsernameActive ? Color{224, 187, 106, 255} : GRAY);
      DrawText(app.authUsername.c_str(), 315, 279, 20, RAYWHITE);
      DrawText("Password (8+ characters)", 300, 345, 19, Color{220, 214, 190, 255});
      DrawRectangleLines(300, 375, 680, 48, !app.authUsernameActive ? Color{224, 187, 106, 255} : GRAY);
      DrawText(std::string(app.authPassword.size(), '*').c_str(), 315, 389, 20, RAYWHITE);
      DrawText(app.authSubmitting ? "Contacting backend..." :
                                  "TAB switches field   ENTER submits   ESC returns",
               300, 485, 17,
               Color{174, 174, 187, 255});
    }

void handleAccountInput(AppState& app) {
      if (app.needsAccountChoice) {
        if (IsKeyPressed(KEY_G)) {
          app.account = {"guest", {}, {}};
          configureAccountStorage(app);
          ed::writeAccountProfile(app.profilePath, app.account);
          app.needsAccountChoice = false;
          app.notice = "Guest mode enabled.";
        }
        if (IsKeyPressed(KEY_S)) {
          app.needsAccountChoice = false;
          app.accountForm = true;
          app.authUsername.clear();
          app.authPassword.clear();
        }
        return;
      }
      if (app.account.mode == "account") return;
      if (IsKeyPressed(KEY_ESCAPE)) {
        app.needsAccountChoice = true;
        app.accountForm = false;
        app.authUsername.clear();
        app.authPassword.clear();
        return;
      }
      if (app.authSubmitting) return;
      if (IsKeyPressed(KEY_TAB)) app.authUsernameActive = !app.authUsernameActive;
      int character = GetCharPressed();
      while (character > 0) {
        if (character >= 32 && character <= 126) {
          auto& field = app.authUsernameActive ? app.authUsername : app.authPassword;
          if (field.size() < 64) field.push_back(static_cast<char>(character));
        }
        character = GetCharPressed();
      }
      if (IsKeyPressed(KEY_BACKSPACE)) {
        auto& field = app.authUsernameActive ? app.authUsername : app.authPassword;
        if (!field.empty()) field.pop_back();
      }
      if (IsKeyPressed(KEY_ENTER)) {
        if (app.authUsername.size() < 3 || app.authPassword.size() < 8) {
          app.notice = "Username needs 3+ characters; password needs 8+ characters.";
          app.noticeLifetime = 5.0F;
          return;
        }
        app.authSubmitting = true;
        auto result = ed::cloudLogin(app.backendHost, app.backendPort, app.authUsername, app.authPassword);
        if (!result.ok && result.status == 401) {
          const auto registration =
              ed::cloudRegister(app.backendHost, app.backendPort, app.authUsername, app.authPassword);
          if (registration.ok) result = registration;
        }
        const auto token = ed::cloudResponseField(result, "token");
        if (!token.empty()) {
          app.account = {"account", app.authUsername, token};
          if (ed::writeAccountProfile(app.profilePath, app.account)) {
            configureAccountStorage(app);
            loadSave(app);
            if (app.account.mode == "account") {
              loadCloudSave(app);
            }
            app.needsAccountChoice = false;
            app.accountForm = false;
            app.authSubmitting = false;
            app.notice = "Signed in. Main game loaded.";
            app.noticeLifetime = 5.0F;
          } else {
            app.authSubmitting = false;
            app.notice = "Signed in, but the account profile could not be saved.";
            app.noticeLifetime = 5.0F;
          }
        } else {
          app.authSubmitting = false;
          app.notice = result.error.empty() ? "Sign-in failed. Check your credentials." : result.error;
          app.noticeLifetime = 5.0F;
        }
      }
}

void stopSaveWorker(AppState& app) {
  requestSave(app);
  {
    std::lock_guard lock(app.saveMutex);
    app.saveWorkerRunning = false;
  }
  app.saveCondition.notify_one();
  if (app.saveWorker.joinable()) app.saveWorker.join();
}

void loadSave(AppState& app) {
  ed::SaveSnapshot snapshot;
  if (ed::readSave(app.savePath, snapshot)) {
    app.game = snapshot.game;
    app.inventory = snapshot.inventory;
    app.campaign = snapshot.campaign;
  }
}

Color materialColor(const ed::BlockSpec& block) {
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

const char* rarityName(ed::Rarity rarity) {
  switch (rarity) {
    case ed::Rarity::Common: return "COMMON";
    case ed::Rarity::Uncommon: return "UNCOMMON";
    case ed::Rarity::Rare: return "RARE";
    case ed::Rarity::Epic: return "EPIC";
  }
  return "UNKNOWN";
}

const char* materialIdForTier(int tier) {
  static constexpr const char* ids[] = {"ashen-stone", "copper", "iron", "silvervein",
                                        "aurelite", "runic-slate", "starmetal", "wyrmite",
                                        "voidglass", "aetherium"};
  return ids[std::clamp(tier, 1, 10) - 1];
}

const char* screenName(Screen screen) {
  switch (screen) {
    case Screen::Mine: return "MINE";
    case Screen::Inventory: return "INVENTORY";
    case Screen::Crafting: return "CRAFTING";
    case Screen::Campaign: return "CAMPAIGN";
    case Screen::Dungeon: return "DUNGEON";
    case Screen::GemHollow: return "GEM HOLLOW";
    case Screen::Enchanting: return "ENCHANTING";
    case Screen::Profile: return "PROFILE";
  }
  return "";
}

void setNotice(AppState& app, std::string message) {
  app.notice = std::move(message);
  app.noticeLifetime = 4.0F;
}

void spawnBlock(AppState& app) {
  app.blockSpec = ed::generateBlockForLayer(
      app.layers, app.materials, app.layers[static_cast<std::size_t>(app.layerIndex)].id, 0, app.rng);
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

void resetToAccountChoice(AppState& app) {
  app.game = ed::GameState{};
  app.inventory = ed::InventoryState{};
  app.campaign = ed::createCampaignState(app.bosses.size());
  app.account = ed::AccountProfile{};
  app.needsAccountChoice = true;
  app.accountForm = false;
  app.authSubmitting = false;
  app.authUsername.clear();
  app.authPassword.clear();
  app.screen = Screen::Mine;
  app.layerIndex = 0;
  configureAccountStorage(app);
  spawnBlock(app);
}

void signOut(AppState& app) {
  stopSaveWorker(app);
  std::error_code error;
  std::filesystem::remove(app.profilePath, error);
  resetToAccountChoice(app);
  startSaveWorker(app);
  setNotice(app, "Signed out. Local account saves were kept.");
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
  const int damage = static_cast<int>(std::max(
      1.0, ed::miningDamage(power, stats.miningPowerBonusPercent, false,
                            static_cast<double>(stats.critDamageBasisPoints) / 10000.0,
                            app.block.hardness, stats.pierce)));
  app.block.currentHp = std::max(0, app.block.currentHp - damage);
  ++app.game.player.clicks;
  app.blockPulse = 0.16F;
  spawnHitEffects(app, clickPosition, materialColor(app.blockSpec), damage);

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
  setNotice(app, "Block shattered! +" + std::to_string(app.block.xp) + " XP, +" +
                    std::to_string(gold) + " gold.");
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
    app.cinderDance = ed::startCinderDance(0xF17EULL, 30.0F);
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
  if (!result.cleared) {
    setNotice(app, "Dungeon failed on floor " + std::to_string(result.floorsCleared + 1) + ".");
    return;
  }
  app.game.player.gold += static_cast<int>(result.gold);
  app.game.player.goldEarnedThisRun += static_cast<int>(result.gold);
  ed::applyXp(app.game, result.xp);
  app.game.player.arcaneDust += result.arcaneDust;
  ed::addItemToInventory(app.inventory, "trophies",
                         "zone-" + std::to_string(zone.tier), result.trophies);
  setNotice(app, "Dungeon clear! +" + std::to_string(result.gold) + " gold, +" +
                    std::to_string(result.xp) + " XP.");
  app.game.dirty = true;
}

void drawTextCentered(const std::string& text, int y, int size, Color color) {
  DrawText(text.c_str(), (kWidth - MeasureText(text.c_str(), size)) / 2, y, size, color);
}

void drawTopBar(const AppState& app) {
  DrawRectangle(0, 0, kWidth, 78, Color{28, 27, 39, 255});
  DrawLine(0, 78, kWidth, 78, Color{93, 78, 55, 255});
  DrawText(("Lv " + std::to_string(app.game.player.level)).c_str(), 26, 19, 28, RAYWHITE);
  const float xpRatio = static_cast<float>(app.game.player.xp) /
                        static_cast<float>(std::max(1, ed::xpToNextLevel(app.game.player.level)));
  DrawRectangle(108, 26, 260, 20, Color{54, 52, 68, 255});
  DrawRectangle(108, 26, static_cast<int>(260.0F * std::clamp(xpRatio, 0.0F, 1.0F)), 20,
                Color{205, 154, 61, 255});
  DrawText(("XP " + std::to_string(app.game.player.xp)).c_str(), 118, 28, 16, RAYWHITE);
  DrawText(("Gold " + std::to_string(app.game.player.gold)).c_str(), 415, 25, 22,
           Color{248, 211, 89, 255});
  DrawText(("Blocks " + std::to_string(app.game.player.blocksBroken)).c_str(), 600, 28, 18,
           Color{200, 201, 211, 255});
  DrawText(("Essence " + std::to_string(app.game.player.bossEssence)).c_str(), 760, 28, 18,
           Color{232, 145, 236, 255});
  DrawText(("Ember " + std::to_string(app.game.player.emberMarks)).c_str(), 915, 28, 18,
           Color{255, 139, 65, 255});
  DrawText("1 Mine   2 Inventory   3 Crafting   4 Campaign   5 Dungeon   6 Gem Hollow   7 Enchant   8 Profile   F5 Save", 16, 55, 14,
           Color{156, 156, 173, 255});
  DrawRectangleRounded({1010, 14, 72, 34}, 0.2F, 8, Color{52, 76, 67, 255});
  DrawText("SAVE", 1026, 24, 15, Color{210, 255, 220, 255});
  DrawRectangleRounded({1090, 14, 160, 34}, 0.2F, 8, Color{82, 52, 53, 255});
  DrawText("SIGN OUT", 1120, 24, 15, Color{255, 210, 210, 255});
}

void drawNavigation(const AppState& app) {
  if (app.sidebarOpen) {
    DrawRectangle(0, 0, 200, 720, Color{28, 27, 39, 240});
    DrawText("MENU", 20, 20, 20, RAYWHITE);
    // Draw close button
    DrawRectangleRounded({150, 15, 30, 30}, 0.2F, 4, Color{200, 50, 50, 255});
    DrawText("X", 158, 20, 20, RAYWHITE);
    
    const Screen navScreens[] = {Screen::Mine, Screen::Inventory, Screen::Crafting, Screen::Campaign, Screen::Dungeon, Screen::GemHollow, Screen::Enchanting, Screen::Profile};
    for (int i = 0; i < 8; ++i) {
      const int y = 80 + i * 50;
      const Screen screen = navScreens[i];
      const bool active = app.screen == screen;
      DrawRectangleRounded({10, static_cast<float>(y), 180, 40}, 0.2F, 4, active ? Color{112, 76, 45, 255} : Color{48, 47, 61, 255});
      DrawText(screenName(screen), 20, y + 12, 16, active ? RAYWHITE : Color{170, 170, 180, 255});
    }
  } else {
    // Hamburger menu button on the right, below the sign out button
    DrawRectangleRounded({1220, 100, 40, 40}, 0.2F, 4, Color{48, 47, 61, 200});
    DrawRectangle(1228, 110, 24, 4, RAYWHITE);
    DrawRectangle(1228, 118, 24, 4, RAYWHITE);
    DrawRectangle(1228, 126, 24, 4, RAYWHITE);
  }
}

#include <map>

std::map<std::string, Texture2D> g_textures;
Texture2D getTexture(const std::string& path) {
    if (g_textures.find(path) == g_textures.end()) {
        g_textures[path] = LoadTexture(path.c_str());
    }
    return g_textures[path];
}

void drawMineScreen(const AppState& app) {
  drawTextCentered("THE EMBERDEEP", 110, 34, Color{224, 187, 106, 255});
  drawTextCentered("Layer: " + app.layers[static_cast<std::size_t>(app.layerIndex)].name, 153, 20,
                   Color{190, 190, 205, 255});
  DrawText("Q / E change layer", 28, 116, 16, Color{140, 140, 153, 255});

  const float hpRatio = static_cast<float>(app.block.currentHp) /
                        static_cast<float>(std::max(1, app.block.maxHp));
  DrawRectangle(320, 205, 640, 26, Color{52, 46, 56, 255});
  DrawRectangle(320, 205, static_cast<int>(640.0F * hpRatio), 26, Color{190, 75, 66, 255});
  DrawText((std::to_string(app.block.currentHp) + " / " + std::to_string(app.block.maxHp)).c_str(),
           565, 209, 17, RAYWHITE);

  const float pulse = 1.0F + app.blockPulse * 0.45F;
  const int blockSize = static_cast<int>(220.0F * pulse);
  const int blockX = 640 - blockSize / 2;
  const int blockY = 305 - blockSize / 2;
  const Color blockColor = materialColor(app.blockSpec);
  std::string baseName = app.blockSpec.materialId;
  std::replace(baseName.begin(), baseName.end(), '-', '_');
  std::string texName = "sprites/Mining_Block/" + baseName + (hpRatio <= 0.50F ? "_break.png" : "_idle.png");
  Texture2D tex = getTexture(texName);
  
  if (tex.id != 0) {
      int frames = tex.width / 32;
      if (frames < 1) frames = 1;
      int frame = (static_cast<int>(GetTime() * 60) / 15) % frames;
      Rectangle source = {static_cast<float>(frame * 32), 0.0f, 32.0f, 32.0f};
      DrawTexturePro(tex, source, 
                     {static_cast<float>(blockX), static_cast<float>(blockY), static_cast<float>(blockSize), static_cast<float>(blockSize)}, 
                     {0, 0}, 0.0f, WHITE);
  } else {
      DrawRectangleRounded({static_cast<float>(blockX), static_cast<float>(blockY),
                            static_cast<float>(blockSize), static_cast<float>(blockSize)},
                           0.14F, 12, blockColor);
      DrawRectangleLinesEx({static_cast<float>(blockX), static_cast<float>(blockY),
                            static_cast<float>(blockSize), static_cast<float>(blockSize)},
                           5.0F, Color{239, 225, 189, 255});
  }
  DrawCircle(590, 302, 13, Color{255, 255, 255, 130});
  DrawCircle(690, 352, 9, Color{255, 255, 255, 100});
  DrawText(app.blockSpec.materialName.c_str(), blockX + 18, blockY + blockSize + 18, 23, RAYWHITE);
  DrawText(rarityName(app.blockSpec.rarity), blockX + 18, blockY + blockSize + 48, 16,
           Color{255, 210, 106, 255});
  DrawText(("Hardness " + std::to_string(app.block.hardness) + "   XP " +
            std::to_string(app.block.xp))
               .c_str(),
           445, 520, 18, Color{179, 179, 193, 255});
  DrawText("Click the block or press SPACE to mine", 440, 572, 20, Color{235, 232, 213, 255});
}

void drawCampaignScreen(const AppState& app) {
  const Color caveTop{18, 21, 38, 255};
  const Color caveBottom{42, 27, 42, 255};
  DrawRectangleGradientV(0, 0, 1280, 720, caveTop, caveBottom);
  DrawCircle(170, 185, 145, Color{51, 42, 67, 180});
  DrawCircle(1110, 205, 190, Color{55, 39, 59, 180});
  DrawTriangle({0, 0}, {170, 0}, {0, 255}, Color{10, 13, 26, 230});
  DrawTriangle({1280, 0}, {1110, 0}, {1280, 285}, Color{10, 13, 26, 230});
  for (int x = 70; x < 1280; x += 170) {
    DrawTriangle({static_cast<float>(x), 0},
                 {static_cast<float>(x + 34), 0},
                 {static_cast<float>(x + 17), 95},
                 Color{11, 14, 27, 255});
  }
  DrawRectangle(0, 610, 1280, 110, Color{22, 18, 27, 220});
  DrawCircle(180, 600, 70, Color{31, 24, 31, 255});
  DrawCircle(1080, 605, 90, Color{31, 24, 31, 255});
  drawTextCentered("CAMPAIGN", 108, 34, Color{224, 187, 106, 255});
  DrawText("A / D select. ENTER starts. Space/click plays. BACKSPACE gives up.", 30, 155, 17,
           Color{174, 174, 187, 255});
  const auto& boss = app.bosses[static_cast<std::size_t>(app.selectedBoss)];
  const bool unlocked = ed::isBossUnlocked(app.game, app.campaign, app.bosses,
                                           static_cast<std::size_t>(app.selectedBoss));
  DrawRectangleRounded({260, 215, 760, 280}, 0.08F, 10,
                       unlocked ? Color{61, 55, 70, 255} : Color{43, 43, 53, 255});
  DrawText(boss.name.c_str(), 300, 250, 27, unlocked ? RAYWHITE : Color{120, 120, 130, 255});
  DrawText(("Required level: " + std::to_string(boss.minLevel)).c_str(), 300, 298, 18,
           Color{182, 182, 194, 255});
  DrawText(("Clear score: " + std::to_string(boss.clearScore) + "   2*: " +
            std::to_string(boss.twoStarScore) + "   3*: " + std::to_string(boss.threeStarScore))
               .c_str(),
           300, 330, 18, Color{204, 175, 111, 255});
  DrawText(("Best stars: " +
            std::to_string(app.campaign.bestStars[static_cast<std::size_t>(app.selectedBoss)]))
               .c_str(),
           300, 370, 18, Color{255, 218, 100, 255});
  if (app.campaignAttempt) {
    DrawRectangle(0, 145, 1280, 520, Color{13, 17, 31, 255});
    DrawCircle(140, 280, 190, Color{35, 49, 71, 255});
    DrawCircle(1140, 300, 230, Color{48, 35, 61, 255});
    for (int x = 40; x < 1280; x += 150) {
      DrawTriangle({static_cast<float>(x), 145},
                   {static_cast<float>(x + 30), 145},
                   {static_cast<float>(x + 15), 230},
                   Color{8, 12, 24, 255});
    }
    drawTextCentered(boss.name + " — BATTLE ARENA", 170, 26,
                     Color{255, 224, 145, 255});
  }
  if (app.campaignIntro) {
    DrawRectangle(210, 165, 860, 390, Color{12, 13, 25, 235});
    DrawRectangleLinesEx({210, 165, 860, 390}, 3.0F, Color{224, 187, 106, 255});
    drawTextCentered(boss.name, 245, 40, Color{255, 224, 145, 255});
    drawTextCentered(boss.challenge == ed::BossChallenge::FrenzyDig
                             ? "GRUBNOK'S FRENZY DIG"
                                 : boss.challenge == ed::BossChallenge::BoneDuel
                                       ? "SIR RATTLEBONE'S BONE DUEL"
                                       : boss.challenge == ed::BossChallenge::CinderDance
                                             ? "MORWENNA'S CINDER DANCE"
                                             : boss.challenge == ed::BossChallenge::HeadHunt
                                                   ? "THESSALY'S HEAD HUNT"
                                                   : boss.challenge == ed::BossChallenge::ParryDuel
                                                         ? "ALDRIC'S PARRY DUEL"
                                                         : boss.challenge == ed::BossChallenge::RuneRecall
                                                               ? "ISOLDE'S RUNE RECALL"
                                                               : boss.challenge == ed::BossChallenge::StarTrace
                                                                     ? "SERAPHEL'S STAR TRACE"
                                                                           : boss.challenge == ed::BossChallenge::Heartpierce
                                                                                 ? "IGNATHIA'S HEARTPIERCE"
                                                                                 : boss.challenge == ed::BossChallenge::SoulDodge
                                                                                       ? "XAR'MAL'S SOUL DODGE"
                                                                                       : "THE HOLLOW KING'S FIVE TRIALS",
                             305, 24, Color{199, 199, 218, 255});
    drawTextCentered(boss.challenge == ed::BossChallenge::FrenzyDig
                             ? "Mine the golden throne before the frenzy ends."
                             : boss.challenge == ed::BossChallenge::BoneDuel
                                   ? "Parry the incoming marker inside the green zone."
                                   : boss.challenge == ed::BossChallenge::CinderDance
                                         ? "Keep the ember rhythm alive across four lanes."
                                         : boss.challenge == ed::BossChallenge::HeadHunt
                                               ? "Strike real hydra heads and ignore the blue decoys."
                                               : boss.challenge == ed::BossChallenge::ParryDuel
                                                     ? "Match each attack direction before it lands."
                                                     : boss.challenge == ed::BossChallenge::RuneRecall
                                                           ? "Memorize each rune sequence, then repeat it."
                                                           : boss.challenge == ed::BossChallenge::StarTrace
                                                                 ? "Touch the numbered stars in order."
                                                                 : boss.challenge == ed::BossChallenge::Heartpierce
                                                                       ? "Track the heart with the lance."
                                                                       : boss.challenge == ed::BossChallenge::SoulDodge
                                                                             ? "Survive the deterministic bullet patterns."
                                                                             : "Complete all five trials with shared hearts.",
                         375, 20, Color{227, 222, 205, 255});
    drawTextCentered("Press ENTER when ready", 470, 22, Color{122, 217, 255, 255});
    return;
  }
  if (!app.campaignAttempt) {
    DrawText("Press ENTER to view the boss title card", 300, 452, 18,
                 Color{178, 178, 190, 255});
    return;
  }
  const auto& playerStats = ed::calculatePlayerStats(app.game, app.inventory);
  if (boss.challenge == ed::BossChallenge::FrenzyDig) {
    DrawCircle(640, 350, 105, Color{105, 73, 48, 255});
    DrawCircleLines(640, 350, 105, Color{235, 194, 109, 255});
    if (app.frenzyDig.nuggetVisible) {
      DrawCircle(640.0F + (app.frenzyDig.nuggetX - 0.5F) * 700.0F,
                 350.0F + (app.frenzyDig.nuggetY - 0.5F) * 260.0F, 18,
                 Color{255, 221, 72, 255});
    }
    DrawText(("Taps: " + std::to_string(app.frenzyDig.taps) + "   Time: " +
              std::to_string(static_cast<int>(10.0F - app.frenzyDig.elapsed)) + "s")
                 .c_str(),
             410, 505, 22, RAYWHITE);
    DrawText("Golden nugget gives +5 taps", 475, 545, 18, Color{255, 221, 72, 255});
  } else if (boss.challenge == ed::BossChallenge::BoneDuel) {
    DrawRectangle(360, 325, 560, 18, Color{57, 55, 70, 255});
    const float zone = ed::boneDuelZoneWidth(
        app.boneDuel, static_cast<double>(playerStats.focusBasisPoints) / 10000.0);
    DrawRectangle(640 - static_cast<int>(280 * zone), 325,
                  static_cast<int>(560 * zone), 18, Color{92, 208, 117, 255});
    DrawRectangle(static_cast<int>(360 + 560 * app.boneDuel.needle) - 5, 300, 10, 68,
                  Color{235, 235, 245, 255});
    DrawLine(640, 292, 640, 375, Color{255, 221, 72, 255});
    DrawText(("Round " + std::to_string(app.boneDuel.round) + "/8   Misses: " +
              std::to_string(app.boneDuel.misses) + "   Perfect: " +
              std::to_string(app.boneDuel.perfectHits))
                 .c_str(),
             425, 430, 21, RAYWHITE);
    DrawText("Click or press Space to parry inside the green zone", 385, 475, 18,
             Color{174, 235, 187, 255});
  } else if (boss.challenge == ed::BossChallenge::CinderDance) {
    DrawText(("Groove: " + std::to_string(app.cinderDance.groove) +
              "   Perfect: " + std::to_string(app.cinderDance.perfectHits) +
              "   Good: " + std::to_string(app.cinderDance.goodHits) +
              "   Misses: " + std::to_string(app.cinderDance.misses))
                 .c_str(),
             390, 255, 21, RAYWHITE);
    for (int lane = 0; lane < 4; ++lane) {
      const int x = 350 + lane * 150;
      DrawRectangle(x, 300, 110, 250, Color{38, 34, 53, 255});
      DrawRectangle(x, 505, 110, 12, Color{255, 197, 80, 255});
      const char* labels[] = {"D", "F", "J", "K"};
      DrawText(labels[lane], x + 43, 535, 24, Color{255, 230, 160, 255});
    }
    for (const auto& note : app.cinderDance.chart) {
      const float distance = note.time - app.cinderDance.elapsed;
      if (!note.judged && distance >= -0.1F && distance <= 2.0F) {
        const int x = 350 + note.lane * 150;
        const int y = 505 - static_cast<int>(distance * 100.0F);
        DrawCircle(x + 55, y, 14, Color{255, 118, 91, 255});
      }
    }
    DrawText("Hit the lane when its note reaches the gold line", 405, 580, 18,
             Color{255, 221, 150, 255});
  } else if (boss.challenge == ed::BossChallenge::HeadHunt) {
    DrawText(("Real heads: " + std::to_string(app.headHunt.realHits) +
              "   Decoys: " + std::to_string(app.headHunt.decoyHits) +
              "   Hearts lost: " + std::to_string(app.headHunt.heartsLost))
                 .c_str(),
             380, 255, 21, RAYWHITE);
    for (int cell = 0; cell < 9; ++cell) {
      const int x = 430 + (cell % 3) * 140;
      const int y = 290 + (cell / 3) * 95;
      DrawCircle(x + 45, y + 35, 32, Color{45, 38, 48, 255});
      const auto target = std::find_if(
          app.headHunt.targets.begin(), app.headHunt.targets.end(),
          [cell](const ed::HeadHuntTarget& item) { return item.cell == cell; });
      if (target != app.headHunt.targets.end()) {
        DrawCircle(x + 45, y + 35, 23,
                   target->real ? Color{79, 194, 108, 255} : Color{74, 157, 220, 255});
        DrawCircle(x + 38, y + 29, 4, Color{240, 238, 174, 255});
        DrawCircle(x + 52, y + 29, 4, Color{240, 238, 174, 255});
      }
      DrawText(std::to_string(cell + 1).c_str(), x + 40, y + 70, 16,
               Color{180, 180, 194, 255});
    }
    DrawText("Click a cell or press 1-9 to strike", 455, 585, 18,
             Color{174, 235, 187, 255});
  } else if (boss.challenge == ed::BossChallenge::ParryDuel) {
    DrawText(("Attack " + std::to_string(app.parryDuel.attack) + "/24   Parries: " +
              std::to_string(app.parryDuel.successfulParries) + "   Hearts lost: " +
              std::to_string(app.parryDuel.heartsLost))
                 .c_str(),
             390, 255, 21, RAYWHITE);
    const char* directions[] = {"LEFT", "DOWN", "RIGHT"};
    const std::string telegraph =
        app.parryDuel.feint ? "FEINT — do not press a direction"
                            : "INCOMING: " + std::string(directions[app.parryDuel.direction]);
    DrawText(telegraph.c_str(), 430, 330, 22,
             app.parryDuel.feint ? Color{255, 139, 102, 255}
                                 : Color{255, 224, 145, 255});
    const float reactionWindow =
        std::max(0.35F, 0.6F - 0.25F * app.parryDuel.attack / 23.0F);
    DrawRectangle(430, 365, 420, 10, Color{49, 45, 62, 255});
    DrawRectangle(430, 365,
                  static_cast<int>(420.0F *
                                   std::clamp(1.0F - app.parryDuel.elapsed / reactionWindow,
                                              0.0F, 1.0F)),
                  10, Color{255, 139, 102, 255});
    const char* labels[] = {"LEFT", "DOWN", "RIGHT"};
    for (int direction = 0; direction < 3; ++direction) {
      const int x = 330 + direction * 220;
      DrawRectangleRounded({static_cast<float>(x), 390, 170, 70}, 0.15F, 8,
                           direction == app.parryDuel.direction && !app.parryDuel.feint
                               ? Color{108, 78, 48, 255}
                               : Color{67, 62, 82, 255});
      DrawText(labels[direction], x + 42, 414, 18, Color{255, 224, 145, 255});
    }
    DrawText("Arrow keys or click a direction", 475, 520, 18,
             Color{190, 190, 205, 255});
  } else if (boss.challenge == ed::BossChallenge::RuneRecall) {
    DrawText(("Round " + std::to_string(app.runeRecall.round) + "/10   Mistakes: " +
              std::to_string(app.runeRecall.mistakes))
                 .c_str(),
             465, 255, 21, RAYWHITE);
    DrawText(app.runeRecall.showing ? "MEMORIZE THIS SEQUENCE"
                                    : "Repeat the rune sequence with keys 1-6",
             430, 310, 20,
             app.runeRecall.showing ? Color{255, 224, 145, 255}
                                    : Color{218, 211, 190, 255});
    if (app.runeRecall.showing) {
      std::string sequence;
      for (const int rune : app.runeRecall.sequence) {
        if (!sequence.empty()) sequence += "  ";
        sequence += std::to_string(rune + 1);
      }
      DrawText(sequence.c_str(), 500, 345, 28, Color{255, 238, 180, 255});
    } else {
      DrawText(("Progress: " + std::to_string(app.runeRecall.inputIndex) + "/" +
                std::to_string(app.runeRecall.sequence.size()))
                   .c_str(),
               500, 345, 20, Color{174, 235, 187, 255});
    }
    for (int rune = 0; rune < 6; ++rune) {
      const int x = 285 + (rune % 3) * 245;
      const int y = 370 + (rune / 3) * 100;
      DrawRectangleRounded({static_cast<float>(x), static_cast<float>(y), 180, 70},
                           0.12F, 8, Color{59, 76, 103, 255});
      DrawText(std::to_string(rune + 1).c_str(), x + 82, y + 22, 24,
               Color{255, 224, 145, 255});
    }
  }
  if (boss.challenge == ed::BossChallenge::StarTrace) {
    DrawText(("Time: " + std::to_string(static_cast<int>(40.0F - app.starTrace.elapsed)) +
              "s   Constellations: " + std::to_string(app.starTrace.constellation) +
              "   Hearts lost: " + std::to_string(app.starTrace.heartsLost))
                 .c_str(),
             355, 255, 21, RAYWHITE);
    for (std::size_t index = 0; index < app.starTrace.stars.size(); ++index) {
      const int x = 300 + static_cast<int>(app.starTrace.starX[index] * 600.0F);
      const int y = 280 + static_cast<int>(app.starTrace.starY[index] * 300.0F);
      DrawCircle(x, y, 30, index == static_cast<std::size_t>(app.starTrace.targetIndex)
                               ? Color{255, 224, 105, 255}
                               : Color{103, 137, 195, 255});
      DrawText(std::to_string(app.starTrace.stars[index] + 1).c_str(), x - 8, y - 10, 20,
               Color{20, 25, 43, 255});
    }
    DrawText("Tap the highlighted stars in order. Wrong stars cost a heart.", 375, 575, 18,
             Color{255, 224, 145, 255});
  } else if (boss.challenge == ed::BossChallenge::Heartpierce) {
    const char* phaseNames[] = {"Cold Scales", "Ember Heat", "Wyrmfire Fury"};
    DrawText(("Phase " + std::to_string(app.heartpierce.phase + 1) + ": " +
              phaseNames[app.heartpierce.phase] + "   Hearts: " +
              std::to_string(app.heartpierce.hearts) + "   Overlap: " +
              std::to_string(static_cast<int>(app.heartpierce.gauge * 100.0F)) + "%")
                 .c_str(),
             350, 255, 21, RAYWHITE);
    DrawText(("Time: " + std::to_string(static_cast<int>(app.heartpierce.totalTime)) +
              "s   Hearts lost: " + std::to_string(app.heartpierce.heartsLost) +
              "   Heartpierce: " +
              std::to_string(static_cast<int>(app.heartpierce.gauge * 100.0F)) + "%")
                 .c_str(), 350, 285, 18, Color{255, 224, 145, 255});
    DrawRectangle(365, 325, 290, 220, Color{45, 34, 43, 255});
    DrawCircle(510, 420, 78, Color{113, 39, 39, 255});
    DrawCircle(510, 420, 58, Color{238, 80, 42, 255});
    DrawCircle(535, 397, 13, Color{255, 220, 106, 255});
    DrawCircleLines(510, 420, 92, Color{230, 220, 190, 255});
    DrawRectangle(800, 275, 80, 300, Color{30, 33, 43, 255});
    DrawRectangle(822, 295, 36, 260, Color{64, 67, 80, 255});
    const int heartY = 555 - static_cast<int>(app.heartpierce.heartY * 240.0F);
    const int lanceY = 555 - static_cast<int>(app.heartpierce.lanceY * 240.0F);
    const float zonePixels = (0.09F + 0.04F * (2 - app.heartpierce.phase)) *
                             (1.0F + app.heartpierce.focus) * 240.0F;
    DrawRectangle(805, heartY - static_cast<int>(zonePixels),
                  70, static_cast<int>(zonePixels * 2.0F),
                  Color{255, 80, 48, 55});
    DrawCircle(840, heartY, 17, app.heartpierce.dartTelegraph > 0.0F
                                  ? Color{255, 255, 255, 255}
                                  : Color{255, 85, 45, 255});
    DrawRectangle(813, lanceY - 12, 54, 24, Color{118, 205, 210, 255});
    DrawTriangle({840, static_cast<float>(lanceY - 32)},
                 {825, static_cast<float>(lanceY - 10)},
                 {855, static_cast<float>(lanceY - 10)}, Color{211, 235, 229, 255});
    DrawRectangle(680, 350, 18, 200, Color{45, 35, 38, 255});
    DrawRectangle(680, 550 - static_cast<int>(app.heartpierce.gauge * 200.0F),
                  18, static_cast<int>(app.heartpierce.gauge * 200.0F),
                  Color{255, 194, 55, 255});
    DrawText("Hold SPACE or mouse button to rise; release to fall.", 350, 600, 18,
             Color{255, 224, 145, 255});
  } else if (boss.challenge == ed::BossChallenge::SoulDodge) {
    DrawText(("Time: " + std::to_string(static_cast<int>(60.0F - app.soulDodge.elapsed)) +
              "s   Hearts: " + std::to_string(app.soulDodge.hearts) +
              "   Hits: " + std::to_string(app.soulDodge.hits)).c_str(),
             390, 255, 21, RAYWHITE);
    DrawRectangle(300, 285, 680, 300, Color{18, 20, 39, 255});
    DrawRectangleLines(300, 285, 680, 300, Color{118, 91, 157, 255});
    for (const auto& hazard : app.soulDodge.hazards) {
      if (hazard.kind == ed::SoulHazardKind::Laser) {
        const int x = 300 + static_cast<int>(hazard.x * 680.0F);
        DrawRectangle(x - 5, 285, 10, 300,
                      hazard.telegraph > 0.0F ? Color{255, 238, 166, 150}
                                             : Color{214, 80, 255, 220});
      } else {
        const float hazardX = hazard.kind == ed::SoulHazardKind::Rain
                                  ? hazard.x
                                  : hazard.x + std::cos(hazard.angle) * hazard.radius;
        const float hazardY = hazard.kind == ed::SoulHazardKind::Rain
                                  ? hazard.y
                                  : hazard.y + std::sin(hazard.angle) * hazard.radius;
        const int x = 300 + static_cast<int>(hazardX * 680.0F);
        const int y = 285 + static_cast<int>(hazardY * 300.0F);
        DrawCircle(x, y, hazard.kind == ed::SoulHazardKind::Ring ? 12 : 8,
                   hazard.telegraph > 0.0F ? Color{255, 255, 255, 160}
                                           : Color{178, 75, 232, 230});
      }
    }
    DrawCircle(300 + static_cast<int>(app.soulDodge.playerX * 680.0F),
               285 + static_cast<int>(app.soulDodge.playerY * 300.0F), 7,
               app.soulDodge.invulnerable > 0.0F ? Color{255, 255, 255, 180}
                                                 : Color{255, 88, 132, 255});
    DrawText("Move with WASD/arrows. Hold Shift for precision movement.", 365, 615, 18,
             Color{255, 224, 145, 255});
  } else if (boss.challenge == ed::BossChallenge::FiveTrials) {
    DrawText(("TRIAL " + std::to_string(app.fiveTrials.phase) +
              " / 5   Hearts: " + std::to_string(app.fiveTrials.hearts) +
              "   Lost: " + std::to_string(app.fiveTrials.heartsLost)).c_str(),
             420, 255, 21, RAYWHITE);
    DrawRectangle(350, 320, 580, 180, Color{32, 35, 53, 255});
    if (app.fiveTrials.phase == 1) {
      DrawText(("BURST: " + std::to_string(std::max(0, 6 - static_cast<int>(app.fiveTrials.burstElapsed))) +
                " seconds remaining").c_str(), 440, 390, 24, RAYWHITE);
      DrawText(("Taps: " + std::to_string(app.fiveTrials.burstTaps) +
                " / 48").c_str(), 540, 440, 22, Color{255, 224, 145, 255});
    } else if (app.fiveTrials.phase == 2) {
      DrawText(("RHYTHM: D F J K   " +
                std::to_string(std::max(0, 30 - static_cast<int>(app.fiveTrials.phaseElapsed))) +
                " seconds").c_str(), 420, 300, 24, RAYWHITE);
      const char* laneKeys[] = {"D", "F", "J", "K"};
      for (int lane = 0; lane < 4; ++lane) {
        const int laneX = 390 + lane * 120;
        DrawRectangle(laneX, 330, 116, 160,
                      lane % 2 == 0 ? Color{37, 43, 66, 255}
                                    : Color{31, 37, 58, 255});
        DrawRectangleLines(laneX, 330, 116, 160, Color{93, 107, 145, 255});
        DrawRectangle(laneX + 5, 450, 106, 35, Color{71, 103, 132, 255});
        DrawRectangleLinesEx({static_cast<float>(laneX + 5), 450, 106, 35},
                             2.0F, Color{255, 224, 145, 255});
        DrawText(laneKeys[lane], laneX + 51, 456, 22, Color{255, 238, 180, 255});
      }
      const auto& chart = app.cinderDance.chart;
      for (std::size_t index = app.cinderDance.nextNote;
           index < chart.size() && index < app.cinderDance.nextNote + 8; ++index) {
        const int x = 450 + chart[index].lane * 120;
        const int y = 470 - static_cast<int>((chart[index].time -
                                               app.cinderDance.elapsed) * 55.0F);
        if (y >= 330 && y <= 440) {
          DrawCircle(x, y, 17, Color{255, 168, 76, 255});
          DrawCircleLines(x, y, 20, Color{255, 238, 180, 255});
        }
      }
      DrawText("Hit when the note reaches the outlined receptor.", 445, 510, 17,
               Color{255, 224, 145, 255});
    } else if (app.fiveTrials.phase == 3) {
      DrawText("PARRY: match LEFT / DOWN / RIGHT", 410, 405, 22, RAYWHITE);
    } else if (app.fiveTrials.phase == 4) {
      DrawText("SOUL DODGE: survive the remix", 455, 405, 22, RAYWHITE);
    } else {
      DrawText("CROWN STRIKE: press SPACE in the center zone", 395, 405, 21, RAYWHITE);
      DrawRectangle(450, 455, 380, 16, Color{57, 55, 70, 255});
      DrawRectangle(635, 451, 10, 24, Color{92, 208, 117, 255});
      DrawRectangle(450 + static_cast<int>(380 * app.crownStrike.needle) - 5,
                    440, 10, 45, RAYWHITE);
    }
  }
}

void drawDungeonScreen(const AppState& app) {
  drawTextCentered("DUNGEON", 108, 34, Color{224, 187, 106, 255});
  DrawText("Select zone with A / D. Press ENTER to simulate a deterministic run.", 30, 155, 17,
           Color{174, 174, 187, 255});
  const auto& zone = app.zones[static_cast<std::size_t>(app.selectedZone)];
  DrawRectangleRounded({280, 230, 720, 220}, 0.08F, 10, Color{53, 52, 65, 255});
  DrawText(zone.name.c_str(), 330, 270, 30, RAYWHITE);
  DrawText(("Zone tier " + std::to_string(zone.tier)).c_str(), 330, 315, 19,
           Color{204, 175, 111, 255});
  DrawText("5 floors  |  auto-battle  |  gold, XP and trophies", 330, 355, 18,
           Color{182, 182, 194, 255});
  DrawText("ENTER to run", 330, 405, 20, Color{122, 217, 255, 255});
}

std::string getCurrentDateString() {
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm* tm = std::localtime(&t);
  char buf[16];
  std::strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
  return std::string(buf);
}

void drawGemHollowScreen(AppState& app) {
  drawTextCentered("GEM HOLLOW", 108, 34, Color{224, 187, 106, 255});
  
  std::string today = getCurrentDateString();
  auto& st = app.game.player;
  
  if (st.lastGemHollowDate != today) {
    if (st.lastGemHollowDate.empty()) {
      st.gemHollowStreak = 0;
    } else {
      st.gemHollowStreak = std::min(10, st.gemHollowStreak + 1);
    }
    st.lastGemHollowDate = today;
    st.gemHollowActive = true;
    st.gemHollowSwingsRemaining = 40 + st.gemHollowStreak;
    st.gemHollowTodayGems = 0;
    st.gemHollowTodayDust = 0;
    st.gemHollowTodayGold = 0;
    
    st.gemHollowVeins.clear();
    for (int i = 0; i < 12; ++i) {
      st.gemHollowVeins.push_back(app.rng.nextRange(1, 3));
    }
  }
  
  DrawText(("Swings Remaining: " + std::to_string(st.gemHollowSwingsRemaining)).c_str(), 50, 150, 20, RAYWHITE);
  DrawText(("Today's Loot: " + std::to_string(st.gemHollowTodayGems) + " Gems | " + 
            std::to_string(st.gemHollowTodayDust) + " Dust | " + 
            std::to_string(st.gemHollowTodayGold) + " Gold").c_str(), 50, 180, 20, Color{248, 211, 89, 255});
            
  const int startX = 300;
  const int startY = 250;
  const int spacingX = 180;
  const int spacingY = 140;
  
  for (int i = 0; i < 12; ++i) {
    int col = i % 4;
    int row = i / 4;
    int x = startX + col * spacingX;
    int y = startY + row * spacingY;
    
    int dur = st.gemHollowVeins[i];
    
    if (dur > 0) {
      DrawRectangle(x, y, 100, 100, Color{80, 80, 90, 255});
      DrawText(std::to_string(dur).c_str(), x + 40, y + 40, 20, RAYWHITE);
      
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        if (mouse.x >= x && mouse.x <= x + 100 && mouse.y >= y && mouse.y <= y + 100) {
          if (st.gemHollowSwingsRemaining > 0) {
            st.gemHollowSwingsRemaining--;
            st.gemHollowVeins[i]--;
            
            bool allCleared = true;
            for (int v : st.gemHollowVeins) {
              if (v > 0) { allCleared = false; break; }
            }
            
            if (st.gemHollowVeins[i] == 0) {
              int roll = app.rng.nextRange(0, 99);
              if (roll < 45) {
                st.gemHollowTodayGems++;
                const char* gemIds[] = {"amber", "garnet", "jade", "moonstone", "sapphire", "bloodstone", "star-ruby", "arcane-opal", "dragon-eye", "aether-diamond"};
                std::string gemId = gemIds[app.rng.nextRange(0, 9)];
                app.inventory.gems[gemId]++;
                app.gemHollowSummaryItems.push_back("Gem: " + gemId);
                setNotice(app, "Found a " + gemId + "!");
              } else if (roll < 80) {
                int dust = 2 + app.rng.nextRange(0, 4);
                st.gemHollowTodayDust += dust;
                st.arcaneDust += dust;
                app.gemHollowSummaryItems.push_back("Arcane Dust x" + std::to_string(dust));
                setNotice(app, "Found " + std::to_string(dust) + " Arcane Dust!");
              } else {
                int gold = 50 + app.rng.nextRange(0, 49);
                st.gemHollowTodayGold += gold;
                st.gold += gold;
                app.gemHollowSummaryItems.push_back("Gold x" + std::to_string(gold));
                setNotice(app, "Found " + std::to_string(gold) + " Gold!");
              }
            } else {
              setNotice(app, "Hit vein!");
            }
            
            // Show summary popup when out of swings or all veins cleared
            if (allCleared || st.gemHollowSwingsRemaining == 0) {
              st.gemHollowSwingsRemaining = 0;
              app.gemHollowShowSummary = true;
            }
          } else {
            app.gemHollowShowSummary = true;
          }
        }
      }
    } else {
      DrawRectangle(x, y, 100, 100, Color{40, 40, 50, 255});
    }
  }
  
  // Summary popup
  if (app.gemHollowShowSummary) {
    DrawRectangle(290, 180, 700, 380, Color{18, 17, 28, 240});
    DrawRectangleLinesEx({290, 180, 700, 380}, 2.0F, Color{224, 187, 106, 255});
    drawTextCentered("GEM HOLLOW COMPLETE", 210, 30, Color{224, 187, 106, 255});
    DrawText(("Gems found: " + std::to_string(st.gemHollowTodayGems)).c_str(), 330, 250, 18, RAYWHITE);
    DrawText(("Arcane Dust: " + std::to_string(st.gemHollowTodayDust)).c_str(), 330, 278, 18, Color{180, 220, 255, 255});
    DrawText(("Gold: " + std::to_string(st.gemHollowTodayGold)).c_str(), 330, 306, 18, Color{248, 211, 89, 255});
    DrawText("Items received:", 330, 345, 16, Color{174, 174, 187, 255});
    int itemY = 368;
    int shown = 0;
    for (const auto& item : app.gemHollowSummaryItems) {
      if (shown >= 6) { DrawText("...", 330, itemY, 15, Color{174, 174, 187, 255}); break; }
      DrawText(("- " + item).c_str(), 330, itemY, 15, RAYWHITE);
      itemY += 22;
      shown++;
    }
    DrawRectangleRounded({490, 520, 300, 36}, 0.2F, 6, Color{112, 76, 45, 255});
    drawTextCentered("Click to close", 535, 18, RAYWHITE);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse = GetMousePosition();
      if (mouse.x >= 490 && mouse.x <= 790 && mouse.y >= 520 && mouse.y <= 556) {
        app.gemHollowShowSummary = false;
        app.gemHollowSummaryItems.clear();
      }
    }
  }
}


void drawEnchantingScreen(AppState& app) {
  drawTextCentered("ENCHANTING", 108, 34, Color{224, 187, 106, 255});
  
  if (app.game.player.level < 20) {
    DrawText("Enchanting unlocks at Level 20.", 300, 300, 20, Color{180, 50, 50, 255});
    return;
  }
  
  DrawText("Select an item from your inventory to enchant.", 50, 150, 18, RAYWHITE);
  
  int y = 200;
  for (size_t i = 0; i < app.inventory.gear.size(); ++i) {
    auto& piece = app.inventory.gear[i];
    std::string text = piece.name + " (Lv " + std::to_string(piece.tier) + ") - " + piece.slot;
    if (!piece.enchantment.empty()) text += " [*]";
    
    Color c = (app.selectedGear == i) ? Color{255, 238, 180, 255} : RAYWHITE;
    DrawText(text.c_str(), 100, y, 18, c);
    y += 30;
  }
  
  DrawText("W / S select gear | N enchant selected", 50, y + 40, 18, Color{174, 174, 187, 255});
  
  if (app.selectedGear < app.inventory.gear.size()) {
    auto& piece = app.inventory.gear[app.selectedGear];
    int dustCost = ed::enchantDustCost(piece);
    int goldCost = ed::enchantGoldCost(piece);
    
    DrawRectangle(600, 200, 400, 300, Color{40, 40, 50, 255});
    DrawText(piece.name.c_str(), 620, 220, 24, RAYWHITE);
    DrawText(("Cost: " + std::to_string(dustCost) + " Dust | " + std::to_string(goldCost) + " Gold").c_str(), 620, 260, 20, Color{248, 211, 89, 255});
    
    if (!piece.enchantment.empty()) {
      DrawText(("Current Enchantment: " + piece.enchantment).c_str(), 620, 300, 18, Color{150, 255, 150, 255});
    } else {
      DrawText("No Enchantment", 620, 300, 18, Color{180, 180, 180, 255});
    }
    
    DrawText(("Your Dust: " + std::to_string(app.game.player.arcaneDust)).c_str(), 620, 360, 18, RAYWHITE);
    DrawText(("Your Gold: " + std::to_string(app.game.player.gold)).c_str(), 620, 390, 18, RAYWHITE);
  }
}


void drawProfileScreen(const AppState& app) {
  drawTextCentered("PROFILE", 108, 34, Color{224, 187, 106, 255});
  const auto& player = app.game.player;
  DrawRectangleRounded({300, 180, 680, 390}, 0.06F, 10, Color{53, 52, 65, 255});
  DrawCircle(640, 285, 64, Color{204, 158, 104, 255});
  DrawRectangleRounded({585, 350, 110, 150}, 0.18F, 8, Color{84, 105, 143, 255});
  DrawText(("Level " + std::to_string(player.level)).c_str(), 410, 230, 23, RAYWHITE);
  DrawText(("XP " + std::to_string(player.xp) + " / " +
            std::to_string(ed::xpToNextLevel(player.level)))
               .c_str(),
           410, 270, 19, Color{185, 185, 199, 255});
  DrawText(("Gold: " + std::to_string(player.gold)).c_str(), 410, 320, 19, Color{248, 211, 89, 255});
  DrawText(("Bosses cleared: " + std::to_string(player.bossesCleared)).c_str(), 410, 355, 19,
           RAYWHITE);
  DrawText(("Echo rank: " + std::to_string(player.echoRank)).c_str(), 410, 390, 19, RAYWHITE);
  DrawText(("Deepest depth: " + std::to_string(player.deepestDepth)).c_str(), 410, 425, 19,
           RAYWHITE);
  DrawText("Procedural paper-doll preview", 520, 530, 17, Color{177, 177, 188, 255});
}

int inventoryGearStartY(const AppState& app) {
  int y = 205;
  const auto advanceBucket = [&y](const std::map<std::string, int>& bucket) {
    int x = 250;
    for (const auto& [_, count] : bucket) {
      static_cast<void>(count);
      x += 155;
      if (x > 1100) {
        x = 250;
        y += 42;
      }
    }
    y += 48;
  };
  advanceBucket(app.inventory.ore);
  advanceBucket(app.inventory.ingots);
  advanceBucket(app.inventory.trophies);
  advanceBucket(app.inventory.gems);
  return y + 42;
}

void syncGearScroll(AppState& app) {
  if (app.inventory.gear.empty()) {
    app.selectedGear = 0;
    return;
  }
  app.selectedGear = std::clamp(app.selectedGear, 0,
                                static_cast<int>(app.inventory.gear.size()) - 1);
  constexpr int contentTop = 140;
  constexpr int contentBottom = 655;
  const int gearStart = inventoryGearStartY(app);
  const int selectedTop = gearStart + (app.selectedGear / 3) * 70;
  const int selectedBottom = selectedTop + 52;
  if (selectedTop - app.inventoryScroll < contentTop) {
    app.inventoryScroll = selectedTop - contentTop;
  } else if (selectedBottom - app.inventoryScroll > contentBottom) {
    app.inventoryScroll = selectedBottom - contentBottom;
  }
}

int inventoryContentHeight(const AppState& app) {
  const int gearStart = inventoryGearStartY(app);
  const int gearRows = (static_cast<int>(app.inventory.gear.size()) + 2) / 3;
  return app.inventory.gear.empty() ? gearStart + 30 : gearStart + gearRows * 70 + 45;
}

void clampInventoryScroll(AppState& app) {
  constexpr int contentTop = 140;
  constexpr int contentBottom = 655;
  const int maxScroll = std::max(0, inventoryContentHeight(app) - contentBottom);
  app.inventoryScroll = std::clamp(app.inventoryScroll, 0, maxScroll);
  syncGearScroll(app);
  app.inventoryScroll = std::clamp(app.inventoryScroll, 0, maxScroll);
}

void drawInventoryScreen(const AppState& app) {
  drawTextCentered("INVENTORY", 108, 34, Color{224, 187, 106, 255});
  DrawText("Mouse wheel or W/S scroll inventory", 70, 134, 15, Color{174, 174, 187, 255});
  BeginScissorMode(0, 140, 1280, 515);
  const int offset = app.inventoryScroll;
  DrawText("Resources", 70, 164 - offset, 24, Color{235, 224, 191, 255});
  int y = 205;
  const auto drawBucket = [&](const char* title, const std::map<std::string, int>& bucket) {
    DrawText(title, 70, y - offset, 18, Color{174, 174, 187, 255});
    int x = 250;
    for (const auto& [id, count] : bucket) {
      DrawRectangleRounded({static_cast<float>(x), static_cast<float>(y - 5 - offset), 145, 34}, 0.18F, 6,
                           Color{52, 52, 67, 255});
      DrawText((id + " x" + std::to_string(count)).c_str(), x + 8, y + 4 - offset, 14, RAYWHITE);
      x += 155;
      if (x > 1100) {
        x = 250;
        y += 42;
      }
    }
    y += 48;
  };
  drawBucket("Ore", app.inventory.ore);
  drawBucket("Ingots", app.inventory.ingots);
  drawBucket("Trophies", app.inventory.trophies);
  drawBucket("Gems", app.inventory.gems);
  DrawText("Gear", 70, y - offset, 24, Color{235, 224, 191, 255});
  y += 42;
  if (app.inventory.gear.empty()) {
    DrawText("No gear crafted yet. Mine ore, smelt it, then visit Crafting.", 70, y - offset, 18,
             Color{174, 174, 187, 255});
  } else {
    constexpr int cardWidth = 330;
    constexpr int cardHeight = 52;
    constexpr int cardGap = 18;
    constexpr int rowHeight = 70;
    for (std::size_t index = 0; index < app.inventory.gear.size(); ++index) {
      const auto& piece = app.inventory.gear[static_cast<std::size_t>(index)];
      const int column = static_cast<int>(index % 3);
      const int row = static_cast<int>(index / 3);
      const bool selected = static_cast<int>(index) == app.selectedGear;
      DrawRectangleRounded({70.0F + static_cast<float>(column) * 370.0F,
                            static_cast<float>(y + row * rowHeight - offset), cardWidth, cardHeight},
                           0.12F, 6, selected ? Color{112, 76, 45, 255} : Color{52, 52, 67, 255});
      DrawText((piece.name + " +" + std::to_string(piece.upgradeLevel)).c_str(),
               84 + column * 370, y + row * rowHeight + 8 - offset, 17,
               RAYWHITE);
      DrawText(("Tier " + std::to_string(piece.tier) + " | " + piece.slot).c_str(),
               84 + column * 370, y + row * rowHeight + 30 - offset, 13,
               Color{180, 180, 192, 255});
    }
  }
  EndScissorMode();
}

void drawCraftingScreen(const AppState& app) {
  drawTextCentered("CRAFTING", 108, 34, Color{224, 187, 106, 255});
  const auto& recipe = app.recipes[static_cast<std::size_t>(app.selectedRecipe)];
  DrawText("Scroll or W/S through material groups. Click a recipe, then ENTER to craft.", 70, 155, 17,
           Color{174, 174, 187, 255});
  const int listX = 70;
  const int listY = 195;
  const int listWidth = 540;
  const int listHeight = 405;
  DrawRectangleRounded({static_cast<float>(listX), static_cast<float>(listY),
                        static_cast<float>(listWidth), static_cast<float>(listHeight)},
                       0.04F, 8, Color{39, 39, 52, 255});
  BeginScissorMode(listX, listY, listWidth, listHeight);
  int row = 0;
  for (int tier = 1; tier <= 10; ++tier) {
    DrawText(("TIER " + std::to_string(tier) + " MATERIALS").c_str(), listX + 18,
             listY + row * 42 - app.craftingScroll + 10, 16,
             Color{224, 187, 106, 255});
    ++row;
    for (std::size_t index = 0; index < app.recipes.size(); ++index) {
      const auto& item = app.recipes[index];
      if (item.tier != tier) continue;
      const int y = listY + row * 42 - app.craftingScroll;
      const bool selected = static_cast<int>(index) == app.selectedRecipe;
      DrawRectangleRounded({static_cast<float>(listX + 10), static_cast<float>(y),
                            static_cast<float>(listWidth - 20), 36.0F},
                           0.2F, 6, selected ? Color{104, 72, 48, 255}
                                             : Color{53, 52, 65, 255});
      DrawText(item.itemName.c_str(), listX + 24, y + 8, 16,
               selected ? RAYWHITE : Color{196, 196, 207, 255});
      DrawText(item.slot.c_str(), listX + 390, y + 9, 14, Color{177, 177, 191, 255});
      ++row;
    }
  }
  EndScissorMode();
  DrawRectangleLinesEx({static_cast<float>(listX), static_cast<float>(listY),
                        static_cast<float>(listWidth), static_cast<float>(listHeight)},
                       2.0F, Color{91, 82, 104, 255});
  DrawRectangleRounded({650, 195, 560, 405}, 0.05F, 8, Color{53, 52, 65, 255});
  DrawText(recipe.itemName.c_str(), 700, 240, 30, RAYWHITE);
  DrawText(("Tier " + std::to_string(recipe.tier) + " " + recipe.slot).c_str(), 700, 285, 20,
           Color{204, 175, 111, 255});
  DrawText(("Cost: " + std::to_string(recipe.ingots) + " ingots  |  " +
            std::to_string(recipe.trophies) + " trophies  |  " +
            std::to_string(recipe.gold) + " gold")
               .c_str(),
           700, 330, 18, Color{190, 190, 202, 255});
  DrawText(recipe.requiresGem ? "Also requires any gem" : "No gem required", 700, 370, 18,
           recipe.requiresGem ? Color{129, 214, 241, 255} : Color{174, 174, 187, 255});
  const int oreCount = ed::countItem(app.inventory, "ore", app.blockSpec.materialId);
  const int ingotCount = ed::countItem(app.inventory, "ingots", materialIdForTier(recipe.tier));
  DrawText(("Current ore: " + std::to_string(oreCount) + "   Current ingots: " +
            std::to_string(ingotCount) + "   Dust: " +
            std::to_string(app.game.player.arcaneDust))
               .c_str(),
           700, 410, 18, Color{180, 210, 178, 255});
  DrawText("Smelt 4 matching ore with M. Crafting adds gear to Inventory.", 700, 455, 16,
           Color{174, 174, 187, 255});
  DrawText("A/D select recipe   W/S or wheel scroll groups", 700, 520, 16,
           Color{174, 174, 187, 255});
}

void updateEffects(AppState& app, float delta) {
  app.blockPulse = std::max(0.0F, app.blockPulse - delta);
  app.shake = std::max(0.0F, app.shake - delta);
  app.noticeLifetime = std::max(0.0F, app.noticeLifetime - delta);
  if (app.screen == Screen::Campaign && app.campaignAttempt) {
    const auto challenge = app.bosses[static_cast<std::size_t>(app.selectedBoss)].challenge;
    if (challenge == ed::BossChallenge::FrenzyDig) {
      ed::updateFrenzyDig(app.frenzyDig, delta, app.rng);
      if (app.frenzyDig.finished) {
        const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
        app.campaignScore = ed::frenzyDigScore(app.frenzyDig, stats.attack);
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::SoulDodge) {
      const float moveX = (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) -
                          (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A));
      const float moveY = (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) -
                          (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W));
      ed::updateSoulDodge(app.soulDodge, delta, moveX, moveY,
                          IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT), 10);
      if (app.soulDodge.finished) {
        app.campaignScore = ed::soulDodgeScore(app.soulDodge);
        app.campaignHeartsLost = app.soulDodge.hits;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::FiveTrials) {
      if (app.fiveTrials.phase == 1) {
        app.fiveTrials.burstElapsed += delta;
        if (app.fiveTrials.burstElapsed >= 6.0F) {
          const bool passed = app.fiveTrials.burstTaps >= 48;
          ed::advanceFiveTrials(app.fiveTrials, passed, passed ? 0 : 1);
          if (app.fiveTrials.phase == 2) {
            app.cinderDance = ed::startCinderDance(0xF17EULL, 30.0F);
            app.cinderDance.gauntletMode = true;
          }
        }
      } else if (app.fiveTrials.phase == 2) {
        app.fiveTrials.phaseElapsed += delta;
        ed::updateCinderDance(app.cinderDance, delta);
        if (app.fiveTrials.phaseElapsed >= 30.0F) {
          app.cinderDance.finished = true;
          app.cinderDance.failed = app.cinderDance.groove == 0;
        }
        if (app.cinderDance.finished) {
          const bool passed = !app.cinderDance.failed;
          ed::advanceFiveTrials(app.fiveTrials, passed, passed ? 0 : 1);
          if (app.fiveTrials.phase == 3) {
            app.parryDuel = ed::startParryDuel(0xF17EULL, 12);
          }
        }
      } else if (app.fiveTrials.phase == 3) {
        ed::updateParryDuel(app.parryDuel, delta, app.rng);
        if (app.parryDuel.finished) {
          ed::advanceFiveTrials(app.fiveTrials, app.parryDuel.cleared,
                                app.parryDuel.heartsLost);
          if (app.fiveTrials.phase == 4) {
            app.soulDodge = ed::startSoulDodge(0xF17EULL, 0, 40.0F);
          }
        }
      } else if (app.fiveTrials.phase == 4) {
        const float moveX = (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) -
                            (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A));
        const float moveY = (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) -
                            (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W));
        ed::updateSoulDodge(app.soulDodge, delta, moveX, moveY,
                            IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT), 10);
        if (app.soulDodge.finished) {
          ed::advanceFiveTrials(app.fiveTrials, app.soulDodge.cleared,
                                app.soulDodge.hits);
          if (app.fiveTrials.phase == 5) app.crownStrike = ed::startCrownStrike();
        }
      } else {
        ed::updateCrownStrike(app.crownStrike, delta);
        if (app.crownStrike.finished) {
          ed::advanceFiveTrials(app.fiveTrials, app.crownStrike.cleared,
                                app.crownStrike.misses);
        }
      }
      if (app.fiveTrials.finished) {
        app.campaignScore = ed::fiveTrialsScore(app.fiveTrials);
        app.campaignHeartsLost = app.fiveTrials.heartsLost;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::StarTrace) {
      ed::updateStarTrace(app.starTrace, delta, app.rng);
      if (app.starTrace.finished) {
        const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
        app.campaignScore = ed::starTraceScore(app.starTrace, stats.speed);
        app.campaignHeartsLost = app.starTrace.heartsLost;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::Heartpierce) {
      ed::updateHeartpierce(app.heartpierce, delta,
                            IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_BUTTON_LEFT));
      if (app.heartpierce.finished) {
        app.campaignScore = ed::heartpierceScore(app.heartpierce);
        app.campaignHeartsLost = app.heartpierce.heartsLost;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::BoneDuel) {
      ed::updateBoneDuel(app.boneDuel, delta);
      if (app.boneDuel.finished) {
        const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
        app.campaignScore = ed::boneDuelScore(app.boneDuel, stats.attack, stats.pierce);
        app.campaignHeartsLost = app.boneDuel.misses;
        if (app.boneDuel.misses >= 3) app.campaignScore = 0;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::CinderDance) {
      ed::updateCinderDance(app.cinderDance, delta);
      if (app.cinderDance.finished) {
        const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
        app.campaignScore = ed::cinderDanceScore(app.cinderDance, stats.attack) / 100;
        app.campaignHeartsLost = app.cinderDance.failed ? 3 : 0;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::HeadHunt) {
      const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
      ed::updateHeadHunt(app.headHunt, delta, app.rng,
                         static_cast<double>(stats.speed) / 100.0);
      if (app.headHunt.finished) {
        app.campaignScore = ed::headHuntScore(app.headHunt, stats.attack);
        app.campaignHeartsLost = app.headHunt.heartsLost;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::ParryDuel) {
      ed::updateParryDuel(app.parryDuel, delta, app.rng);
      if (app.parryDuel.finished) {
        const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
        app.campaignScore = ed::parryDuelScore(app.parryDuel, stats.attack);
        app.campaignHeartsLost = app.parryDuel.heartsLost;
        attemptCampaign(app);
      }
    } else if (challenge == ed::BossChallenge::RuneRecall) {
      ed::updateRuneRecall(app.runeRecall, delta, app.rng);
      if (app.runeRecall.finished) {
        const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
        app.campaignScore = ed::runeRecallScore(app.runeRecall, stats.focusBasisPoints / 100);
        app.campaignHeartsLost = app.runeRecall.mistakes;
        attemptCampaign(app);
      }
    }
  }
  if (app.game.dirty &&
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::steady_clock::now() - app.lastAutosave)
              .count() >= 10) {
    requestSave(app);
    app.lastAutosave = std::chrono::steady_clock::now();
  }
  for (auto& text : app.floatingText) {
    text.lifetime -= delta;
    text.position.y -= 28.0F * delta;
  }
  app.floatingText.erase(
      std::remove_if(app.floatingText.begin(), app.floatingText.end(),
                     [](const FloatingText& text) { return text.lifetime <= 0.0F; }),
      app.floatingText.end());
  for (auto& particle : app.particles) {
    particle.lifetime -= delta;
    particle.position.x += particle.velocity.x * delta;
    particle.position.y += particle.velocity.y * delta;
    particle.velocity.y += 160.0F * delta;
  }
  app.particles.erase(
      std::remove_if(app.particles.begin(), app.particles.end(),
                     [](const Particle& particle) { return particle.lifetime <= 0.0F; }),
      app.particles.end());
}

bool handleActiveCampaignInput(AppState& app) {
  if (app.screen != Screen::Campaign || !app.campaignAttempt) return false;
  if (IsKeyPressed(KEY_BACKSPACE)) {
    app.campaignAttempt = false;
    app.campaignIntro = false;
    setNotice(app, "Boss attempt abandoned.");
    return true;
  }

  const auto challenge =
      app.bosses[static_cast<std::size_t>(app.selectedBoss)].challenge;
  if (challenge == ed::BossChallenge::FrenzyDig &&
      (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
    const Vector2 mouse = GetMousePosition();
    const float normalizedX = std::clamp((mouse.x - 290.0F) / 700.0F, 0.0F, 1.0F);
    const float normalizedY = std::clamp((mouse.y - 220.0F) / 260.0F, 0.0F, 1.0F);
    ed::tapFrenzyDig(app.frenzyDig, normalizedX, normalizedY, app.rng);
    return true;
  }
  if (challenge == ed::BossChallenge::BoneDuel &&
      (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
    const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
    if (!ed::strikeBoneDuel(
            app.boneDuel, static_cast<double>(stats.focusBasisPoints) / 10000.0)) {
      setNotice(app, "Miss! The needle must be inside the green zone.");
    }
    return true;
  }
  if (challenge == ed::BossChallenge::CinderDance) {
    const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
    const int keys[] = {KEY_D, KEY_F, KEY_J, KEY_K};
    for (int lane = 0; lane < 4; ++lane) {
      if (IsKeyPressed(keys[lane])) {
        ed::judgeCinderDance(
            app.cinderDance, lane,
            static_cast<double>(stats.focusBasisPoints) / 10000.0);
      }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      const Vector2 mouse = GetMousePosition();
      if (mouse.y >= 290.0F && mouse.y <= 550.0F) {
        const int lane = static_cast<int>((mouse.x - 350.0F) / 150.0F);
        if (lane >= 0 && lane < 4) {
          ed::judgeCinderDance(
              app.cinderDance, lane,
              static_cast<double>(stats.focusBasisPoints) / 10000.0);
        }
      }
    }
    return true;
  }
  if (challenge == ed::BossChallenge::HeadHunt) {
    int cell = -1;
    const int keys[] = {KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE,
                        KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE};
    for (int index = 0; index < 9; ++index) {
      if (IsKeyPressed(keys[index])) cell = index;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      const Vector2 mouse = GetMousePosition();
      if (mouse.x >= 430.0F && mouse.x < 850.0F &&
          mouse.y >= 290.0F && mouse.y < 575.0F) {
        const int column = static_cast<int>((mouse.x - 430.0F) / 140.0F);
        const int row = static_cast<int>((mouse.y - 290.0F) / 95.0F);
        if (column < 3 && row < 3) cell = row * 3 + column;
      }
    }
    if (cell >= 0) ed::hitHeadHunt(app.headHunt, cell);
    return true;
  }
  if (challenge == ed::BossChallenge::ParryDuel) {
    int direction = -1;
    if (IsKeyPressed(KEY_LEFT)) direction = 0;
    if (IsKeyPressed(KEY_DOWN)) direction = 1;
    if (IsKeyPressed(KEY_RIGHT)) direction = 2;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      const Vector2 mouse = GetMousePosition();
      if (mouse.y >= 390.0F && mouse.y <= 460.0F &&
          mouse.x >= 330.0F && mouse.x < 990.0F) {
        direction = std::clamp(static_cast<int>((mouse.x - 330.0F) / 220.0F), 0, 2);
      }
    }
    if (direction >= 0 && !ed::parryDuelInput(app.parryDuel, direction)) {
      setNotice(app, "Wrong parry or feint.");
    }
    return true;
  }
  if (challenge == ed::BossChallenge::RuneRecall) {
    int rune = -1;
    const int keys[] = {KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX};
    for (int index = 0; index < 6; ++index) {
      if (IsKeyPressed(keys[index])) rune = index;
    }
    if (rune >= 0 && !ed::inputRuneRecall(app.runeRecall, rune)) {
      setNotice(app, "Wrong rune. The sequence resets.");
    }
    return true;
  }
  if (challenge == ed::BossChallenge::StarTrace) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      const Vector2 mouse = GetMousePosition();
      for (std::size_t index = 0; index < app.starTrace.stars.size(); ++index) {
        const int x = 300 + static_cast<int>(app.starTrace.starX[index] * 600.0F);
        const int y = 280 + static_cast<int>(app.starTrace.starY[index] * 300.0F);
        if (CheckCollisionPointCircle(mouse, {static_cast<float>(x), static_cast<float>(y)}, 34.0F)) {
          ed::hitStarTrace(app.starTrace, app.starTrace.stars[index]);
          break;
        }
      }
    }
    return true;
  }
  if (challenge == ed::BossChallenge::Heartpierce) {
    return true;
  }
  if (challenge == ed::BossChallenge::SoulDodge) {
    return true;
  }
  if (challenge == ed::BossChallenge::FiveTrials) {
    if (app.fiveTrials.phase == 1 &&
        (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
      ++app.fiveTrials.burstTaps;
    } else if (app.fiveTrials.phase == 2) {
      const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
      const int keys[] = {KEY_D, KEY_F, KEY_J, KEY_K};
      for (int lane = 0; lane < 4; ++lane) {
        if (IsKeyPressed(keys[lane])) {
          ed::judgeCinderDance(app.cinderDance, lane,
                               static_cast<double>(stats.focusBasisPoints) / 10000.0);
        }
      }
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const Vector2 mouse = GetMousePosition();
        if (mouse.y >= 440.0F && mouse.y <= 500.0F &&
            mouse.x >= 390.0F && mouse.x < 870.0F) {
          const int lane = static_cast<int>((mouse.x - 390.0F) / 120.0F);
          if (lane >= 0 && lane < 4) {
            ed::judgeCinderDance(
                app.cinderDance, lane,
                static_cast<double>(stats.focusBasisPoints) / 10000.0);
          }
        }
      }
    } else if (app.fiveTrials.phase == 3) {
      int direction = -1;
      if (IsKeyPressed(KEY_LEFT)) direction = 0;
      if (IsKeyPressed(KEY_DOWN)) direction = 1;
      if (IsKeyPressed(KEY_RIGHT)) direction = 2;
      if (direction >= 0) ed::parryDuelInput(app.parryDuel, direction);
    } else if (app.fiveTrials.phase == 5 &&
               (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
      ed::strikeCrown(app.crownStrike, 0.0F);
    }
    return true;
  }
  return true;
}

void handleInput(AppState& app) {
  if (handleActiveCampaignInput(app)) return;
  const bool cinderDanceActive =
      app.screen == Screen::Campaign && app.campaignAttempt &&
      app.bosses[static_cast<std::size_t>(app.selectedBoss)].challenge ==
          ed::BossChallenge::CinderDance;
  if (IsKeyPressed(KEY_F) && !cinderDanceActive) {
    signOut(app);
    return;
  }
  if (IsKeyPressed(KEY_F5)) {
    saveNow(app);
    app.lastAutosave = std::chrono::steady_clock::now();
  }
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    const Vector2 mouse = GetMousePosition();
    if (CheckCollisionPointRec(mouse, {1010, 14, 72, 34})) {
      saveNow(app);
      app.lastAutosave = std::chrono::steady_clock::now();
      return;
    }
    if (CheckCollisionPointRec(mouse, {1090, 14, 160, 34})) {
      signOut(app);
      return;
    }
    if (app.sidebarOpen) {
      if (CheckCollisionPointRec(mouse, {150, 15, 30, 30})) {
        app.sidebarOpen = false;
      } else {
        const Screen navScreens[] = {Screen::Mine, Screen::Inventory, Screen::Crafting, Screen::Campaign, Screen::Dungeon, Screen::GemHollow, Screen::Enchanting, Screen::Profile};
        for (int i = 0; i < 8; ++i) {
          const int y = 80 + i * 50;
          if (CheckCollisionPointRec(mouse, {10, static_cast<float>(y), 180, 40})) {
            app.screen = navScreens[i];
            app.sidebarOpen = false;
            break;
          }
        }
      }
    } else {
      if (CheckCollisionPointRec(mouse, {1220, 100, 40, 40})) {
        app.sidebarOpen = true;
      }
    }
  }

  if (IsKeyPressed(KEY_ONE)) app.screen = Screen::Mine;
  if (IsKeyPressed(KEY_TWO)) app.screen = Screen::Inventory;
  if (IsKeyPressed(KEY_THREE)) app.screen = Screen::Crafting;
  if (IsKeyPressed(KEY_FOUR)) app.screen = Screen::Campaign;
  if (IsKeyPressed(KEY_FIVE)) app.screen = Screen::Dungeon;
  if (IsKeyPressed(KEY_SIX)) app.screen = Screen::GemHollow;
  if (IsKeyPressed(KEY_SEVEN)) app.screen = Screen::Enchanting;
  if (IsKeyPressed(KEY_EIGHT)) app.screen = Screen::Profile;

  if (app.screen == Screen::Mine) {
    if (IsKeyPressed(KEY_Q)) {
      changeLayer(app, -1);
    }
    if (IsKeyPressed(KEY_E)) {
      changeLayer(app, 1);
    }
    if (IsKeyPressed(KEY_SPACE)) mine(app, {640, 305});
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      const Vector2 mouse = GetMousePosition();
      if (CheckCollisionPointRec(mouse, {400, 190, 480, 330})) mine(app, mouse);
    }
  } else if (app.screen == Screen::Inventory) {
    if (IsKeyPressed(KEY_A) && !app.inventory.gear.empty()) {
      app.selectedGear = std::max(0, app.selectedGear - 1);
    }
    if (IsKeyPressed(KEY_D) && !app.inventory.gear.empty()) {
      app.selectedGear = std::min(static_cast<int>(app.inventory.gear.size()) - 1,
                                  app.selectedGear + 1);
    }
    if (IsKeyPressed(KEY_W)) {
      app.inventoryScroll -= 120;
    }
    if (IsKeyPressed(KEY_S)) {
      app.inventoryScroll += 120;
    }
    const float wheel = GetMouseWheelMove();
    if (wheel != 0.0F) {
      app.inventoryScroll -= static_cast<int>(wheel * 80.0F);
    }
    clampInventoryScroll(app);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !app.inventory.gear.empty()) {
      const int gearY = inventoryGearStartY(app) - app.inventoryScroll;
      const Vector2 mouse = GetMousePosition();
      if (mouse.y >= gearY && mouse.y < gearY + inventoryContentHeight(app) &&
          mouse.x >= 70.0F && mouse.x < 1180.0F) {
        const int column = std::clamp(static_cast<int>((mouse.x - 70.0F) / 370.0F), 0, 2);
        const int row = static_cast<int>((mouse.y - gearY) / 70.0F);
        const int index = row * 3 + column;
        if (index >= 0 && index < static_cast<int>(app.inventory.gear.size())) {
          app.selectedGear = index;
          clampInventoryScroll(app);
        }
      }
    }
    if (!app.inventory.gear.empty() && IsKeyPressed(KEY_U)) {
      auto& piece = app.inventory.gear[static_cast<std::size_t>(app.selectedGear)];
      if (ed::upgradeGear(app.game, piece)) {
        setNotice(app, "Gear upgraded.");
      } else {
        const int cost = ed::upgradeGoldCost(piece);
        setNotice(app, cost == 0 ? "This gear is already at maximum level."
                                 : "Need " + std::to_string(cost) + " gold and " +
                                       std::to_string(piece.upgradeLevel + 1) + " Arcane Dust.");
      }
    }
    if (!app.inventory.gear.empty() && IsKeyPressed(KEY_E)) {
      auto& piece = app.inventory.gear[static_cast<std::size_t>(app.selectedGear)];
      const bool equipped = ed::equipGear(app.inventory, piece);
      if (equipped && piece.slot == "pickaxe") {
        app.game.player.pickaxeTier = std::max(app.game.player.pickaxeTier, piece.tier);
      }
      app.game.dirty = equipped;
      setNotice(app, equipped ? "Gear equipped. Stats updated." : "Cannot equip gear.");
    }
    if (!app.inventory.gear.empty() && IsKeyPressed(KEY_N)) {
      app.screen = Screen::Enchanting;
    }
    if (!app.inventory.gear.empty() && IsKeyPressed(KEY_G)) {
      auto& piece = app.inventory.gear[static_cast<std::size_t>(app.selectedGear)];
      if (piece.socketedGems.size() < piece.sockets) {
        std::string gemToInsert = "";
        for (const auto& [gemId, count] : app.inventory.gems) {
          if (count > 0) {
            gemToInsert = gemId;
            break;
          }
        }
        if (!gemToInsert.empty()) {
          ed::insertGem(app.inventory, piece, gemToInsert);
          app.game.dirty = true;
          setNotice(app, "Inserted " + gemToInsert + ".");
        } else {
          setNotice(app, "No gems available.");
        }
      } else {
        setNotice(app, "No free sockets.");
      }
    }
    if (!app.inventory.gear.empty() && IsKeyPressed(KEY_R)) {
      auto& piece = app.inventory.gear[static_cast<std::size_t>(app.selectedGear)];
      if (!piece.socketedGems.empty()) {
        ed::removeGem(app.inventory, piece, piece.socketedGems.size() - 1);
        app.game.dirty = true;
        setNotice(app, "Removed gem.");
      } else {
        setNotice(app, "No gems to remove.");
      }
    }
  } else if (app.screen == Screen::Crafting) {
    if (IsKeyPressed(KEY_A)) {
      app.selectedRecipe = std::max(0, app.selectedRecipe - 1);
    }
    if (IsKeyPressed(KEY_D)) {
      app.selectedRecipe = std::min(static_cast<int>(app.recipes.size()) - 1,
                                    app.selectedRecipe + 1);
    }
    const auto& recipe = app.recipes[static_cast<std::size_t>(app.selectedRecipe)];
    if (IsKeyPressed(KEY_W)) {
      app.craftingScroll = std::max(0, app.craftingScroll - 120);
    }
    if (IsKeyPressed(KEY_S)) {
      app.craftingScroll += 120;
    }
    const float wheel = GetMouseWheelMove();
    if (wheel != 0.0F) {
      app.craftingScroll -= static_cast<int>(wheel * 80.0F);
    }
    const int craftingContentHeight =
        (10 + static_cast<int>(app.recipes.size())) * 42;
    app.craftingScroll = std::clamp(app.craftingScroll, 0, craftingContentHeight - 405);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      const Vector2 mouse = GetMousePosition();
      if (mouse.x >= 70.0F && mouse.x < 610.0F &&
          mouse.y >= 195.0F && mouse.y < 600.0F) {
        const int targetRow =
            (static_cast<int>(mouse.y) - 195 + app.craftingScroll) / 42;
        int row = 0;
        bool selected = false;
        for (int tier = 1; tier <= 10 && !selected; ++tier) {
          ++row;
          for (std::size_t index = 0; index < app.recipes.size(); ++index) {
            if (app.recipes[index].tier != tier) continue;
            if (row == targetRow) {
              app.selectedRecipe = static_cast<int>(index);
              selected = true;
              break;
            }
            ++row;
          }
        }
      }
    }
    if (IsKeyPressed(KEY_M)) {
      const int tier = std::clamp(recipe.tier, 1, 10);
      const int recipeLayer = std::clamp(tier - 1, 0, static_cast<int>(app.layers.size()) - 1);
      if (!ed::canUnlockLayer(app.game, app.layers[static_cast<std::size_t>(recipeLayer)])) {
        setNotice(app, layerLockReason(app.game, app.layers[static_cast<std::size_t>(recipeLayer)]));
      } else {
        setNotice(app, ed::smeltOre(app.inventory, materialIdForTier(tier), 1)
                            ? "Smelted 4 ore into 1 ingot."
                            : "Need 4 matching ore.");
      }
    }
    if (IsKeyPressed(KEY_ENTER)) {
      if (ed::craftGear(app.game, app.inventory, recipe)) {
        setNotice(app, "Gear crafted. Check Inventory.");
      } else {
        setNotice(app, craftFailureReason(app, recipe));
      }
    }
  } else if (app.screen == Screen::Campaign) {
    if (IsKeyPressed(KEY_A)) {
      const int nextBoss = std::max(0, app.selectedBoss - 1);
      if (nextBoss != app.selectedBoss &&
          !ed::isBossUnlocked(app.game, app.campaign, app.bosses,
                              static_cast<std::size_t>(nextBoss))) {
        setNotice(app, "That boss is locked. Clear the previous boss and meet its level requirement.");
      } else {
        app.selectedBoss = nextBoss;
        app.campaignScore = 0;
        app.campaignAttempt = false;
        app.campaignIntro = false;
      }
    }
    if (IsKeyPressed(KEY_D)) {
      const int nextBoss = std::min(static_cast<int>(app.bosses.size()) - 1, app.selectedBoss + 1);
      if (!ed::isBossUnlocked(app.game, app.campaign, app.bosses,
                              static_cast<std::size_t>(nextBoss))) {
        const auto& boss = app.bosses[static_cast<std::size_t>(nextBoss)];
        setNotice(app, "That boss is locked. Reach level " + std::to_string(boss.minLevel) +
                           " and clear the previous boss.");
      } else {
        app.selectedBoss = nextBoss;
        app.campaignScore = 0;
        app.campaignAttempt = false;
        app.campaignIntro = false;
      }
    }
    if (IsKeyPressed(KEY_ENTER)) {
      if (!app.campaignAttempt) {
        const auto bossIndex = static_cast<std::size_t>(app.selectedBoss);
        if (!ed::isBossUnlocked(app.game, app.campaign, app.bosses, bossIndex)) {
          const auto& lockedBoss = app.bosses[bossIndex];
          setNotice(app, "Boss locked. Reach level " + std::to_string(lockedBoss.minLevel) +
                             " and clear the previous boss.");
        } else if (app.campaignIntro) {
          app.campaignIntro = false;
          startCampaignAttempt(app);
        } else {
          app.campaignIntro = true;
        }
      }
    }
    if (app.campaignAttempt) {
      const auto challenge =
          app.bosses[static_cast<std::size_t>(app.selectedBoss)].challenge;
      if (challenge == ed::BossChallenge::FrenzyDig &&
          (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        const Vector2 mouse = GetMousePosition();
        const float normalizedX = std::clamp((mouse.x - 290.0F) / 700.0F, 0.0F, 1.0F);
        const float normalizedY = std::clamp((mouse.y - 220.0F) / 260.0F, 0.0F, 1.0F);
        ed::tapFrenzyDig(app.frenzyDig, normalizedX, normalizedY, app.rng);
      }
      if (challenge == ed::BossChallenge::BoneDuel &&
          (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
        const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
        if (!ed::strikeBoneDuel(
                app.boneDuel, static_cast<double>(stats.focusBasisPoints) / 10000.0)) {
          setNotice(app, "Miss! The needle must be inside the green zone.");
        }
      }
      if (challenge == ed::BossChallenge::CinderDance) {
        const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
        const int keys[] = {KEY_D, KEY_F, KEY_J, KEY_K};
        for (int lane = 0; lane < 4; ++lane) {
          if (IsKeyPressed(keys[lane])) {
            ed::judgeCinderDance(
                app.cinderDance, lane,
                static_cast<double>(stats.focusBasisPoints) / 10000.0);
          }
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
          const Vector2 mouse = GetMousePosition();
          if (mouse.y >= 290.0F && mouse.y <= 550.0F) {
            const int lane = static_cast<int>((mouse.x - 350.0F) / 150.0F);
            if (lane >= 0 && lane < 4) {
              ed::judgeCinderDance(
                  app.cinderDance, lane,
                  static_cast<double>(stats.focusBasisPoints) / 10000.0);
            }
          }
        }
      }
      if (challenge == ed::BossChallenge::HeadHunt &&
          (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) ||
           IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_THREE) ||
           IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_FIVE) || IsKeyPressed(KEY_SIX) ||
           IsKeyPressed(KEY_SEVEN) || IsKeyPressed(KEY_EIGHT) || IsKeyPressed(KEY_NINE))) {
        int cell = -1;
        const int keys[] = {KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE,
                            KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE};
        for (int index = 0; index < 9; ++index) {
          if (IsKeyPressed(keys[index])) cell = index;
        }
        if (cell < 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
          const Vector2 mouse = GetMousePosition();
          if (mouse.x >= 430.0F && mouse.x < 850.0F &&
              mouse.y >= 290.0F && mouse.y < 575.0F) {
            const int column = static_cast<int>((mouse.x - 430.0F) / 140.0F);
            const int row = static_cast<int>((mouse.y - 290.0F) / 95.0F);
            if (column < 3 && row < 3) cell = row * 3 + column;
          }
        }
        if (cell >= 0) ed::hitHeadHunt(app.headHunt, cell);
      }
    }
    if (IsKeyPressed(KEY_BACKSPACE) && (app.campaignAttempt || app.campaignIntro)) {
      app.campaignAttempt = false;
      app.campaignIntro = false;
      setNotice(app, "Boss attempt abandoned.");
    }
  } else if (app.screen == Screen::Dungeon) {
    if (IsKeyPressed(KEY_A)) {
      const int nextZone = std::max(0, app.selectedZone - 1);
      if (nextZone != app.selectedZone && !dungeonUnlocked(app, nextZone)) {
        setNotice(app, dungeonLockReason(app, nextZone));
      } else {
        app.selectedZone = nextZone;
      }
    }
    if (IsKeyPressed(KEY_D)) {
      const int nextZone = std::min(static_cast<int>(app.zones.size()) - 1, app.selectedZone + 1);
      if (!dungeonUnlocked(app, nextZone)) {
        setNotice(app, dungeonLockReason(app, nextZone));
      } else {
        app.selectedZone = nextZone;
      }
    }
    if (IsKeyPressed(KEY_ENTER)) {
      if (!dungeonUnlocked(app, app.selectedZone)) {
        setNotice(app, dungeonLockReason(app, app.selectedZone));
      } else {
        runDungeon(app);
      }
    }
  }
}

} // namespace

int main() {
  InitWindow(kWidth, kHeight, "Emberdeep: The Hollow Crown");
  SetTargetFPS(60);
  // Escape is handled by the account form and should not close the client.
  SetExitKey(KEY_NULL);

  AppState app;
  initializeAccount(app);
  if (app.needsAccountChoice || app.accountForm) {
    app.noticeLifetime = 0.0F;
  }
  loadSave(app);
  loadCloudSave(app);
  for (std::size_t i = 0; i < app.layers.size(); ++i) {
    if (app.layers[i].id == app.game.player.currentLayer &&
        ed::canUnlockLayer(app.game, app.layers[i])) {
      app.layerIndex = static_cast<int>(i);
      break;
    }
  }
  app.game.player.currentLayer = app.layers[static_cast<std::size_t>(app.layerIndex)].id;
  spawnBlock(app);
  startSaveWorker(app);

  while (!WindowShouldClose()) {
    const float delta = GetFrameTime();
    if (app.needsAccountChoice || app.accountForm) {
      handleAccountInput(app);
    } else {
      handleInput(app);
    }
    updateEffects(app, delta);

    BeginDrawing();
    ClearBackground(Color{20, 19, 29, 255});
    const float offsetX = app.shake > 0.0F ? static_cast<float>(GetRandomValue(-3, 3)) : 0.0F;
    const float offsetY = app.shake > 0.0F ? static_cast<float>(GetRandomValue(-3, 3)) : 0.0F;
    BeginMode2D(Camera2D{{0.0F, 0.0F}, {offsetX, offsetY}, 0.0F, 1.0F});
    if (app.needsAccountChoice) {
      drawAccountPrompt(app);
    } else if (app.accountForm) {
      drawAuthForm(app);
    } else {
      drawTopBar(app);
      switch (app.screen) {
        case Screen::Mine: drawMineScreen(app); break;
        case Screen::Inventory: drawInventoryScreen(app); break;
        case Screen::Crafting: drawCraftingScreen(app); break;
        case Screen::Campaign: drawCampaignScreen(app); break;
        case Screen::Dungeon: drawDungeonScreen(app); break;
        case Screen::Profile: drawProfileScreen(app); break;
        case Screen::GemHollow: drawGemHollowScreen(app); break;
        case Screen::Enchanting: drawEnchantingScreen(app); break;
      }
      drawNavigation(app);
    }
    for (const auto& particle : app.particles) {
      DrawCircleV(particle.position, 4.0F, Fade(particle.color, particle.lifetime * 2.0F));
    }
    for (const auto& text : app.floatingText) {
      DrawText(text.text.c_str(), static_cast<int>(text.position.x), static_cast<int>(text.position.y),
               22, Fade(text.color, text.lifetime));
    }
    EndMode2D();
    if (app.noticeLifetime > 0.0F) {
      DrawRectangle(300, 625, 680, 30, Color{35, 34, 45, 230});
      drawTextCentered(app.notice, 632, 16, Color{234, 229, 202, 255});
    }
    if (app.screen == Screen::Inventory && !app.needsAccountChoice && !app.accountForm) {
      DrawRectangle(0, 685, 1280, 35, Color{28, 27, 39, 200});
      DrawText(("Gear " + std::to_string(app.selectedGear + 1) + "/" +
                std::to_string(app.inventory.gear.size()) + "   Scroll: W/S").c_str(),
               70, 695, 14, Color{174, 174, 187, 255});
      DrawText("A/D select   U upgrade   E equip   G insert gem   R remove gem   N enchant", 360, 695, 14,
               Color{174, 174, 187, 255});
    }
    EndDrawing();

  }

  stopSaveWorker(app);
  CloseWindow();
  return 0;
}
