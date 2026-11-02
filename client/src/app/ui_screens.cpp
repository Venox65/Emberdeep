#include "ui_screens.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "campaign_minigames.hpp"
#include "game_actions.hpp"
#include "ed/postgame.hpp"

namespace ed::app {

namespace {

std::map<std::string, Texture2D> g_textures;

std::string getCurrentDateString() {
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm* tm = std::localtime(&t);
  char buf[16];
  std::strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
  return std::string(buf);
}

} // namespace

Texture2D getTexture(const std::string& path) {
  if (g_textures.find(path) == g_textures.end()) {
    g_textures[path] = LoadTexture(path.c_str());
  }
  return g_textures[path];
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
  DrawText(("XP " + ed::formatCount(app.game.player.xp)).c_str(), 118, 28, 16, RAYWHITE);
  DrawText(("Gold " + ed::formatCount(app.game.player.gold)).c_str(), 390, 25, 21,
           Color{248, 211, 89, 255});
  DrawText(("Essence " + ed::formatCount(app.game.player.bossEssence)).c_str(), 540, 28, 17,
           Color{232, 145, 236, 255});
  DrawText(("Ember " + ed::formatCount(app.game.player.emberMarks)).c_str(), 690, 28, 17,
           Color{255, 139, 65, 255});
  DrawText(("Cores " + ed::formatCount(app.game.player.chronoCores)).c_str(), 840, 28, 17,
           Color{120, 230, 250, 255});
  DrawText("1 Mine   2 Inventory   3 Crafting   4 Campaign   5 Dungeon   6 Gem Hollow   7 Enchant   8 Profile   9 Settings   F5 Save", 16, 55, 14,
           Color{156, 156, 173, 255});
  DrawRectangleRounded({1010, 14, 72, 34}, 0.2F, 8, Color{52, 76, 67, 255});
  DrawText("SAVE", 1026, 24, 15, Color{210, 255, 220, 255});

  const Rectangle selectSaveBtn{1090, 14, 160, 34};
  const Vector2 mouse = GetMousePosition();
  const bool hoverSelect = CheckCollisionPointRec(mouse, selectSaveBtn);
  DrawRectangleRounded(selectSaveBtn, 0.2F, 8,
                       hoverSelect ? Color{55, 65, 90, 255} : Color{40, 48, 68, 255});
  DrawRectangleRoundedLines(selectSaveBtn, 0.2F, 8,
                            hoverSelect ? Color{130, 175, 240, 255} : Color{80, 98, 135, 255});
  const char* label = "SELECT SAVE";
  const int labelW = MeasureText(label, 15);
  DrawText(label, 1090 + (160 - labelW) / 2, 24, 15, Color{220, 235, 255, 255});
}

void drawNavigation(const AppState& app) {
  if (app.sidebarOpen) {
    DrawRectangle(0, 0, 200, 720, Color{28, 27, 39, 240});
    DrawText("MENU", 20, 20, 20, RAYWHITE);
    DrawRectangleRounded({150, 15, 30, 30}, 0.2F, 4, Color{200, 50, 50, 255});
    DrawText("X", 158, 20, 20, RAYWHITE);

    const Screen navScreens[] = {Screen::Mine, Screen::Inventory, Screen::Crafting, Screen::Campaign, Screen::Dungeon, Screen::GemHollow, Screen::Enchanting, Screen::Profile, Screen::Settings, Screen::DescentHub};
    for (int i = 0; i < 10; ++i) {
      const int y = 70 + i * 46;
      const Screen screen = navScreens[i];
      const bool active = app.screen == screen;
      DrawRectangleRounded({10, static_cast<float>(y), 180, 38}, 0.2F, 4, active ? Color{112, 76, 45, 255} : Color{48, 47, 61, 255});
      DrawText(screenName(screen), 20, y + 10, 15, active ? RAYWHITE : Color{170, 170, 180, 255});
    }
  } else {
    DrawRectangleRounded({1220, 100, 40, 40}, 0.2F, 4, Color{48, 47, 61, 200});
    DrawRectangle(1228, 110, 24, 4, RAYWHITE);
    DrawRectangle(1228, 118, 24, 4, RAYWHITE);
    DrawRectangle(1228, 126, 24, 4, RAYWHITE);
  }
}

void drawLoadingScreen(const AppState& app) {
  DrawRectangle(0, 0, kWidth, kHeight, Color{18, 17, 26, 255});

  // Game Title
  drawTextCentered("EMBERDEEP", 240, 52, Color{248, 211, 89, 255});
  drawTextCentered("THE HOLLOW CROWN", 305, 22, Color{210, 170, 95, 255});

  // Loading indicator
  const float progress = std::clamp(app.loadingTimer / 1.0F, 0.0F, 1.0F);
  drawTextCentered("Descending into the depths...", 380, 16, Color{175, 175, 195, 255});

  const Rectangle barRec{440, 412, 400, 12};
  DrawRectangleRounded(barRec, 0.5F, 4, Color{36, 34, 48, 255});
  if (progress > 0.0F) {
    DrawRectangleRounded({440, 412, 400.0F * progress, 12}, 0.5F, 4, Color{235, 155, 55, 255});
  }
  DrawRectangleRoundedLines(barRec, 0.5F, 4, Color{80, 75, 100, 255});

  drawTextCentered("Loading resources & save profiles...", 440, 14, Color{130, 130, 145, 255});
  drawTextCentered("Press SPACE or Click to skip", 660, 13, Color{100, 100, 115, 255});
}

void drawTitleScreen(const AppState& app) {
  DrawRectangle(0, 0, kWidth, kHeight, Color{18, 17, 26, 255});

  // Ambient glow
  DrawCircleGradient({640.0F, 220.0F}, 340.0F, Color{45, 35, 60, 160}, Color{18, 17, 26, 0});

  // Game Title
  drawTextCentered("EMBERDEEP", 160, 56, Color{248, 211, 89, 255});
  drawTextCentered("THE HOLLOW CROWN", 230, 24, Color{218, 175, 100, 255});

  DrawLine(440, 276, 840, 276, Color{90, 76, 55, 255});

  // Active Save Badge
  std::string saveName = app.savePath.filename().string();
  std::string activeInfo = "Active Profile: " + saveName +
                           "  (Lv. " + std::to_string(app.game.player.level) +
                           "  |  Echo Rank " + std::to_string(app.game.player.echoRank) +
                           "  |  Depth " + std::to_string(app.game.player.deepestDepth) + ")";
  drawTextCentered(activeInfo, 296, 15, Color{165, 185, 215, 255});

  const Vector2 mouse = GetMousePosition();

  // Big Button: ENTER GAME
  const Rectangle enterBtn{450, 350, 380, 68};
  const bool hoverEnter = CheckCollisionPointRec(mouse, enterBtn);
  DrawRectangleRounded(enterBtn, 0.25F, 6, hoverEnter ? Color{195, 115, 30, 255} : Color{150, 85, 20, 255});
  DrawRectangleRoundedLines(enterBtn, 0.25F, 6, hoverEnter ? Color{255, 230, 140, 255} : Color{235, 185, 75, 255});
  const char* enterText = "ENTER GAME";
  const int enterW = MeasureText(enterText, 26);
  DrawText(enterText, 450 + (380 - enterW) / 2, 368, 26, RAYWHITE);

  // Smaller Button below: CHOOSE SAVE
  const Rectangle saveBtn{490, 442, 300, 44};
  const bool hoverSave = CheckCollisionPointRec(mouse, saveBtn);
  DrawRectangleRounded(saveBtn, 0.25F, 6, hoverSave ? Color{55, 68, 98, 255} : Color{38, 48, 70, 255});
  DrawRectangleRoundedLines(saveBtn, 0.25F, 6, hoverSave ? Color{140, 185, 250, 255} : Color{85, 115, 165, 255});
  const char* saveText = "CHOOSE SAVE";
  const int saveW = MeasureText(saveText, 17);
  DrawText(saveText, 490 + (300 - saveW) / 2, 454, 17, Color{225, 238, 255, 255});

  // Footer status note
  drawTextCentered("Press ENTER to delve  |  Offline Single-Player Campaign", 655, 14, Color{125, 125, 140, 255});
}

void drawMineScreen(const AppState& app) {
  drawTextCentered("THE EMBERDEEP", 106, 32, Color{224, 187, 106, 255});
  drawTextCentered("Layer: " + app.layers[static_cast<std::size_t>(app.layerIndex)].name, 140, 18,
                   Color{190, 190, 205, 255});
  DrawText("Q / E change layer", 28, 116, 16, Color{140, 140, 153, 255});
  if (app.game.player.autoClickerLevel >= 1) {
    const double speed = ed::autoClickerSpeed(app.game.player.autoClickerLevel);
    char buf[64];
    if (!app.game.player.autoClickerEnabled) {
      std::snprintf(buf, sizeof(buf), "Auto-Clicker: %.1f/s [OFF]", speed);
      DrawText(buf, 1010, 116, 15, Color{160, 160, 175, 255});
    } else if (app.activeMiningTimer > 0.0F) {
      const int secLeft = static_cast<int>(std::ceil(app.activeMiningTimer));
      std::snprintf(buf, sizeof(buf), "Auto-Clicker: PAUSED (%ds)", secLeft);
      DrawText(buf, 970, 116, 15, Color{245, 195, 90, 255});
    } else {
      std::snprintf(buf, sizeof(buf), "Auto-Clicker: %.1f/s [ACTIVE]", speed);
      DrawText(buf, 995, 116, 15, Color{120, 230, 250, 255});
    }
  }

  const bool isLayer10 = (app.layers[static_cast<std::size_t>(app.layerIndex)].id == "aether-vault");
  const bool strataUnlocked = (isLayer10 && ed::isDeepStrataUnlocked(app.game));

  if (strataUnlocked) {
    DrawText("[ / ] adjust depth   |   T auto-advance", 28, 138, 14, Color{124, 247, 224, 210});

    const Vector2 mouse = GetMousePosition();
    const Rectangle minusBtn{410, 164, 34, 22};
    const Rectangle plusBtn{835, 164, 34, 22};
    const Rectangle autoBtn{885, 164, 95, 22};

    const int maxDepth = std::max(1, app.game.player.deepestDepth + 1);
    const bool canDec = (app.deepStrataDepth > 0);
    const bool canInc = (app.deepStrataDepth < maxDepth);

    const bool hoverMinus = CheckCollisionPointRec(mouse, minusBtn);
    DrawRectangleRounded(minusBtn, 0.25F, 4, canDec ? (hoverMinus ? Color{70, 78, 100, 255} : Color{50, 54, 70, 255}) : Color{34, 34, 44, 255});
    DrawText("-", 423, 166, 18, canDec ? RAYWHITE : Color{110, 110, 125, 255});

    const bool hoverPlus = CheckCollisionPointRec(mouse, plusBtn);
    DrawRectangleRounded(plusBtn, 0.25F, 4, canInc ? (hoverPlus ? Color{70, 78, 100, 255} : Color{50, 54, 70, 255}) : Color{34, 34, 44, 255});
    DrawText("+", 847, 166, 18, canInc ? RAYWHITE : Color{110, 110, 125, 255});

    const bool hoverAuto = CheckCollisionPointRec(mouse, autoBtn);
    DrawRectangleRounded(autoBtn, 0.25F, 4, app.deepStrataAutoAdvance ? (hoverAuto ? Color{55, 95, 75, 255} : Color{40, 75, 58, 255}) : (hoverAuto ? Color{65, 65, 78, 255} : Color{48, 48, 58, 255}));
    DrawText(app.deepStrataAutoAdvance ? "Auto: ON" : "Auto: OFF", 897, 168, 14, app.deepStrataAutoAdvance ? Color{160, 255, 200, 255} : Color{180, 180, 195, 255});

    if (app.deepStrataDepth == 0) {
      drawTextCentered("Standard Vault (Depth 0)", 167, 16, Color{190, 190, 205, 255});
    } else {
      drawTextCentered("DEEP STRATA: DEPTH " + std::to_string(app.deepStrataDepth), 166, 17, Color{124, 247, 224, 255});
    }
  }

  const float hpRatio = static_cast<float>(app.block.currentHp) /
                        static_cast<float>(std::max<std::int64_t>(1, app.block.maxHp));

  const float pulse = 1.0F + app.blockPulse * 0.45F;
  const int blockSize = static_cast<int>(220.0F * pulse);
  const int blockX = 640 - blockSize / 2;
  const int blockY = 335 - blockSize / 2;
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
  DrawCircle(590, 332, 13, Color{255, 255, 255, 130});
  DrawCircle(690, 382, 9, Color{255, 255, 255, 100});

  if (app.weakSpotActive) {
    const float time = static_cast<float>(GetTime());
    const float pulseW = 1.0F + 0.15F * std::sin(time * 10.0F);
    const float radius = 22.0F * pulseW;
    DrawCircleV(app.weakSpotPos, radius + 8.0F, Color{255, 180, 40, 70});
    DrawCircleV(app.weakSpotPos, radius + 4.0F, Color{255, 215, 60, 130});
    DrawCircleV(app.weakSpotPos, radius, Color{255, 240, 140, 210});
    DrawCircleV(app.weakSpotPos, radius * 0.45F, Color{255, 255, 255, 255});
    DrawCircleLines(static_cast<int>(app.weakSpotPos.x), static_cast<int>(app.weakSpotPos.y), radius, Color{255, 130, 20, 255});
    DrawCircleLines(static_cast<int>(app.weakSpotPos.x), static_cast<int>(app.weakSpotPos.y), radius * 0.7F, Color{255, 230, 90, 255});
    DrawLine(static_cast<int>(app.weakSpotPos.x - radius - 6), static_cast<int>(app.weakSpotPos.y),
             static_cast<int>(app.weakSpotPos.x + radius + 6), static_cast<int>(app.weakSpotPos.y), Color{255, 235, 100, 220});
    DrawLine(static_cast<int>(app.weakSpotPos.x), static_cast<int>(app.weakSpotPos.y - radius - 6),
             static_cast<int>(app.weakSpotPos.x), static_cast<int>(app.weakSpotPos.y + radius + 6), Color{255, 235, 100, 220});
    DrawText("WEAK SPOT", static_cast<int>(app.weakSpotPos.x - 38), static_cast<int>(app.weakSpotPos.y - radius - 18), 12, Color{255, 230, 100, 255});
  }

  // Healthbar drawn clearly above the block (unobstructed by block and on top)
  DrawRectangleRounded({320, 194, 640, 24}, 0.25F, 4, Color{45, 40, 52, 255});
  DrawRectangleRounded({320, 194, std::max(0.0F, 640.0F * hpRatio), 24}, 0.25F, 4, Color{195, 70, 60, 255});
  DrawRectangleRoundedLines({320, 194, 640, 24}, 0.25F, 4, Color{90, 80, 105, 255});
  drawTextCentered(ed::formatCount(app.block.currentHp) + " / " + ed::formatCount(app.block.maxHp),
                   198, 16, RAYWHITE);

  const int textStartY = blockY + blockSize + 10;
  drawTextCentered(app.blockSpec.materialName, textStartY, 22, RAYWHITE);
  drawTextCentered(rarityName(app.blockSpec.rarity), textStartY + 24, 15, Color{255, 210, 106, 255});
  drawTextCentered("Hardness " + std::to_string(app.block.hardness) + "   XP " + ed::formatCount(app.block.xp),
                   textStartY + 45, 16, Color{179, 179, 193, 255});

  if (strataUnlocked) {
    if (app.blockSpec.depth > 0) {
      const float shardChancePct = static_cast<float>(app.blockSpec.aetherShardChanceBasisPoints) / 100.0F;
      char chanceBuf[32];
      std::snprintf(chanceBuf, sizeof(chanceBuf), "%.2f%%", shardChancePct);
      std::string depthInfo = "Shard Chance: " + std::string(chanceBuf) +
                              "   |   Aetherium: x" + std::to_string(app.blockSpec.dropMultiplier) +
                              "   |   Shards: " + ed::formatCount(app.game.player.aetherShards) +
                              "   |   Best: Depth " + std::to_string(app.game.player.deepestDepth);
      drawTextCentered(depthInfo, textStartY + 68, 15, Color{124, 247, 224, 255});
    } else {
      std::string depthInfo = "Deep Strata Unlocked! Press ] or click [+] to descend  (Best: Depth " +
                              std::to_string(app.game.player.deepestDepth) +
                              "   |   Shards: " + ed::formatCount(app.game.player.aetherShards) + ")";
      drawTextCentered(depthInfo, textStartY + 68, 15, Color{160, 220, 210, 255});
    }
    drawTextCentered("Click the block or press SPACE to mine", textStartY + 92, 18, Color{235, 232, 213, 255});
  } else {
    drawTextCentered("Click the block or press SPACE to mine", textStartY + 72, 18, Color{235, 232, 213, 255});
  }
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

  if (app.inBossCombat) {
    if (app.bossCombatShowResults) {
      drawTextCentered("BOSS COMBAT RESULTS", 108, 34, Color{224, 187, 106, 255});
      DrawRectangleRounded({310, 180, 660, 360}, 0.08F, 10, Color{45, 42, 58, 255});
      DrawRectangleRoundedLines({310, 180, 660, 360}, 0.08F, 10,
                                app.bossCombatResult.victory
                                    ? (app.bossCombatIsEcho ? Color{218, 160, 255, 255} : Color{245, 215, 110, 255})
                                    : Color{220, 80, 80, 255});

      if (app.bossCombatResult.victory) {
        std::string winTitle = app.bossCombatIsEcho ? "ECHO BANISHED!" : "VICTORY! BOSS VANQUISHED!";
        drawTextCentered(winTitle, 205, 28, app.bossCombatIsEcho ? Color{235, 190, 255, 255} : Color{122, 235, 175, 255});
        std::string starStr = "Minigame Performance: " + std::string(app.bossCombatStars, '*') +
                              " (" + std::to_string(app.bossCombatStars) + " Star" + (app.bossCombatStars > 1 ? "s)" : ")");
        drawTextCentered(starStr, 245, 18, Color{255, 225, 130, 255});

        int lootY = 285;
        DrawText("Combat Spoils:", 350, lootY, 20, RAYWHITE);
        lootY += 32;
        if (app.bossCombatRewardGold > 0) {
          DrawText(("+ " + ed::formatCount(app.bossCombatRewardGold) + " Gold").c_str(), 370, lootY, 18, Color{248, 211, 89, 255});
          lootY += 26;
        }
        if (app.bossCombatRewardXp > 0) {
          DrawText(("+ " + ed::formatCount(app.bossCombatRewardXp) + " XP").c_str(), 370, lootY, 18, Color{130, 215, 255, 255});
          lootY += 26;
        }
        if (app.bossCombatRewardSigils > 0) {
          DrawText(("+ " + ed::formatCount(app.bossCombatRewardSigils) + " Echo Sigils").c_str(), 370, lootY, 18, Color{218, 160, 255, 255});
          lootY += 26;
        }
        if (!app.bossCombatRewardLegendary.empty()) {
          DrawText(("+ Legendary Unlocked: " + app.bossCombatRewardLegendary).c_str(), 370, lootY, 18, Color{255, 175, 80, 255});
          lootY += 26;
        }
      } else {
        drawTextCentered("DEFEATED IN COMBAT", 205, 28, Color{245, 80, 80, 255});
        std::string fallMsg = "You were struck down by " + app.bossCombatBossName + ".";
        drawTextCentered(fallMsg, 245, 18, Color{220, 200, 200, 255});

        drawTextCentered(("Boss HP Remaining: " + ed::formatCount(app.bossCombatBossHp) + " / " +
                          ed::formatCount(app.bossCombatBossMaxHp)).c_str(), 290, 19, Color{255, 140, 140, 255});

        drawTextCentered("Your current gear & stats were insufficient to survive.", 340, 17, Color{190, 185, 200, 255});
        drawTextCentered("Tip: Upgrade weapons & armor (+11+ Mythic), socket gems, or get 3 stars!", 370, 15, Color{235, 215, 150, 255});
      }

      const Rectangle contBtn{490, 470, 300, 44};
      DrawRectangleRounded(contBtn, 0.2F, 4, Color{60, 58, 78, 255});
      DrawRectangleRoundedLines(contBtn, 0.2F, 4, Color{180, 175, 205, 255});
      drawTextCentered("CONTINUE [SPACE / ENTER]", 482, 18, WHITE);
      return;
    }

    const std::string arenaTitle = app.bossCombatIsEcho
        ? "ECHO BOSS COMBAT: " + app.bossCombatBossName
        : "BOSS COMBAT: " + app.bossCombatBossName;
    drawTextCentered(arenaTitle, 108, 30, app.bossCombatIsEcho ? Color{225, 170, 255, 255} : Color{245, 215, 110, 255});

    std::string starBonusDesc;
    if (app.bossCombatStars >= 3) {
      starBonusDesc = "Minigame Mastery: +30% Combat Stats | Boss HP -15%";
    } else if (app.bossCombatStars >= 2) {
      starBonusDesc = "Minigame Precision: +15% Combat Stats | Boss HP -8%";
    } else {
      starBonusDesc = "Minigame Cleared: Standard Combat Parameters";
    }
    drawTextCentered(starBonusDesc, 145, 15, Color{210, 200, 160, 255});

    // PLAYER CARD (Left)
    DrawRectangleRounded({120, 180, 360, 250}, 0.08F, 8, Color{42, 45, 62, 255});
    DrawRectangleRoundedLines({120, 180, 360, 250}, 0.08F, 8, Color{85, 110, 165, 255});
    DrawText("CHAMPION", 150, 198, 22, Color{235, 235, 245, 255});
    DrawText(("Level " + std::to_string(app.game.player.level)).c_str(), 390, 202, 16, Color{180, 180, 200, 255});

    const float playerHpPct = std::clamp(static_cast<float>(app.bossCombatPlayerHp) /
                                         static_cast<float>(std::max<std::int64_t>(1, app.bossCombatPlayerMaxHp)), 0.0F, 1.0F);
    DrawRectangleRounded({150, 235, 300, 22}, 0.3F, 4, Color{30, 25, 35, 255});
    DrawRectangleRounded({150, 235, 300.0F * playerHpPct, 22}, 0.3F, 4, Color{70, 190, 95, 255});
    std::string playerHpText = ed::formatCount(app.bossCombatPlayerHp) + " / " + ed::formatCount(app.bossCombatPlayerMaxHp);
    DrawText(playerHpText.c_str(), 160, 238, 15, WHITE);

    const auto pStats = ed::calculatePlayerStats(app.game, app.inventory);
    DrawText(("Attack: " + std::to_string(pStats.attack)).c_str(), 150, 275, 17, Color{245, 165, 120, 255});
    DrawText(("Defense: " + std::to_string(pStats.defense)).c_str(), 150, 305, 17, Color{130, 200, 245, 255});
    DrawText(("Speed: " + std::to_string(pStats.speed)).c_str(), 150, 335, 17, Color{180, 235, 160, 255});
    DrawText(("Crit: " + std::to_string(pStats.critChanceBasisPoints / 100) + "%").c_str(), 310, 275, 17, Color{245, 225, 120, 255});
    DrawText(("Pierce: " + std::to_string(pStats.pierce)).c_str(), 310, 305, 17, Color{215, 165, 245, 255});

    // BOSS CARD (Right)
    DrawRectangleRounded({800, 180, 360, 250}, 0.08F, 8, app.bossCombatIsEcho ? Color{52, 36, 68, 255} : Color{58, 38, 42, 255});
    DrawRectangleRoundedLines({800, 180, 360, 250}, 0.08F, 8, app.bossCombatIsEcho ? Color{195, 120, 255, 255} : Color{205, 90, 90, 255});
    DrawText(app.bossCombatBossName.c_str(), 830, 198, 20, app.bossCombatIsEcho ? Color{245, 210, 255, 255} : Color{255, 220, 220, 255});

    const float bossHpPct = std::clamp(static_cast<float>(app.bossCombatBossHp) /
                                       static_cast<float>(std::max<std::int64_t>(1, app.bossCombatBossMaxHp)), 0.0F, 1.0F);
    DrawRectangleRounded({830, 235, 300, 22}, 0.3F, 4, Color{30, 25, 35, 255});
    DrawRectangleRounded({830, 235, 300.0F * bossHpPct, 22}, 0.3F, 4, Color{225, 65, 75, 255});
    std::string bossHpText = ed::formatCount(app.bossCombatBossHp) + " / " + ed::formatCount(app.bossCombatBossMaxHp);
    DrawText(bossHpText.c_str(), 840, 238, 15, WHITE);

    DrawText(("Special: [" + app.bossCombatBossAbility + "]").c_str(), 830, 280, 17, Color{245, 185, 130, 255});
    DrawText("Unleashes 150% damage every 3 turns", 830, 310, 14, Color{190, 180, 190, 255});
    if (app.bossCombatIsEcho) {
      DrawText(("Empowered by Echo Rank " + std::to_string(app.bossCombatEchoRank)).c_str(), 830, 340, 15, Color{218, 160, 255, 255});
    }

    drawTextCentered("VS", 270, 36, Color{255, 215, 120, 255});

    // BATTLE LOG (Bottom Center)
    DrawRectangleRounded({200, 455, 880, 165}, 0.08F, 8, Color{24, 25, 36, 230});
    DrawRectangleRoundedLines({200, 455, 880, 165}, 0.08F, 8, Color{70, 72, 95, 255});

    int logY = 466;
    int startLog = std::max(0, static_cast<int>(app.bossBattleLog.size()) - 5);
    for (std::size_t i = startLog; i < app.bossBattleLog.size(); ++i) {
      const auto& line = app.bossBattleLog[i];
      Color lineCol = RAYWHITE;
      if (line.find("CRITICAL") != std::string::npos) lineCol = Color{255, 225, 90, 255};
      else if (line.find("unleashes") != std::string::npos || line.find("devastating") != std::string::npos) lineCol = Color{255, 135, 135, 255};
      else if (line.find("vanquished") != std::string::npos || line.find("banished") != std::string::npos) lineCol = Color{130, 245, 170, 255};
      else if (line.find("struck down") != std::string::npos) lineCol = Color{245, 100, 100, 255};
      else if (line.find("Mastery") != std::string::npos) lineCol = Color{218, 160, 255, 255};
      DrawText(line.c_str(), 230, logY, 17, lineCol);
      logY += 27;
    }

    DrawText("HOLD SPACE or ENTER to fast-forward  |  ESC to forfeit", 430, 634, 16, Color{170, 170, 190, 255});
    return;
  }

  const bool echoUnlocked = ed::isEchoCampaignUnlocked(app.game);
  if (app.echoCampaignMode) {
    drawTextCentered(("ECHO CAMPAIGN — RANK " + std::to_string(std::max(1, app.game.player.echoRank))).c_str(), 108, 34, Color{218, 160, 255, 255});
  } else {
    drawTextCentered("CAMPAIGN", 108, 34, Color{224, 187, 106, 255});
  }

  if (app.game.player.echoSigils > 0 || echoUnlocked) {
    DrawText(("Echo Sigils: " + ed::formatCount(app.game.player.echoSigils)).c_str(), 1020, 108, 18, Color{218, 160, 255, 255});
  }

  if (echoUnlocked && !app.campaignAttempt) {
    DrawText("A / D select. ENTER starts. E / TAB toggles mode. BACKSPACE gives up.", 30, 135, 15,
             Color{174, 174, 187, 255});
    // Mode toggle tabs
    const Rectangle stdTab{400, 155, 220, 36};
    const Rectangle echoTab{660, 155, 220, 36};
    DrawRectangleRounded(stdTab, 0.2F, 4, !app.echoCampaignMode ? Color{112, 76, 45, 255} : Color{50, 48, 62, 255});
    DrawRectangleRoundedLines(stdTab, 0.2F, 4, !app.echoCampaignMode ? Color{224, 187, 106, 255} : Color{80, 78, 95, 255});
    DrawText("STANDARD", 460, 165, 16, !app.echoCampaignMode ? WHITE : Color{180, 180, 190, 255});

    DrawRectangleRounded(echoTab, 0.2F, 4, app.echoCampaignMode ? Color{90, 45, 120, 255} : Color{50, 48, 62, 255});
    DrawRectangleRoundedLines(echoTab, 0.2F, 4, app.echoCampaignMode ? Color{218, 160, 255, 255} : Color{80, 78, 95, 255});
    DrawText(("ECHO (RANK " + std::to_string(std::max(1, app.game.player.echoRank)) + ")").c_str(), 690, 165, 16,
             app.echoCampaignMode ? WHITE : Color{180, 180, 190, 255});
  } else {
    DrawText("A / D select. ENTER starts. Space/click plays. BACKSPACE gives up.", 30, 155, 17,
             Color{174, 174, 187, 255});
  }

  const auto& boss = app.bosses[static_cast<std::size_t>(app.selectedBoss)];
  const bool unlocked = ed::isBossUnlocked(app.game, app.campaign, app.bosses,
                                           static_cast<std::size_t>(app.selectedBoss));

  if (app.echoCampaignMode) {
    const Color cardBg = unlocked ? Color{52, 38, 68, 255} : Color{38, 32, 45, 255};
    DrawRectangleRounded({260, 215, 760, 280}, 0.08F, 10, cardBg);
    DrawRectangleRoundedLines({260, 215, 760, 280}, 0.08F, 10, Color{180, 110, 245, 255});

    DrawText(("Echo of " + boss.name).c_str(), 300, 245, 27, unlocked ? Color{245, 225, 255, 255} : Color{130, 120, 140, 255});
    const bool clearedThisRank = ed::isEchoBossCleared(app.game, app.selectedBoss);
    if (clearedThisRank) {
      DrawText(("Status: BANISHED (Cleared at Echo Rank " + std::to_string(std::max(1, app.game.player.echoRank)) + ")").c_str(),
               300, 288, 17, Color{150, 255, 170, 255});
    } else {
      DrawText("Status: ACTIVE ECHO (Awaiting banishment)", 300, 288, 17, Color{255, 210, 130, 255});
    }

    const double echoMod = ed::echoDifficultyMultiplier(std::max(1, app.game.player.echoRank));
    const int targetScore = static_cast<int>(std::llround(boss.clearScore * echoMod));
    DrawText(("Echo Difficulty: x" + std::to_string(echoMod).substr(0, 4) +
              "   Target Score: " + std::to_string(targetScore)).c_str(),
             300, 326, 18, Color{218, 170, 255, 255});

    const int goldReward = ed::echoBossRewardGold(boss.firstClearGold, std::max(1, app.game.player.echoRank));
    const int sigilReward = ed::echoBossRewardSigils(std::max(1, app.game.player.echoRank));
    DrawText(("Echo Rewards: +" + ed::formatCount(goldReward) + " Gold, +" +
              ed::formatCount(sigilReward) + " Echo Sigil(s)").c_str(),
             300, 365, 18, Color{255, 220, 120, 255});
  } else {
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
  }

  if (app.campaignAttempt) {
    DrawRectangle(0, 145, 1280, 520, Color{13, 17, 31, 255});
    DrawCircle(140, 280, 190, app.echoCampaignMode ? Color{45, 28, 65, 255} : Color{35, 49, 71, 255});
    DrawCircle(1140, 300, 230, app.echoCampaignMode ? Color{55, 25, 75, 255} : Color{48, 35, 61, 255});
    for (int x = 40; x < 1280; x += 150) {
      DrawTriangle({static_cast<float>(x), 145},
                   {static_cast<float>(x + 30), 145},
                   {static_cast<float>(x + 15), 230},
                   Color{8, 12, 24, 255});
    }
    const std::string arenaTitle = app.echoCampaignMode
        ? "ECHO OF " + boss.name + " — RANK " + std::to_string(std::max(1, app.game.player.echoRank))
        : boss.name + " — BATTLE ARENA";
    drawTextCentered(arenaTitle, 170, 26,
                     app.echoCampaignMode ? Color{235, 180, 255, 255} : Color{255, 224, 145, 255});
  }

  drawCampaignMinigames(app, boss);
}

void drawDungeonScreen(AppState& app) {
  const bool abyssUnlocked = (app.game.player.bossesCleared >= 10 || app.game.player.ascensions > 0 || app.game.player.abyssBestFloor > 0);

  if (app.inDungeonCombat) {
    if (app.dungeonShowResults) {
      if (app.dungeonResult.isAbyss) {
        drawTextCentered("ABYSS RESULTS", 108, 34, Color{218, 160, 255, 255});
        DrawRectangleRounded({310, 175, 660, 375}, 0.08F, 10, Color{45, 32, 60, 255});
        DrawRectangleRoundedLines({310, 175, 660, 375}, 0.08F, 10, Color{190, 110, 255, 255});

        const int reachedFloor = app.dungeonResult.floorsCleared + (app.dungeonResult.cleared ? 0 : 1);
        drawTextCentered(("Deepest Floor Reached: Floor " + std::to_string(reachedFloor)).c_str(), 198, 25, Color{235, 205, 255, 255});

        if (app.dungeonResult.floorsCleared >= app.game.player.abyssBestFloor && app.dungeonResult.floorsCleared > 0) {
          drawTextCentered("★ NEW PERSONAL BEST! ★", 232, 18, Color{255, 215, 0, 255});
        }

        DrawText(("Floors Cleared: " + std::to_string(app.dungeonResult.floorsCleared)).c_str(), 350, 265, 19, RAYWHITE);
        DrawText(("Gold Earned: +" + ed::formatCount(app.dungeonResult.gold)).c_str(), 350, 298, 19, Color{248, 211, 89, 255});
        DrawText(("XP Earned: +" + ed::formatCount(app.dungeonResult.xp)).c_str(), 350, 331, 19, Color{200, 140, 255, 255});
        DrawText(("Arcane Dust: +" + ed::formatCount(app.dungeonResult.arcaneDust)).c_str(), 350, 364, 19, Color{130, 210, 255, 255});
        DrawText(("Echo Sigils: +" + ed::formatCount(app.dungeonResult.echoSigils)).c_str(), 350, 397, 19, Color{235, 170, 255, 255});
        DrawText(("Aether Shards: +" + ed::formatCount(app.dungeonResult.aetherShards)).c_str(), 350, 430, 19, Color{150, 230, 255, 255});
        if (app.dungeonResult.gemFound) {
          DrawText("Bonus Gem Discovered!", 350, 463, 19, Color{255, 215, 100, 255});
        }

        DrawText("Press SPACE, ENTER or ESC to Finish", 440, 570, 20, Color{174, 174, 187, 255});
        return;
      }

      drawTextCentered("DUNGEON RESULTS", 108, 34, Color{224, 187, 106, 255});
      DrawRectangleRounded({340, 200, 600, 300}, 0.08F, 10, Color{53, 52, 65, 255});

      if (app.dungeonResult.cleared) {
        drawTextCentered("Run Cleared!", 230, 30, Color{122, 217, 255, 255});
      } else {
        drawTextCentered(("Failed on Floor " + std::to_string(app.dungeonResult.floorsCleared + 1)).c_str(), 230, 30, RED);
      }

      DrawText(("Gold Earned: " + ed::formatCount(app.dungeonResult.gold)).c_str(), 380, 290, 20, RAYWHITE);
      DrawText(("XP Earned: " + ed::formatCount(app.dungeonResult.xp)).c_str(), 380, 330, 20, RAYWHITE);
      DrawText(("Trophies Found: " + ed::formatCount(app.dungeonResult.trophies)).c_str(), 380, 370, 20, RAYWHITE);
      DrawText(("Arcane Dust: " + ed::formatCount(app.dungeonResult.arcaneDust)).c_str(), 380, 410, 20, RAYWHITE);
      if (app.dungeonResult.gemFound) {
        DrawText("You found a Gem!", 380, 450, 20, Color{224, 187, 106, 255});
      }

      DrawText("Press SPACE, ENTER or ESC to Finish", 440, 600, 20, Color{174, 174, 187, 255});
      return;
    }

    if (app.dungeonResult.isAbyss) {
      drawTextCentered("THE ABYSS COMBAT", 108, 34, Color{218, 160, 255, 255});
    } else {
      drawTextCentered("DUNGEON COMBAT", 108, 34, Color{224, 187, 106, 255});
    }

    DrawRectangleRounded({100, 200, 200, 180}, 0.1F, 8, Color{53, 52, 65, 255});
    DrawText("Player", 150, 390, 20, RAYWHITE);
    DrawText((ed::formatCount(app.dungeonPlayerHp) + " / " + ed::formatCount(app.dungeonPlayerMaxHp)).c_str(), 140, 420, 18, RED);

    int numEnemies = static_cast<int>(app.dungeonEnemies.size());
    int startX = 1080 - (numEnemies * 220);
    for (int i = 0; i < numEnemies; ++i) {
      auto& e = app.dungeonEnemies[i];
      if (e.hp > 0) {
        DrawRectangleRounded({static_cast<float>(startX + i * 220), 200, 180, 180}, 0.1F, 8,
                             app.dungeonResult.isAbyss ? Color{65, 30, 75, 255} : Color{70, 40, 40, 255});
        DrawText(e.name.c_str(), startX + i * 220 + 20, 390, 20, RAYWHITE);
        DrawText((ed::formatCount(e.hp) + " / " + ed::formatCount(e.maxHp)).c_str(), startX + i * 220 + 20, 420, 18, RED);
      }
    }

    DrawRectangleRounded({280, 500, 720, 150}, 0.08F, 10, Color{30, 30, 40, 200});
    int logY = 510;
    int startLog = std::max(0, static_cast<int>(app.dungeonBattleLog.size()) - 5);
    for (std::size_t i = startLog; i < app.dungeonBattleLog.size(); ++i) {
      DrawText(app.dungeonBattleLog[i].c_str(), 300, logY, 18, RAYWHITE);
      logY += 25;
    }

    DrawText("Press SPACE or ESC to skip floor", 480, 670, 18, Color{174, 174, 187, 255});
    return;
  }

  drawTextCentered("DUNGEON", 108, 34, Color{224, 187, 106, 255});

  if (abyssUnlocked) {
    DrawText("A / D select zone. ENTER starts. E / TAB toggles mode.", 30, 135, 15,
             Color{174, 174, 187, 255});
    // Mode toggle tabs
    const Rectangle stdTab{400, 155, 220, 36};
    const Rectangle abyssTab{660, 155, 220, 36};
    DrawRectangleRounded(stdTab, 0.2F, 4, !app.abyssDungeonMode ? Color{112, 76, 45, 255} : Color{50, 48, 62, 255});
    DrawRectangleRoundedLines(stdTab, 0.2F, 4, !app.abyssDungeonMode ? Color{224, 187, 106, 255} : Color{80, 78, 95, 255});
    DrawText("EXPEDITIONS", 455, 165, 16, !app.abyssDungeonMode ? WHITE : Color{180, 180, 190, 255});

    DrawRectangleRounded(abyssTab, 0.2F, 4, app.abyssDungeonMode ? Color{90, 45, 120, 255} : Color{50, 48, 62, 255});
    DrawRectangleRoundedLines(abyssTab, 0.2F, 4, app.abyssDungeonMode ? Color{218, 160, 255, 255} : Color{80, 78, 95, 255});
    DrawText("THE ABYSS", 725, 165, 16, app.abyssDungeonMode ? WHITE : Color{180, 180, 190, 255});
  } else {
    DrawText("Select zone with A / D. Press ENTER to start run.", 30, 155, 17,
             Color{174, 174, 187, 255});
  }

  if (app.abyssDungeonMode) {
    DrawRectangleRounded({260, 215, 760, 280}, 0.08F, 10, Color{45, 32, 60, 255});
    DrawRectangleRoundedLines({260, 215, 760, 280}, 0.08F, 10, Color{190, 110, 255, 255});

    DrawText("THE ABYSS — ENDLESS GAUNTLET", 300, 245, 27, Color{245, 225, 255, 255});
    DrawText(("Personal Best: Floor " + std::to_string(app.game.player.abyssBestFloor)).c_str(),
             300, 290, 19, Color{255, 215, 90, 255});
    DrawText("Infinite subterranean floors. Enemies grow +8% stronger each floor.",
             300, 325, 17, Color{210, 190, 225, 255});
    DrawText("Milestones: Void Bosses appear every 10 floors with Sigils & Shards.",
             300, 355, 17, Color{180, 220, 255, 255});
    DrawText("Rewards: High Gold, XP, Arcane Dust, Echo Sigils, Aether Shards.",
             300, 385, 17, Color{200, 240, 180, 255});

    DrawText("Press ENTER to descend into The Abyss", 300, 440, 20, Color{218, 160, 255, 255});
  } else {
    const auto& zone = app.zones[static_cast<std::size_t>(app.selectedZone)];
    DrawRectangleRounded({280, 230, 720, 220}, 0.08F, 10, Color{53, 52, 65, 255});
    DrawText(zone.name.c_str(), 330, 270, 30, RAYWHITE);
    DrawText(("Zone tier " + std::to_string(zone.tier)).c_str(), 330, 315, 19,
             Color{204, 175, 111, 255});
    DrawText("5 floors  |  auto-battle  |  gold, XP and trophies", 330, 355, 18,
             Color{182, 182, 194, 255});
    DrawText("ENTER to run", 330, 405, 20, Color{122, 217, 255, 255});
  }
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
      const bool craftable = ed::canCraftGear(app.game, app.inventory, item);
      Color slotBg;
      if (selected) {
        slotBg = craftable ? Color{55, 110, 65, 255} : Color{104, 72, 48, 255};
      } else {
        slotBg = craftable ? Color{38, 68, 48, 255} : Color{53, 52, 65, 255};
      }
      DrawRectangleRounded({static_cast<float>(listX + 10), static_cast<float>(y),
                            static_cast<float>(listWidth - 20), 36.0F},
                           0.2F, 6, slotBg);
      if (craftable && !selected) {
        DrawRectangleRoundedLines({static_cast<float>(listX + 10), static_cast<float>(y),
                                   static_cast<float>(listWidth - 20), 36.0F},
                                  0.2F, 6, Color{80, 165, 105, 180});
      }
      Color textCol = selected ? RAYWHITE : (craftable ? Color{215, 245, 215, 255} : Color{196, 196, 207, 255});
      DrawText(item.itemName.c_str(), listX + 24, y + 8, 16, textCol);
      DrawText(item.slot.c_str(), listX + 390, y + 9, 14, craftable ? Color{160, 220, 170, 255} : Color{177, 177, 191, 255});
      ++row;
    }
  }
  EndScissorMode();
  DrawRectangleLinesEx({static_cast<float>(listX), static_cast<float>(listY),
                        static_cast<float>(listWidth), static_cast<float>(listHeight)},
                       2.0F, Color{91, 82, 104, 255});
  const bool recipeCraftable = ed::canCraftGear(app.game, app.inventory, recipe);
  DrawRectangleRounded({650, 195, 560, 405}, 0.05F, 8, recipeCraftable ? Color{40, 54, 46, 255} : Color{53, 52, 65, 255});
  if (recipeCraftable) {
    DrawRectangleRoundedLines({650, 195, 560, 405}, 0.05F, 8, Color{80, 165, 105, 200});
  }
  DrawText(recipe.itemName.c_str(), 700, 240, 30, RAYWHITE);
  DrawText(("Tier " + std::to_string(recipe.tier) + " " + recipe.slot).c_str(), 700, 285, 20,
           Color{204, 175, 111, 255});
  DrawText(("Cost: " + ed::formatCount(recipe.ingots) + " ingots  |  " +
            ed::formatCount(recipe.trophies) + " trophies  |  " +
            ed::formatCount(recipe.gold) + " gold")
               .c_str(),
           700, 330, 18, Color{190, 190, 202, 255});
  DrawText(recipe.requiresGem ? "Also requires any gem" : "No gem required", 700, 370, 18,
           recipe.requiresGem ? Color{129, 214, 241, 255} : Color{174, 174, 187, 255});
  const int oreCount = ed::countItem(app.inventory, "ore", app.blockSpec.materialId);
  const int ingotCount = ed::countItem(app.inventory, "ingots", materialIdForTier(recipe.tier));
  DrawText(("Current ore: " + ed::formatCount(oreCount) + "   Current ingots: " +
            ed::formatCount(ingotCount) + "   Dust: " +
            ed::formatCount(app.game.player.arcaneDust))
               .c_str(),
           700, 410, 18, Color{180, 210, 178, 255});
  if (recipeCraftable) {
    DrawRectangleRounded({700, 450, 300, 36}, 0.2F, 4, Color{45, 110, 60, 255});
    DrawRectangleRoundedLines({700, 450, 300, 36}, 0.2F, 4, Color{100, 220, 130, 255});
    DrawText("READY TO FORGE (ENTER)", 735, 460, 15, WHITE);
  } else {
    DrawText("Smelt 4 matching ore with M. Crafting adds gear to Inventory.", 700, 455, 16,
             Color{174, 174, 187, 255});
  }
  DrawText("A/D select recipe   W/S or wheel scroll groups", 700, 520, 16,
           Color{174, 174, 187, 255});
}

