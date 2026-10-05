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
  int selectedGear = 0;
  int campaignScore = 0;
  int campaignHeartsLost = 0;
  bool campaignAttempt = false;
  float blockPulse = 0.0F;
  float shake = 0.0F;
  std::string notice = "Click the block to mine.";
  float noticeLifetime = 4.0F;
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
  DrawText("1 Mine   2 Inventory   3 Crafting   4 Campaign   5 Dungeon   6 Profile   F5 Save", 26, 55, 15,
           Color{156, 156, 173, 255});
  DrawRectangleRounded({1090, 14, 160, 34}, 0.2F, 8, Color{82, 52, 53, 255});
  DrawText("SIGN OUT", 1120, 24, 15, Color{255, 210, 210, 255});
}

void drawNavigation(const AppState& app) {
  DrawRectangle(0, 665, kWidth, 55, Color{28, 27, 39, 255});
  for (int i = 0; i < 6; ++i) {
    const int x = 18 + i * 210;
    const Screen screen = static_cast<Screen>(i);
    const bool active = app.screen == screen;
    DrawRectangleRounded({static_cast<float>(x), 675.0F, 170.0F, 34.0F}, 0.25F, 8,
                         active ? Color{112, 76, 45, 255} : Color{48, 47, 61, 255});
    DrawText(screenName(screen), x + 24, 684, 15, active ? RAYWHITE : Color{170, 170, 180, 255});
  }
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
  DrawRectangleRounded({static_cast<float>(blockX), static_cast<float>(blockY),
                        static_cast<float>(blockSize), static_cast<float>(blockSize)},
                       0.14F, 12, blockColor);
  DrawRectangleLinesEx({static_cast<float>(blockX), static_cast<float>(blockY),
                        static_cast<float>(blockSize), static_cast<float>(blockSize)},
                       5.0F, Color{239, 225, 189, 255});
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
  drawTextCentered("CAMPAIGN", 108, 34, Color{224, 187, 106, 255});
  DrawText("A / D select. ENTER starts. SPACE/click builds score. BACKSPACE submits.", 30, 155, 17,
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
  DrawText(("Current score: " + std::to_string(app.campaignScore)).c_str(), 300, 410, 23,
           Color{122, 217, 255, 255});
  DrawText(app.campaignAttempt ? "ATTEMPT ACTIVE" : "Press ENTER to begin", 300, 452, 18,
           app.campaignAttempt ? Color{116, 235, 146, 255} : Color{178, 178, 190, 255});
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

void drawInventoryScreen(const AppState& app) {
  drawTextCentered("INVENTORY", 108, 34, Color{224, 187, 106, 255});
  DrawText("Resources", 70, 164, 24, Color{235, 224, 191, 255});
  int y = 205;
  const auto drawBucket = [&](const char* title, const std::map<std::string, int>& bucket) {
    DrawText(title, 70, y, 18, Color{174, 174, 187, 255});
    int x = 250;
    for (const auto& [id, count] : bucket) {
      DrawRectangleRounded({static_cast<float>(x), static_cast<float>(y - 5), 145, 34}, 0.18F, 6,
                           Color{52, 52, 67, 255});
      DrawText((id + " x" + std::to_string(count)).c_str(), x + 8, y + 4, 14, RAYWHITE);
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
  DrawText("Gear", 70, y, 24, Color{235, 224, 191, 255});
  y += 42;
  if (app.inventory.gear.empty()) {
    DrawText("No gear crafted yet. Mine ore, smelt it, then visit Crafting.", 70, y, 18,
             Color{174, 174, 187, 255});
  } else {
    for (std::size_t i = 0; i < app.inventory.gear.size() && i < 6; ++i) {
      const auto& piece = app.inventory.gear[i];
      const bool selected = static_cast<int>(i) == app.selectedGear;
      DrawRectangleRounded({70.0F + static_cast<float>(i % 3) * 370.0F,
                            static_cast<float>(y + (i / 3) * 70), 330, 52},
                           0.12F, 6, selected ? Color{112, 76, 45, 255} : Color{52, 52, 67, 255});
      DrawText((piece.name + " +" + std::to_string(piece.upgradeLevel)).c_str(),
               84 + static_cast<int>(i % 3) * 370, y + static_cast<int>(i / 3) * 70 + 8, 17,
               RAYWHITE);
      DrawText(("Tier " + std::to_string(piece.tier) + " | " + piece.slot).c_str(),
               84 + static_cast<int>(i % 3) * 370, y + static_cast<int>(i / 3) * 70 + 30, 13,
               Color{180, 180, 192, 255});
    }
    DrawText("A / D select gear   U upgrade selected   E equip selected", 70, 570, 17,
             Color{174, 174, 187, 255});
  }
}

void drawCraftingScreen(const AppState& app) {
  drawTextCentered("CRAFTING", 108, 34, Color{224, 187, 106, 255});
  const auto& recipe = app.recipes[static_cast<std::size_t>(app.selectedRecipe)];
  DrawText("A / D select recipe   S smelt current ore   ENTER craft selected gear", 70, 155, 17,
           Color{174, 174, 187, 255});
  DrawRectangleRounded({220, 205, 840, 290}, 0.07F, 10, Color{53, 52, 65, 255});
  DrawText(recipe.itemName.c_str(), 280, 245, 32, RAYWHITE);
  DrawText(("Tier " + std::to_string(recipe.tier) + " " + recipe.slot).c_str(), 280, 290, 20,
           Color{204, 175, 111, 255});
  DrawText(("Cost: " + std::to_string(recipe.ingots) + " ingots  |  " +
            std::to_string(recipe.trophies) + " trophies  |  " +
            std::to_string(recipe.gold) + " gold")
               .c_str(),
           280, 335, 18, Color{190, 190, 202, 255});
  DrawText(recipe.requiresGem ? "Also requires any gem" : "No gem required", 280, 370, 18,
           recipe.requiresGem ? Color{129, 214, 241, 255} : Color{174, 174, 187, 255});
  const int oreCount = ed::countItem(app.inventory, "ore", app.blockSpec.materialId);
  const int ingotCount = ed::countItem(app.inventory, "ingots", materialIdForTier(recipe.tier));
  DrawText(("Current ore: " + std::to_string(oreCount) + "   Current ingots: " +
            std::to_string(ingotCount) + "   Dust: " +
            std::to_string(app.game.player.arcaneDust))
               .c_str(),
           280, 420, 18, Color{180, 210, 178, 255});
  DrawText("Smelting converts 4 ore into 1 ingot. Crafting adds gear to Inventory.", 280, 455, 16,
           Color{174, 174, 187, 255});
}

void updateEffects(AppState& app, float delta) {
  app.blockPulse = std::max(0.0F, app.blockPulse - delta);
  app.shake = std::max(0.0F, app.shake - delta);
  app.noticeLifetime = std::max(0.0F, app.noticeLifetime - delta);
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

void handleInput(AppState& app) {
  if (IsKeyPressed(KEY_F)) {
    signOut(app);
    return;
  }
  if (IsKeyPressed(KEY_F5)) {
    requestSave(app);
    app.lastAutosave = std::chrono::steady_clock::now();
    setNotice(app, "Game saved.");
  }
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    const Vector2 mouse = GetMousePosition();
    if (CheckCollisionPointRec(mouse, {1090, 14, 160, 34})) {
      signOut(app);
      return;
    }
    for (int i = 0; i < 6; ++i) {
      const Rectangle navigationButton = {
          18.0F + static_cast<float>(i * 210),
          675.0F,
          170.0F,
          34.0F,
      };
      if (CheckCollisionPointRec(mouse, navigationButton)) {
        app.screen = static_cast<Screen>(i);
        break;
      }
    }
  }

  if (IsKeyPressed(KEY_ONE)) app.screen = Screen::Mine;
  if (IsKeyPressed(KEY_TWO)) app.screen = Screen::Inventory;
  if (IsKeyPressed(KEY_THREE)) app.screen = Screen::Crafting;
  if (IsKeyPressed(KEY_FOUR)) app.screen = Screen::Campaign;
  if (IsKeyPressed(KEY_FIVE)) app.screen = Screen::Dungeon;
  if (IsKeyPressed(KEY_SIX)) app.screen = Screen::Profile;

  if (app.screen == Screen::Mine) {
    if (IsKeyPressed(KEY_Q)) {
      app.layerIndex = std::max(0, app.layerIndex - 1);
      app.game.player.currentLayer = app.layers[static_cast<std::size_t>(app.layerIndex)].id;
      spawnBlock(app);
    }
    if (IsKeyPressed(KEY_E)) {
      app.layerIndex = std::min(static_cast<int>(app.layers.size()) - 1, app.layerIndex + 1);
      app.game.player.currentLayer = app.layers[static_cast<std::size_t>(app.layerIndex)].id;
      spawnBlock(app);
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
    if (!app.inventory.gear.empty() && IsKeyPressed(KEY_U)) {
      auto& piece = app.inventory.gear[static_cast<std::size_t>(app.selectedGear)];
      setNotice(app, ed::upgradeGear(app.game, piece) ? "Gear upgraded." : "Upgrade unavailable.");
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
  } else if (app.screen == Screen::Crafting) {
    if (IsKeyPressed(KEY_A)) {
      app.selectedRecipe = std::max(0, app.selectedRecipe - 1);
    }
    if (IsKeyPressed(KEY_D)) {
      app.selectedRecipe = std::min(static_cast<int>(app.recipes.size()) - 1,
                                    app.selectedRecipe + 1);
    }
    const auto& recipe = app.recipes[static_cast<std::size_t>(app.selectedRecipe)];
    if (IsKeyPressed(KEY_S)) {
      const int tier = std::clamp(recipe.tier, 1, 10);
      setNotice(app, ed::smeltOre(app.inventory, materialIdForTier(tier), 1)
                          ? "Smelted 4 ore into 1 ingot."
                          : "Need 4 matching ore.");
    }
    if (IsKeyPressed(KEY_ENTER)) {
      setNotice(app, ed::craftGear(app.game, app.inventory, recipe)
                          ? "Gear crafted. Check Inventory."
                          : "Crafting requirements not met.");
    }
  } else if (app.screen == Screen::Campaign) {
    if (IsKeyPressed(KEY_A)) {
      app.selectedBoss = std::max(0, app.selectedBoss - 1);
      app.campaignScore = 0;
      app.campaignAttempt = false;
    }
    if (IsKeyPressed(KEY_D)) {
      app.selectedBoss = std::min(static_cast<int>(app.bosses.size()) - 1, app.selectedBoss + 1);
      app.campaignScore = 0;
      app.campaignAttempt = false;
    }
    if (IsKeyPressed(KEY_ENTER)) {
      app.campaignAttempt = true;
      app.campaignScore = 0;
      app.campaignHeartsLost = 0;
    }
    if (app.campaignAttempt && (IsKeyPressed(KEY_SPACE) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {
      ++app.campaignScore;
    }
    if (IsKeyPressed(KEY_BACKSPACE) && app.campaignAttempt) attemptCampaign(app);
  } else if (app.screen == Screen::Dungeon) {
    if (IsKeyPressed(KEY_A)) app.selectedZone = std::max(0, app.selectedZone - 1);
    if (IsKeyPressed(KEY_D)) {
      app.selectedZone = std::min(static_cast<int>(app.zones.size()) - 1, app.selectedZone + 1);
    }
    if (IsKeyPressed(KEY_ENTER)) runDungeon(app);
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
    if (app.layers[i].id == app.game.player.currentLayer) {
      app.layerIndex = static_cast<int>(i);
      break;
    }
  }
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
    EndDrawing();
  }

  stopSaveWorker(app);
  CloseWindow();
  return 0;
}
