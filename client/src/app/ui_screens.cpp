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

    const Screen navScreens[] = {Screen::Mine, Screen::Inventory, Screen::Crafting, Screen::Campaign, Screen::Dungeon, Screen::GemHollow, Screen::Enchanting, Screen::Profile};
    for (int i = 0; i < 8; ++i) {
      const int y = 80 + i * 50;
      const Screen screen = navScreens[i];
      const bool active = app.screen == screen;
      DrawRectangleRounded({10, static_cast<float>(y), 180, 40}, 0.2F, 4, active ? Color{112, 76, 45, 255} : Color{48, 47, 61, 255});
      DrawText(screenName(screen), 20, y + 12, 16, active ? RAYWHITE : Color{170, 170, 180, 255});
    }
  } else {
    DrawRectangleRounded({1220, 100, 40, 40}, 0.2F, 4, Color{48, 47, 61, 200});
    DrawRectangle(1228, 110, 24, 4, RAYWHITE);
    DrawRectangle(1228, 118, 24, 4, RAYWHITE);
    DrawRectangle(1228, 126, 24, 4, RAYWHITE);
  }
}

void drawMineScreen(const AppState& app) {
  drawTextCentered("THE EMBERDEEP", 106, 32, Color{224, 187, 106, 255});
  drawTextCentered("Layer: " + app.layers[static_cast<std::size_t>(app.layerIndex)].name, 140, 18,
                   Color{190, 190, 205, 255});
  DrawText("Q / E change layer", 28, 116, 16, Color{140, 140, 153, 255});

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

  // Healthbar drawn clearly above the block (unobstructed by block and on top)
  DrawRectangleRounded({320, 194, 640, 24}, 0.25F, 4, Color{45, 40, 52, 255});
  DrawRectangleRounded({320, 194, std::max(0.0F, 640.0F * hpRatio), 24}, 0.25F, 4, Color{195, 70, 60, 255});
  DrawRectangleRoundedLines({320, 194, 640, 24}, 0.25F, 4, Color{90, 80, 105, 255});
  drawTextCentered(std::to_string(app.block.currentHp) + " / " + std::to_string(app.block.maxHp),
                   198, 16, RAYWHITE);

  const int textStartY = blockY + blockSize + 10;
  drawTextCentered(app.blockSpec.materialName, textStartY, 22, RAYWHITE);
  drawTextCentered(rarityName(app.blockSpec.rarity), textStartY + 24, 15, Color{255, 210, 106, 255});
  drawTextCentered("Hardness " + std::to_string(app.block.hardness) + "   XP " + std::to_string(app.block.xp),
                   textStartY + 45, 16, Color{179, 179, 193, 255});

  if (strataUnlocked) {
    if (app.blockSpec.depth > 0) {
      const float shardChancePct = static_cast<float>(app.blockSpec.aetherShardChanceBasisPoints) / 100.0F;
      char chanceBuf[32];
      std::snprintf(chanceBuf, sizeof(chanceBuf), "%.2f%%", shardChancePct);
      std::string depthInfo = "Shard Chance: " + std::string(chanceBuf) +
                              "   |   Aetherium: x" + std::to_string(app.blockSpec.dropMultiplier) +
                              "   |   Shards: " + std::to_string(app.game.player.aetherShards) +
                              "   |   Best: Depth " + std::to_string(app.game.player.deepestDepth);
      drawTextCentered(depthInfo, textStartY + 68, 15, Color{124, 247, 224, 255});
    } else {
      std::string depthInfo = "Deep Strata Unlocked! Press ] or click [+] to descend  (Best: Depth " +
                              std::to_string(app.game.player.deepestDepth) +
                              "   |   Shards: " + std::to_string(app.game.player.aetherShards) + ")";
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
          DrawText(("+ " + std::to_string(app.bossCombatRewardGold) + " Gold").c_str(), 370, lootY, 18, Color{248, 211, 89, 255});
          lootY += 26;
        }
        if (app.bossCombatRewardXp > 0) {
          DrawText(("+ " + std::to_string(app.bossCombatRewardXp) + " XP").c_str(), 370, lootY, 18, Color{130, 215, 255, 255});
          lootY += 26;
        }
        if (app.bossCombatRewardSigils > 0) {
          DrawText(("+ " + std::to_string(app.bossCombatRewardSigils) + " Echo Sigils").c_str(), 370, lootY, 18, Color{218, 160, 255, 255});
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

        drawTextCentered(("Boss HP Remaining: " + std::to_string(app.bossCombatBossHp) + " / " +
                          std::to_string(app.bossCombatBossMaxHp)).c_str(), 290, 19, Color{255, 140, 140, 255});

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
    std::string playerHpText = std::to_string(app.bossCombatPlayerHp) + " / " + std::to_string(app.bossCombatPlayerMaxHp);
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
    std::string bossHpText = std::to_string(app.bossCombatBossHp) + " / " + std::to_string(app.bossCombatBossMaxHp);
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
    DrawText(("Echo Sigils: " + std::to_string(app.game.player.echoSigils)).c_str(), 1020, 108, 18, Color{218, 160, 255, 255});
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
    DrawText(("Echo Rewards: +" + std::to_string(goldReward) + " Gold, +" +
              std::to_string(sigilReward) + " Echo Sigil(s)").c_str(),
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
        DrawText(("Gold Earned: +" + std::to_string(app.dungeonResult.gold)).c_str(), 350, 298, 19, Color{248, 211, 89, 255});
        DrawText(("XP Earned: +" + std::to_string(app.dungeonResult.xp)).c_str(), 350, 331, 19, Color{200, 140, 255, 255});
        DrawText(("Arcane Dust: +" + std::to_string(app.dungeonResult.arcaneDust)).c_str(), 350, 364, 19, Color{130, 210, 255, 255});
        DrawText(("Echo Sigils: +" + std::to_string(app.dungeonResult.echoSigils)).c_str(), 350, 397, 19, Color{235, 170, 255, 255});
        DrawText(("Aether Shards: +" + std::to_string(app.dungeonResult.aetherShards)).c_str(), 350, 430, 19, Color{150, 230, 255, 255});
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

      DrawText(("Gold Earned: " + std::to_string(app.dungeonResult.gold)).c_str(), 380, 290, 20, RAYWHITE);
      DrawText(("XP Earned: " + std::to_string(app.dungeonResult.xp)).c_str(), 380, 330, 20, RAYWHITE);
      DrawText(("Trophies Found: " + std::to_string(app.dungeonResult.trophies)).c_str(), 380, 370, 20, RAYWHITE);
      DrawText(("Arcane Dust: " + std::to_string(app.dungeonResult.arcaneDust)).c_str(), 380, 410, 20, RAYWHITE);
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
    DrawText((std::to_string(app.dungeonPlayerHp) + " / " + std::to_string(app.dungeonPlayerMaxHp)).c_str(), 140, 420, 18, RED);

    int numEnemies = static_cast<int>(app.dungeonEnemies.size());
    int startX = 1080 - (numEnemies * 220);
    for (int i = 0; i < numEnemies; ++i) {
      auto& e = app.dungeonEnemies[i];
      if (e.hp > 0) {
        DrawRectangleRounded({static_cast<float>(startX + i * 220), 200, 180, 180}, 0.1F, 8,
                             app.dungeonResult.isAbyss ? Color{65, 30, 75, 255} : Color{70, 40, 40, 255});
        DrawText(e.name.c_str(), startX + i * 220 + 20, 390, 20, RAYWHITE);
        DrawText((std::to_string(e.hp) + " / " + std::to_string(e.maxHp)).c_str(), startX + i * 220 + 20, 420, 18, RED);
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

  if (app.inventory.gear.empty()) {
    DrawText("No gear in inventory.", 400, 300, 20, Color{174, 174, 187, 255});
    return;
  }

  DrawRectangle(50, 145, 530, 530, Color{28, 27, 39, 200});
  DrawText("Select gear:", 70, 158, 16, Color{174, 174, 187, 255});

  int listY = 185;
  for (size_t i = 0; i < app.inventory.gear.size(); ++i) {
    const auto& piece = app.inventory.gear[i];
    const bool selected = app.selectedGear == static_cast<int>(i);
    Color bg = selected ? Color{112, 76, 45, 255} : Color{48, 47, 61, 255};
    DrawRectangleRounded({65, static_cast<float>(listY - 4), 505, 34}, 0.15F, 4, bg);
    std::string label = piece.name + " +" + std::to_string(piece.upgradeLevel) + "  [" + piece.slot + "]";
    if (!piece.enchantment.empty()) label += "  *" + piece.enchantment + "*";
    DrawText(label.c_str(), 75, listY + 4, 15, selected ? RAYWHITE : Color{200, 200, 210, 255});

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse = GetMousePosition();
      if (mouse.x >= 65 && mouse.x <= 570 && mouse.y >= listY - 4 && mouse.y <= listY + 30) {
        app.selectedGear = static_cast<int>(i);
      }
    }
    listY += 42;
  }
  DrawText("W / S or click to select", 70, 650, 14, Color{120, 120, 135, 255});

  if (app.selectedGear >= 0 && app.selectedGear < static_cast<int>(app.inventory.gear.size())) {
    const auto& piece = app.inventory.gear[static_cast<size_t>(app.selectedGear)];
    int dustCost = ed::enchantDustCost(piece);
    int goldCost = ed::enchantGoldCost(piece);
    bool canAfford = app.game.player.arcaneDust >= dustCost && app.game.player.gold >= goldCost;

    DrawRectangle(610, 145, 620, 530, Color{28, 27, 39, 200});
    DrawText(piece.name.c_str(), 630, 165, 26, RAYWHITE);
    DrawText(("Slot: " + piece.slot + "   Tier: " + std::to_string(piece.tier) +
              "   Upgrade: +" + std::to_string(piece.upgradeLevel)).c_str(),
             630, 200, 16, Color{174, 174, 187, 255});

    if (!piece.enchantment.empty()) {
      DrawRectangleRounded({625, 230, 585, 36}, 0.2F, 4, Color{40, 80, 50, 255});
      DrawText(("Enchantment: " + piece.enchantment).c_str(), 635, 241, 16, Color{150, 255, 150, 255});
    } else {
      DrawRectangleRounded({625, 230, 585, 36}, 0.2F, 4, Color{50, 50, 65, 255});
      DrawText("No enchantment", 635, 241, 16, Color{150, 150, 165, 255});
    }

    DrawText(("Sockets: " + std::to_string(piece.socketedGems.size()) + " / " +
              std::to_string(piece.sockets)).c_str(), 630, 280, 16, Color{174, 174, 187, 255});

    DrawText("Enchant cost:", 630, 320, 18, RAYWHITE);
    Color dustColor = app.game.player.arcaneDust >= dustCost ? Color{150, 220, 255, 255} : Color{255, 100, 100, 255};
    Color goldColor = app.game.player.gold >= goldCost ? Color{248, 211, 89, 255} : Color{255, 100, 100, 255};
    DrawText(("  Arcane Dust: " + std::to_string(dustCost) +
              "  (have: " + std::to_string(app.game.player.arcaneDust) + ")").c_str(),
             630, 345, 16, dustColor);
    DrawText(("  Gold: " + std::to_string(goldCost) +
              "  (have: " + std::to_string(app.game.player.gold) + ")").c_str(),
             630, 368, 16, goldColor);

    if (!piece.enchantment.empty()) {
      DrawText(("Rerolls: " + std::to_string(piece.enchantRerolls)).c_str(), 630, 400, 16, Color{174, 174, 187, 255});
    }

    Color btnColor = canAfford ? Color{112, 76, 45, 255} : Color{60, 55, 70, 255};
    DrawRectangleRounded({625, 450, 250, 40}, 0.2F, 6, btnColor);
    DrawText(piece.enchantment.empty() ? "Enchant (N)" : "Re-enchant (N)", 680, 460, 20,
             canAfford ? RAYWHITE : Color{120, 115, 130, 255});

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse = GetMousePosition();
      if (mouse.x >= 625 && mouse.x <= 875 && mouse.y >= 450 && mouse.y <= 490) {
        if (canAfford) {
          auto& mpiece = app.inventory.gear[static_cast<size_t>(app.selectedGear)];
          bool ok = ed::enchantGearPiece(app.game, app.inventory, mpiece.itemId, app.rng);
          if (ok) setNotice(app, "Enchanted: " + mpiece.enchantment);
        } else {
          setNotice(app, "Not enough resources to enchant.");
        }
      }
    }
  }
}

void drawProfileScreen(const AppState& app) {
  drawTextCentered("PROFILE & ASCENSION", 108, 34, Color{224, 187, 106, 255});
  const auto& player = app.game.player;
  const auto stats = ed::calculatePlayerStats(app.game, app.inventory);
  const int levelCap = ed::playerLevelCap(player);
  const Vector2 mouse = GetMousePosition();

  // LEFT PANEL: Character & Stats
  DrawRectangleRounded({40, 160, 440, 505}, 0.05F, 8, Color{36, 35, 48, 255});
  DrawRectangleRoundedLines({40, 160, 440, 505}, 0.05F, 8, Color{60, 58, 80, 255});

  // Paperdoll / Avatar
  bool hasMythicEquipped = false;
  for (const auto& piece : app.inventory.gear) {
    if (piece.upgradeLevel >= 11) {
      if (piece.itemId == app.inventory.equipped.pickaxeId ||
          piece.itemId == app.inventory.equipped.helmId ||
          piece.itemId == app.inventory.equipped.chestplateId ||
          piece.itemId == app.inventory.equipped.gauntletsId ||
          piece.itemId == app.inventory.equipped.bootsId ||
          piece.itemId == app.inventory.equipped.amuletId) {
        hasMythicEquipped = true;
        break;
      }
    }
  }
  if (hasMythicEquipped) {
    const float pulse = (std::sin(static_cast<float>(GetTime()) * 3.5F) + 1.0F) * 0.5F;
    DrawCircleLines(105, 255, 50.0F + pulse * 4.0F, ColorAlpha(Color{205, 130, 255, 255}, 0.5F + 0.5F * pulse));
  }

  DrawCircle(105, 225, 34, Color{204, 158, 104, 255});
  DrawRectangleRounded({82, 264, 46, 68}, 0.2F, 4, Color{84, 105, 143, 255});

  // Level & XP
  DrawText(("Level " + std::to_string(player.level) + " / " + std::to_string(levelCap)).c_str(),
           165, 180, 24, Color{255, 224, 145, 255});
  DrawText(("Max Cap: " + std::to_string(levelCap) + (levelCap > 60 ? " (+Pot)" : "")).c_str(),
           165, 210, 16, Color{130, 220, 255, 255});

  const int nextXp = ed::xpToNextLevel(player.level);
  DrawText(("XP: " + std::to_string(player.xp) + " / " + std::to_string(nextXp)).c_str(),
           165, 235, 15, Color{190, 190, 210, 255});
  const float xpRatio = nextXp > 0 ? std::clamp(static_cast<float>(player.xp) / static_cast<float>(nextXp), 0.0F, 1.0F) : 1.0F;
  DrawRectangle(165, 258, 275, 8, Color{25, 25, 35, 255});
  DrawRectangle(165, 258, static_cast<int>(275 * xpRatio), 8, Color{100, 180, 255, 255});

  DrawText(("Gold: " + std::to_string(player.gold)).c_str(), 165, 275, 16, Color{248, 211, 89, 255});
  DrawText(("Dust: " + std::to_string(player.arcaneDust)).c_str(), 165, 298, 16, Color{190, 150, 250, 255});
  DrawText(("Pickaxe Tier: " + std::to_string(player.pickaxeTier)).c_str(), 165, 321, 16, Color{200, 220, 180, 255});

  DrawLine(55, 350, 465, 350, Color{60, 58, 80, 255});

  // Combat & Mining Stats
  DrawText("ATTRIBUTES", 55, 360, 17, Color{224, 187, 106, 255});
  DrawText(("Mining Power: " + std::to_string(static_cast<int>(stats.miningPower))).c_str(), 55, 388, 15, RAYWHITE);
  DrawText(("Attack: " + std::to_string(stats.attack)).c_str(), 55, 412, 15, RAYWHITE);
  DrawText(("Defense: " + std::to_string(stats.defense)).c_str(), 55, 436, 15, RAYWHITE);
  DrawText(("Max HP: " + std::to_string(stats.maxHp)).c_str(), 55, 460, 15, RAYWHITE);
  DrawText(("Speed: " + std::to_string(stats.speed)).c_str(), 265, 388, 15, RAYWHITE);
  DrawText(("Luck: " + std::to_string(stats.luck)).c_str(), 265, 412, 15, RAYWHITE);
  DrawText(("Pierce: " + std::to_string(stats.pierce)).c_str(), 265, 436, 15, RAYWHITE);
  DrawText(("Crit: " + std::to_string(stats.critChanceBasisPoints / 100) + "%").c_str(), 265, 460, 15, RAYWHITE);

  DrawLine(55, 490, 465, 490, Color{60, 58, 80, 255});

  // Progression Stats
  DrawText("LIFETIME PROGRESSION", 55, 500, 17, Color{224, 187, 106, 255});
  DrawText(("Bosses Cleared: " + std::to_string(player.bossesCleared) + " / 10").c_str(), 55, 528, 15, RAYWHITE);
  DrawText(("Echo Rank: " + std::to_string(player.echoRank)).c_str(), 55, 552, 15, RAYWHITE);
  DrawText(("Deepest Depth: " + std::to_string(player.deepestDepth)).c_str(), 55, 576, 15, RAYWHITE);
  DrawText(("Echo Sigils: " + std::to_string(player.echoSigils)).c_str(), 55, 600, 15, Color{218, 160, 255, 255});

  DrawText(("Abyss Best Floor: " + std::to_string(player.abyssBestFloor)).c_str(), 265, 528, 15, RAYWHITE);
  DrawText(("Ascensions: " + std::to_string(player.ascensions)).c_str(), 265, 552, 15, RAYWHITE);
  DrawText(("Blocks Broken: " + std::to_string(player.blocksBroken)).c_str(), 265, 576, 15, RAYWHITE);
  DrawText(("Aether Shards: " + std::to_string(player.aetherShards)).c_str(), 265, 600, 15, Color{124, 247, 224, 255});

  // RIGHT PANEL: Ascension & Ember Perks
  DrawRectangleRounded({505, 160, 735, 505}, 0.05F, 8, Color{36, 35, 48, 255});
  DrawRectangleRoundedLines({505, 160, 735, 505}, 0.05F, 8, Color{60, 58, 80, 255});

  const bool unlocked = (player.bossesCleared >= 10 || player.ascensions > 0);

  DrawText("EMBER PERKS & PRESTIGE", 525, 175, 20, Color{224, 187, 106, 255});
  DrawText(("Ember Marks: " + std::to_string(player.emberMarks)).c_str(),
           525, 205, 18, Color{255, 180, 50, 255});

  if (!unlocked) {
    DrawRectangleRounded({525, 250, 695, 380}, 0.05F, 8, Color{25, 24, 34, 255});
    drawTextCentered("ASCENSION LOCKED", 380, 24, Color{180, 180, 195, 255});
    drawTextCentered("Defeat The Hollow King (Boss 10) to unlock prestige & Ember Perks.", 430, 17, Color{140, 140, 160, 255});
    return;
  }

  const int pendingMarks = ed::calculateAscensionEmberMarks(player.goldEarnedThisRun, player.echoRank);
  DrawText(("Run Gold: " + std::to_string(player.goldEarnedThisRun) + "  ->  +" + std::to_string(pendingMarks) + " Marks on Ascend").c_str(),
           525, 230, 14, Color{180, 220, 180, 255});

  const Rectangle ascendBtn{1060, 173, 160, 42};
  const bool hoverAscend = CheckCollisionPointRec(mouse, ascendBtn);
  DrawRectangleRounded(ascendBtn, 0.2F, 4, hoverAscend ? Color{230, 140, 30, 255} : Color{180, 100, 20, 255});
  DrawRectangleRoundedLines(ascendBtn, 0.2F, 4, Color{255, 210, 120, 255});
  DrawText("ASCEND (ENTER)", 1070, 186, 14, WHITE);

  DrawLine(525, 254, 1220, 254, Color{60, 58, 80, 255});

  struct PerkRow {
    std::string id;
    std::string name;
    std::string desc;
    int rank;
    int maxRank;
    Color color;
  };

  const PerkRow rows[] = {
      {"potential", "Ember Potential", "+5 Level Cap per 3 ranks (Cap: " + std::to_string(levelCap) + "/100)",
       player.emberPerks.potential, 24, Color{100, 200, 255, 255}},
      {"power", "Ember Power", "+5% Mining Power per rank (Current: +" + std::to_string(player.emberPerks.power * 5) + "%)",
       player.emberPerks.power, 20, Color{255, 120, 100, 255}},
      {"wealth", "Ember Wealth", "+5% Gold earned per rank (Current: +" + std::to_string(player.emberPerks.wealth * 5) + "%)",
       player.emberPerks.wealth, 20, Color{255, 215, 0, 255}},
      {"wisdom", "Ember Wisdom", "+5% XP earned per rank (Current: +" + std::to_string(player.emberPerks.wisdom * 5) + "%)",
       player.emberPerks.wisdom, 20, Color{200, 140, 255, 255}},
      {"armory", "Ember Armory", "Start every run with Tier-3 Iron gear set",
       player.emberPerks.armory, 1, Color{120, 220, 150, 255}},
      {"smelting", "Ember Smelting", "Faster & high-yield furnace smelting",
       player.emberPerks.smelting, 5, Color{255, 160, 80, 255}},
  };

  int py = 262;
  for (std::size_t i = 0; i < 6; ++i) {
    const auto& row = rows[i];
    DrawRectangleRounded({520, static_cast<float>(py), 705, 59}, 0.15F, 4, Color{28, 27, 38, 255});
    DrawText(row.name.c_str(), 535, py + 8, 16, row.color);
    DrawText(row.desc.c_str(), 535, py + 30, 13, Color{175, 175, 190, 255});

    DrawText(("Rank " + std::to_string(row.rank) + "/" + std::to_string(row.maxRank)).c_str(),
             835, py + 19, 15, Color{210, 210, 225, 255});

    const Rectangle upgradeBtn{1040, static_cast<float>(py + 9), 175, 40};
    const bool isMax = (row.rank >= row.maxRank);
    const int cost = isMax ? 0 : ed::perkUpgradeCost(row.id, row.rank);
    const bool canAfford = (!isMax && player.emberMarks >= cost);

    if (isMax) {
      DrawRectangleRounded(upgradeBtn, 0.2F, 4, Color{45, 55, 50, 255});
      DrawText("MAX RANK", 1085, py + 20, 15, Color{120, 180, 140, 255});
    } else {
      const bool hover = CheckCollisionPointRec(mouse, upgradeBtn);
      DrawRectangleRounded(upgradeBtn, 0.2F, 4,
                           canAfford ? (hover ? Color{70, 120, 85, 255} : Color{50, 95, 65, 255})
                                     : Color{55, 45, 50, 255});
      DrawRectangleRoundedLines(upgradeBtn, 0.2F, 4,
                                canAfford ? Color{100, 200, 130, 255} : Color{80, 70, 75, 255});
      const std::string btnText = "+" + std::to_string(cost) + " Marks (" + std::to_string(i + 1) + ")";
      DrawText(btnText.c_str(), 1055, py + 20, 15, canAfford ? WHITE : Color{140, 130, 135, 255});
    }

    py += 65;
  }

  if (app.ascensionConfirm) {
    DrawRectangle(0, 0, 1280, 720, Color{0, 0, 0, 215});
    const Rectangle modalRec{370, 190, 540, 330};
    DrawRectangleRounded(modalRec, 0.08F, 8, Color{28, 26, 38, 255});
    DrawRectangleRoundedLines(modalRec, 0.08F, 8, Color{248, 160, 40, 255});

    DrawText("CONFIRM ASCENSION", 480, 220, 24, Color{255, 215, 90, 255});
    DrawText(("You will receive: +" + std::to_string(pendingMarks) + " Ember Marks.").c_str(),
             410, 270, 18, Color{255, 230, 140, 255});
    DrawText("RESETS: Level (to 1), Gold, Ores, Ingots, Trophies,",
             410, 310, 15, Color{240, 150, 150, 255});
    DrawText("        and non-legendary equipment.",
             410, 332, 15, Color{240, 150, 150, 255});
    DrawText("KEEPS: Legendary gear, Gems, Echo rank, Marks & Perks.",
             410, 370, 15, Color{150, 230, 170, 255});

    const Rectangle confirmBtn{410, 430, 210, 48};
    const Rectangle cancelBtn{670, 430, 210, 48};
    const bool hoverConfirm = CheckCollisionPointRec(mouse, confirmBtn);
    const bool hoverCancel = CheckCollisionPointRec(mouse, cancelBtn);

    DrawRectangleRounded(confirmBtn, 0.2F, 4, hoverConfirm ? Color{180, 60, 50, 255} : Color{140, 40, 35, 255});
    DrawRectangleRoundedLines(confirmBtn, 0.2F, 4, Color{255, 120, 100, 255});
    DrawText("ASCEND (Y)", 465, 444, 18, WHITE);

    DrawRectangleRounded(cancelBtn, 0.2F, 4, hoverCancel ? Color{65, 65, 80, 255} : Color{50, 50, 65, 255});
    DrawRectangleRoundedLines(cancelBtn, 0.2F, 4, Color{130, 130, 150, 255});
    DrawText("CANCEL (N)", 725, 444, 18, WHITE);
  }
}

int inventoryGearStartY(const AppState& app) {
  int y = 215;
  const auto advanceBucket = [&y](const std::map<std::string, int>& bucket) {
    int x = 250;
    for (const auto& [_, count] : bucket) {
      static_cast<void>(count);
      x += 230;
      if (x > 1100) {
        x = 250;
        y += 52;
      }
    }
    y += 58;
  };
  advanceBucket(app.inventory.ore);
  advanceBucket(app.inventory.ingots);
  advanceBucket(app.inventory.trophies);
  advanceBucket(app.inventory.gems);
  return y + 42;
}

int inventoryContentHeight(const AppState& app) {
  const int gearStart = inventoryGearStartY(app);
  const int gearRows = (static_cast<int>(app.inventory.gear.size()) + 2) / 3;
  return app.inventory.gear.empty() ? gearStart + 30 : gearStart + gearRows * 70 + 45;
}

void clampInventoryScroll(AppState& app) {
  constexpr int viewportHeight = 500;
  const int totalHeight = inventoryContentHeight(app) - 160;
  const int maxScroll = std::max(0, totalHeight - viewportHeight);
  app.inventoryScroll = std::clamp(app.inventoryScroll, 0, maxScroll);
}

void syncGearScroll(AppState& app) {
  if (app.inventory.gear.empty()) {
    app.selectedGear = 0;
    return;
  }
  app.selectedGear = std::clamp(app.selectedGear, 0,
                                static_cast<int>(app.inventory.gear.size()) - 1);
  constexpr int viewportTop = 160;
  constexpr int viewportBottom = 640;
  const int gearStart = inventoryGearStartY(app);
  const int selectedTop = gearStart + (app.selectedGear / 3) * 70;
  const int selectedBottom = selectedTop + 52;
  if (selectedTop - app.inventoryScroll < viewportTop) {
    app.inventoryScroll = std::max(0, selectedTop - viewportTop);
  } else if (selectedBottom - app.inventoryScroll > viewportBottom) {
    app.inventoryScroll = selectedBottom - viewportBottom;
  }
  clampInventoryScroll(app);
}

void drawInventoryScreen(const AppState& app) {
  drawTextCentered("INVENTORY", 108, 34, Color{224, 187, 106, 255});
  DrawText("Mouse wheel or W/S scroll. A/D select. E equip. U upgrade. G/R gems.", 70, 138, 15, Color{174, 174, 187, 255});
  BeginScissorMode(0, 160, 1280, 500);
  const int offset = app.inventoryScroll;
  DrawText("Resources", 70, 175 - offset, 24, Color{235, 224, 191, 255});
  int resHeaderX = 250;
  if (app.game.player.aetherShards > 0) {
    DrawText(("Aether Shards: " + std::to_string(app.game.player.aetherShards)).c_str(),
             resHeaderX, 178 - offset, 18, Color{124, 247, 224, 255});
    resHeaderX += 200;
  }
  if (app.game.player.echoSigils > 0) {
    DrawText(("Echo Sigils: " + std::to_string(app.game.player.echoSigils)).c_str(),
             resHeaderX, 178 - offset, 18, Color{218, 160, 255, 255});
    resHeaderX += 190;
  }
  if (app.game.player.arcaneDust > 0) {
    DrawText(("Arcane Dust: " + std::to_string(app.game.player.arcaneDust)).c_str(),
             resHeaderX, 178 - offset, 18, Color{195, 175, 245, 255});
  }
  int y = 215;
  const auto drawBucket = [&](const char* title, const std::map<std::string, int>& bucket, bool isIngot = false) {
    DrawText(title, 70, y - offset, 18, Color{174, 174, 187, 255});
    int x = 250;
    for (const auto& [id, count] : bucket) {
      DrawRectangleRounded({static_cast<float>(x), static_cast<float>(y - 5 - offset), 220, 44}, 0.18F, 6,
                           Color{52, 52, 67, 255});

      if (isIngot) {
        std::string safeId = id;
        for (char& c : safeId) if (c == '-') c = '_';
        std::string texPath = "sprites/Materials/" + safeId + "_ingot.png";
        Texture2D tex = getTexture(texPath);
        int frame = static_cast<int>(GetTime() * 4) % 4;
        Rectangle source = {static_cast<float>(frame * 32), 0, 32, 32};
        Rectangle dest = {static_cast<float>(x + 4), static_cast<float>(y + 1 - offset), 32, 32};
        DrawTexturePro(tex, source, dest, {0,0}, 0.0f, WHITE);

        DrawText((id + " x" + std::to_string(count)).c_str(), x + 40, y + 8 - offset, 16, RAYWHITE);
      } else {
        DrawText((id + " x" + std::to_string(count)).c_str(), x + 12, y + 8 - offset, 16, RAYWHITE);
      }

      x += 230;
      if (x > 1100) {
        x = 250;
        y += 52;
      }
    }
    y += 58;
  };
  drawBucket("Ore", app.inventory.ore);
  drawBucket("Ingots", app.inventory.ingots, true);
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
    constexpr int rowHeight = 70;
    for (std::size_t index = 0; index < app.inventory.gear.size(); ++index) {
      const auto& piece = app.inventory.gear[static_cast<std::size_t>(index)];
      const int column = static_cast<int>(index % 3);
      const int row = static_cast<int>(index / 3);
      const bool selected = static_cast<int>(index) == app.selectedGear;
      const bool isMythic = (piece.upgradeLevel >= 11);
      Color cardBg = selected ? Color{112, 76, 45, 255} : (isMythic ? Color{58, 42, 72, 255} : Color{52, 52, 67, 255});
      DrawRectangleRounded({70.0F + static_cast<float>(column) * 370.0F,
                            static_cast<float>(y + row * rowHeight - offset), cardWidth, cardHeight},
                           0.12F, 6, cardBg);
      if (isMythic) {
        const float pulse = (std::sin(static_cast<float>(GetTime()) * 4.0F) + 1.0F) * 0.5F;
        const Color glow = ColorAlpha(Color{205, 140, 255, 255}, 0.5F + 0.5F * pulse);
        DrawRectangleRoundedLines({70.0F + static_cast<float>(column) * 370.0F,
                                   static_cast<float>(y + row * rowHeight - offset), cardWidth, cardHeight},
                                  0.12F, 6, glow);
      }
      std::string nameLabel = piece.name + " +" + std::to_string(piece.upgradeLevel);
      if (isMythic) {
        nameLabel += " [MYTHIC]";
      }
      DrawText(nameLabel.c_str(),
               84 + column * 370, y + row * rowHeight + 8 - offset, 17,
               isMythic ? Color{245, 215, 255, 255} : RAYWHITE);
      std::string subLabel = "Tier " + std::to_string(piece.tier) + " | " + piece.slot;
      if (piece.sockets > 0) {
        subLabel += " | Sockets: " + std::to_string(piece.socketedGems.size()) + "/" + std::to_string(piece.sockets);
      }
      DrawText(subLabel.c_str(),
               84 + column * 370, y + row * rowHeight + 30 - offset, 13,
               Color{180, 180, 192, 255});
    }
  }
  EndScissorMode();

  if (app.selectedGear >= 0 && app.selectedGear < static_cast<int>(app.inventory.gear.size())) {
    const auto& piece = app.inventory.gear[static_cast<std::size_t>(app.selectedGear)];
    std::string statusText;
    if (piece.upgradeLevel >= 20) {
      statusText = piece.name + " +" + std::to_string(piece.upgradeLevel) + "  [MAX MYTHIC LEVEL]";
    } else if (piece.upgradeLevel >= 10) {
      if (ed::isMythicUpgradeUnlocked(app.game)) {
        statusText = "Mythic Upgrade [U] -> +" + std::to_string(piece.upgradeLevel + 1) + ": " +
                     std::to_string(ed::upgradeGoldCost(piece)) + "g, " +
                     std::to_string(ed::upgradeDustCost(piece)) + " dust, " +
                     std::to_string(ed::upgradeAetherShardCost(piece)) + " shard(s), " +
                     std::to_string(ed::upgradeEchoSigilCost(piece)) + " sigil(s)";
      } else {
        statusText = piece.name + " +10  [MAX STANDARD] (Mythic upgrades unlock after Boss 10)";
      }
    } else {
      statusText = "Upgrade [U] -> +" + std::to_string(piece.upgradeLevel + 1) + ": " +
                   std::to_string(ed::upgradeGoldCost(piece)) + "g, " +
                   std::to_string(ed::upgradeDustCost(piece)) + " dust";
    }
    DrawText(statusText.c_str(), 70, 672, 16, Color{225, 210, 160, 255});
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

} // namespace ed::app