void drawSettingsScreen(AppState& app) {
  drawTextCentered("SETTINGS & PREFERENCES", 108, 34, Color{224, 187, 106, 255});
  const Vector2 mouse = GetMousePosition();

  // Outer panel
  DrawRectangleRounded({280, 155, 720, 515}, 0.05F, 8, Color{36, 35, 48, 255});
  DrawRectangleRoundedLines({280, 155, 720, 515}, 0.05F, 8, Color{60, 58, 80, 255});

  // Section 1: Gameplay & Visuals
  DrawText("GAMEPLAY & VISUALS", 310, 180, 19, Color{224, 187, 106, 255});
  DrawLine(310, 206, 970, 206, Color{55, 52, 72, 255});

  // Screen Shake
  DrawText("Screen Shake", 310, 226, 17, RAYWHITE);
  DrawText("Visual feedback on crits, bosses, and heavy strikes", 310, 246, 13, Color{165, 165, 180, 255});
  const Rectangle shakeBtn{850, 220, 100, 36};
  const bool hoverShake = CheckCollisionPointRec(mouse, shakeBtn);
  DrawRectangleRounded(shakeBtn, 0.2F, 4, app.settings.screenShake ? (hoverShake ? Color{55, 115, 75, 255} : Color{45, 95, 60, 255})
                                                                   : (hoverShake ? Color{65, 65, 78, 255} : Color{48, 48, 58, 255}));
  DrawRectangleRoundedLines(shakeBtn, 0.2F, 4, app.settings.screenShake ? Color{100, 210, 130, 255} : Color{80, 80, 95, 255});
  DrawText(app.settings.screenShake ? "ON" : "OFF", 888, 228, 16, app.settings.screenShake ? Color{180, 255, 200, 255} : Color{170, 170, 185, 255});

  // Abbreviated Numbers
  DrawText("Number Formatting", 310, 281, 17, RAYWHITE);
  DrawText("Format large currency as 1.25k, 3.456m, etc.", 310, 301, 13, Color{165, 165, 180, 255});
  const Rectangle numBtn{850, 275, 100, 36};
  const bool hoverNum = CheckCollisionPointRec(mouse, numBtn);
  DrawRectangleRounded(numBtn, 0.2F, 4, app.settings.abbreviatedNumbers ? (hoverNum ? Color{55, 115, 75, 255} : Color{45, 95, 60, 255})
                                                                         : (hoverNum ? Color{65, 65, 78, 255} : Color{48, 48, 58, 255}));
  DrawRectangleRoundedLines(numBtn, 0.2F, 4, app.settings.abbreviatedNumbers ? Color{100, 210, 130, 255} : Color{80, 80, 95, 255});
  DrawText(app.settings.abbreviatedNumbers ? "ON" : "OFF", 888, 283, 16, app.settings.abbreviatedNumbers ? Color{180, 255, 200, 255} : Color{170, 170, 185, 255});

  // Section 2: Audio
  DrawText("AUDIO VOLUMES", 310, 340, 19, Color{224, 187, 106, 255});
  DrawLine(310, 366, 970, 366, Color{55, 52, 72, 255});

  auto drawVolumeControl = [&](const char* label, int volume, int y) {
    DrawText(label, 310, y + 6, 16, RAYWHITE);
    const Rectangle minusBtn{760, static_cast<float>(y), 36, 32};
    const bool hMinus = CheckCollisionPointRec(mouse, minusBtn);
    DrawRectangleRounded(minusBtn, 0.2F, 4, hMinus ? Color{65, 65, 80, 255} : Color{48, 48, 62, 255});
    DrawText("-", 774, y + 4, 20, RAYWHITE);

    DrawRectangleRounded({806, static_cast<float>(y), 98, 32}, 0.2F, 4, Color{26, 25, 36, 255});
    std::string vStr = std::to_string(volume) + "%";
    const int vW = MeasureText(vStr.c_str(), 16);
    DrawText(vStr.c_str(), 806 + (98 - vW) / 2, y + 7, 16, Color{245, 215, 120, 255});

    const Rectangle plusBtn{914, static_cast<float>(y), 36, 32};
    const bool hPlus = CheckCollisionPointRec(mouse, plusBtn);
    DrawRectangleRounded(plusBtn, 0.2F, 4, hPlus ? Color{65, 65, 80, 255} : Color{48, 48, 62, 255});
    DrawText("+", 926, y + 4, 20, RAYWHITE);
  };

  drawVolumeControl("Master Volume", app.settings.masterVolume, 380);
  drawVolumeControl("SFX Volume", app.settings.sfxVolume, 425);
  drawVolumeControl("Music Volume", app.settings.musicVolume, 470);

  // Section 3: Save Management & Actions
  DrawLine(310, 525, 970, 525, Color{55, 52, 72, 255});

  const Rectangle saveNowBtn{320, 545, 190, 44};
  const bool hoverSave = CheckCollisionPointRec(mouse, saveNowBtn);
  DrawRectangleRounded(saveNowBtn, 0.2F, 6, hoverSave ? Color{55, 115, 75, 255} : Color{42, 88, 58, 255});
  DrawRectangleRoundedLines(saveNowBtn, 0.2F, 6, Color{100, 220, 130, 255});
  DrawText("SAVE GAME (F5)", 348, 557, 16, RAYWHITE);

  const Rectangle switchSaveBtn{530, 545, 210, 44};
  const bool hoverSwitch = CheckCollisionPointRec(mouse, switchSaveBtn);
  DrawRectangleRounded(switchSaveBtn, 0.2F, 6, hoverSwitch ? Color{65, 75, 110, 255} : Color{48, 56, 85, 255});
  DrawRectangleRoundedLines(switchSaveBtn, 0.2F, 6, Color{120, 165, 235, 255});
  DrawText("MANAGE SAVES", 562, 557, 16, RAYWHITE);

  const Rectangle titleBtn{760, 545, 190, 44};
  const bool hoverTitle = CheckCollisionPointRec(mouse, titleBtn);
  DrawRectangleRounded(titleBtn, 0.2F, 6, hoverTitle ? Color{115, 65, 75, 255} : Color{80, 45, 55, 255});
  DrawRectangleRoundedLines(titleBtn, 0.2F, 6, Color{215, 120, 135, 255});
  DrawText("TITLE SCREEN", 792, 557, 16, RAYWHITE);

  // Footer status note
  std::string activeSaveStr = "Active Save: " + app.savePath.filename().string() +
                              "  |  Data Source: data/*.json (Offline Single-Player)";
  drawTextCentered(activeSaveStr, 615, 14, Color{160, 160, 175, 255});
}

void drawOfflineProgressModal(const AppState& app) {
  if (!app.offlinePopupActive) return;

  DrawRectangle(0, 0, 1280, 720, Color{0, 0, 0, 220});
  const Rectangle modalRec{340, 110, 600, 500};
  DrawRectangleRounded(modalRec, 0.08F, 8, Color{28, 26, 38, 255});
  DrawRectangleRoundedLines(modalRec, 0.08F, 8, Color{120, 230, 250, 255});

  drawTextCentered("WELCOME BACK, DELVER!", 140, 24, Color{248, 211, 89, 255});

  // Calculate formatted time
  const std::int64_t totalSec = app.offlineResult.secondsOffline;
  std::string timeStr;
  if (totalSec >= 3600) {
    const int hours = static_cast<int>(totalSec / 3600);
    const int mins = static_cast<int>((totalSec % 3600) / 60);
    const int secs = static_cast<int>(totalSec % 60);
    timeStr = std::to_string(hours) + "h " + std::to_string(mins) + "m " + std::to_string(secs) + "s";
  } else if (totalSec >= 60) {
    const int mins = static_cast<int>(totalSec / 60);
    const int secs = static_cast<int>(totalSec % 60);
    timeStr = std::to_string(mins) + "m " + std::to_string(secs) + "s";
  } else {
    timeStr = std::to_string(totalSec) + "s";
  }

  std::string subTitle = "Your Chrono Automaton worked while you were away (" + timeStr + ").";
  drawTextCentered(subTitle, 175, 14, Color{175, 185, 205, 255});

  DrawLine(370, 202, 910, 202, Color{55, 52, 72, 255});

  // Stats
  DrawText("Automatic Clicks:", 380, 220, 16, Color{180, 180, 195, 255});
  DrawText(ed::formatCount(app.offlineResult.clicks).c_str(), 680, 220, 16, Color{120, 230, 250, 255});

  DrawText("Blocks Excavated:", 380, 250, 16, Color{180, 180, 195, 255});
  DrawText(ed::formatCount(app.offlineResult.blocksBroken).c_str(), 680, 250, 16, RAYWHITE);

  DrawText("Gold Earned:", 380, 280, 16, Color{180, 180, 195, 255});
  DrawText(("+ " + ed::formatCount(app.offlineResult.goldEarned)).c_str(), 680, 280, 16, Color{248, 211, 89, 255});

  DrawText("XP Gained:", 380, 310, 16, Color{180, 180, 195, 255});
  DrawText(("+ " + ed::formatCount(app.offlineResult.xpEarned)).c_str(), 680, 310, 16, Color{100, 180, 255, 255});

  if (app.offlineResult.aetherShardsEarned > 0) {
    DrawText("Aether Shards:", 380, 340, 16, Color{180, 180, 195, 255});
    DrawText(("+ " + ed::formatCount(app.offlineResult.aetherShardsEarned)).c_str(), 680, 340, 16, Color{124, 247, 224, 255});
  }

  DrawLine(370, 370, 910, 370, Color{55, 52, 72, 255});

  // Materials preview
  DrawText("Materials Gathered:", 380, 385, 16, Color{224, 187, 106, 255});
  if (app.offlineResult.materialsEarned.empty()) {
    DrawText("None (block had high hardness or short duration)", 380, 412, 14, Color{150, 150, 165, 255});
  } else {
    int matY = 412;
    int count = 0;
    for (const auto& [mat, qty] : app.offlineResult.materialsEarned) {
      if (count >= 3) {
        DrawText("...and more", 380, matY, 14, Color{150, 150, 165, 255});
        break;
      }
      std::string mStr = "+ " + ed::formatCount(qty) + "  " + mat;
      DrawText(mStr.c_str(), 380, matY, 14, Color{200, 225, 210, 255});
      matY += 22;
      ++count;
    }
  }

  // Collect Button
  const Rectangle collectBtn{490, 540, 300, 46};
  const Vector2 mouse = GetMousePosition();
  const bool hoverCollect = CheckCollisionPointRec(mouse, collectBtn);
  DrawRectangleRounded(collectBtn, 0.2F, 6, hoverCollect ? Color{45, 140, 160, 255} : Color{35, 105, 125, 255});
  DrawRectangleRoundedLines(collectBtn, 0.2F, 6, Color{120, 230, 250, 255});
  const char* btnLabel = "COLLECT REWARDS (ENTER)";
  const int btnW = MeasureText(btnLabel, 16);
  DrawText(btnLabel, 490 + (300 - btnW) / 2, 554, 16, WHITE);
}


} // namespace ed::app
