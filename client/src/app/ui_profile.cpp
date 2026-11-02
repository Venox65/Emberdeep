#include "ui_screens.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "ed/achievements.hpp"
#include "ed/formulas.hpp"
#include "ed/postgame.hpp"
#include "ed/stats.hpp"

namespace ed::app {

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
  DrawText(("XP: " + ed::formatCount(player.xp) + " / " + ed::formatCount(nextXp)).c_str(),
           165, 235, 15, Color{190, 190, 210, 255});
  const float xpRatio = nextXp > 0 ? std::clamp(static_cast<float>(player.xp) / static_cast<float>(nextXp), 0.0F, 1.0F) : 1.0F;
  DrawRectangle(165, 258, 275, 8, Color{25, 25, 35, 255});
  DrawRectangle(165, 258, static_cast<int>(275 * xpRatio), 8, Color{100, 180, 255, 255});

  DrawText(("Gold: " + ed::formatCount(player.gold)).c_str(), 165, 275, 16, Color{248, 211, 89, 255});
  DrawText(("Dust: " + ed::formatCount(player.arcaneDust)).c_str(), 165, 298, 16, Color{190, 150, 250, 255});
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
  DrawText(("Echo Sigils: " + ed::formatCount(player.echoSigils)).c_str(), 55, 600, 15, Color{218, 160, 255, 255});

  DrawText(("Abyss Best Floor: " + std::to_string(player.abyssBestFloor)).c_str(), 265, 528, 15, RAYWHITE);
  DrawText(("Ascensions: " + std::to_string(player.ascensions)).c_str(), 265, 552, 15, RAYWHITE);
  DrawText(("Blocks Broken: " + ed::formatCount(player.blocksBroken)).c_str(), 265, 576, 15, RAYWHITE);
  DrawText(("Aether Shards: " + ed::formatCount(player.aetherShards)).c_str(), 265, 600, 15, Color{124, 247, 224, 255});

  // RIGHT PANEL: Ascension & Ember Perks / Achievements
  DrawRectangleRounded({505, 160, 735, 505}, 0.05F, 8, Color{36, 35, 48, 255});
  DrawRectangleRoundedLines({505, 160, 735, 505}, 0.05F, 8, Color{60, 58, 80, 255});

  const auto& allAchs = ed::allAchievements();
  int unlockedCount = 0;
  for (const auto& a : allAchs) {
    if (ed::isAchievementUnlocked(player, a.id)) ++unlockedCount;
  }

  // Right Panel Tabs
  const bool vowsActive = (app.profileTab == ProfileTab::Vows);
  const bool perksActive = (app.profileTab == ProfileTab::Perks);
  const bool achsActive = (app.profileTab == ProfileTab::Achievements);

  const Rectangle vowsTabBtn{505, 160, 240, 36};
  const Rectangle perksTabBtn{750, 160, 240, 36};
  const Rectangle achievementsTabBtn{995, 160, 245, 36};

  DrawRectangleRounded(vowsTabBtn, 0.15F, 4, vowsActive ? Color{52, 48, 68, 255} : Color{28, 27, 38, 255});
  DrawRectangleRoundedLines(vowsTabBtn, 0.15F, 4, vowsActive ? Color{224, 187, 106, 255} : Color{50, 48, 65, 255});
  const char* vowsTitle = "MINER'S VOWS";
  DrawText(vowsTitle, 505 + (240 - MeasureText(vowsTitle, 16)) / 2, 170, 16, vowsActive ? Color{245, 215, 120, 255} : Color{160, 160, 175, 255});

  DrawRectangleRounded(perksTabBtn, 0.15F, 4, perksActive ? Color{52, 48, 68, 255} : Color{28, 27, 38, 255});
  DrawRectangleRoundedLines(perksTabBtn, 0.15F, 4, perksActive ? Color{224, 187, 106, 255} : Color{50, 48, 65, 255});
  const char* perksTitle = "EMBER PERKS";
  DrawText(perksTitle, 750 + (240 - MeasureText(perksTitle, 16)) / 2, 170, 16, perksActive ? Color{245, 215, 120, 255} : Color{160, 160, 175, 255});

  DrawRectangleRounded(achievementsTabBtn, 0.15F, 4, achsActive ? Color{52, 48, 68, 255} : Color{28, 27, 38, 255});
  DrawRectangleRoundedLines(achievementsTabBtn, 0.15F, 4, achsActive ? Color{224, 187, 106, 255} : Color{50, 48, 65, 255});
  std::string achTabTitle = "ACHIEVEMENTS (" + std::to_string(unlockedCount) + "/" + std::to_string(allAchs.size()) + ")";
  DrawText(achTabTitle.c_str(), 995 + (245 - MeasureText(achTabTitle.c_str(), 16)) / 2, 170, 16, achsActive ? Color{245, 215, 120, 255} : Color{160, 160, 175, 255});

  if (achsActive) {
    const float achRatio = allAchs.empty() ? 0.0F : static_cast<float>(unlockedCount) / static_cast<float>(allAchs.size());
    DrawRectangleRounded({525, 204, 695, 14}, 0.3F, 4, Color{25, 24, 34, 255});
    DrawRectangleRounded({525, 204, 695.0F * achRatio, 14}, 0.3F, 4, Color{100, 200, 130, 255});
    DrawRectangleRoundedLines({525, 204, 695, 14}, 0.3F, 4, Color{70, 68, 90, 255});
    std::string pctText = std::to_string(static_cast<int>(achRatio * 100.0F)) + "% Completed (" +
                          std::to_string(unlockedCount) + "/" + std::to_string(allAchs.size()) + ")";
    DrawText(pctText.c_str(), 525, 222, 14, Color{180, 220, 190, 255});
    DrawText("Scroll W/S or wheel   |   Press TAB to switch tab", 840, 222, 13, Color{150, 150, 165, 255});

    BeginScissorMode(510, 242, 725, 415);
    for (std::size_t i = 0; i < allAchs.size(); ++i) {
      const auto& a = allAchs[i];
      const int cardY = 246 + static_cast<int>(i) * 66 - app.achievementsScroll;
      if (cardY + 60 < 240 || cardY > 660) continue;

      const bool isUnlocked = ed::isAchievementUnlocked(player, a.id);
      Color cardBg = isUnlocked ? Color{34, 48, 42, 255} : Color{28, 27, 37, 255};
      Color border = isUnlocked ? Color{65, 130, 85, 255} : Color{50, 48, 62, 255};

      DrawRectangleRounded({520, static_cast<float>(cardY), 705, 58}, 0.15F, 4, cardBg);
      DrawRectangleRoundedLines({520, static_cast<float>(cardY), 705, 58}, 0.15F, 4, border);

      DrawText(a.icon.c_str(), 535, cardY + 16, 20, isUnlocked ? Color{255, 215, 0, 255} : Color{120, 120, 135, 255});
      DrawText(a.title.c_str(), 575, cardY + 8, 17, isUnlocked ? Color{255, 225, 130, 255} : Color{180, 180, 195, 255});
      const int titleW = MeasureText(a.title.c_str(), 17);
      DrawRectangleRounded({static_cast<float>(585 + titleW), static_cast<float>(cardY + 8), 90, 18}, 0.3F, 4, Color{45, 42, 60, 255});
      DrawText(a.category.c_str(), 592 + titleW, cardY + 10, 12, Color{190, 180, 215, 255});
      DrawText(a.description.c_str(), 575, cardY + 32, 13, isUnlocked ? Color{200, 215, 205, 255} : Color{145, 145, 160, 255});

      if (isUnlocked) {
        DrawRectangleRounded({1110, static_cast<float>(cardY + 14), 100, 28}, 0.25F, 4, Color{45, 95, 60, 255});
        DrawText("UNLOCKED", 1122, cardY + 21, 13, Color{160, 255, 180, 255});
      } else {
        DrawRectangleRounded({1110, static_cast<float>(cardY + 14), 100, 28}, 0.25F, 4, Color{40, 39, 50, 255});
        DrawText("LOCKED", 1133, cardY + 21, 13, Color{130, 130, 145, 255});
      }
    }
    EndScissorMode();
    return;
  }

  if (vowsActive) {
    DrawText(("Vow Points Available: " + std::to_string(player.vowPoints)).c_str(),
             525, 208, 18, Color{255, 220, 80, 255});

    const int respecCost = ed::respecVowsCost(app.game);
    const int spentPoints = ed::totalVowPointsSpent(app.game);
    const bool canRespec = (spentPoints > 0 && player.gold >= respecCost);
    const Rectangle respecBtn{1040, 202, 180, 32};
    const bool hoverRespec = CheckCollisionPointRec(mouse, respecBtn);

    DrawRectangleRounded(respecBtn, 0.2F, 4, canRespec ? (hoverRespec ? Color{160, 60, 50, 255} : Color{130, 45, 38, 255})
                                                       : Color{45, 42, 50, 255});
    DrawRectangleRoundedLines(respecBtn, 0.2F, 4, canRespec ? Color{235, 100, 90, 255} : Color{70, 65, 75, 255});
    std::string respecText = "Respec (" + ed::formatCount(respecCost) + "g)";
    DrawText(respecText.c_str(),
             static_cast<int>(respecBtn.x + (respecBtn.width - MeasureText(respecText.c_str(), 14)) / 2),
             static_cast<int>(respecBtn.y + 9), 14, canRespec ? WHITE : Color{130, 130, 140, 255});

    DrawLine(525, 240, 1220, 240, Color{60, 58, 80, 255});

    struct VowDef {
      std::string id;
      std::string name;
      std::string effect;
      bool isCapstone;
    };

    struct BranchDef {
      std::string title;
      Color headerColor;
      std::vector<VowDef> vows;
    };

    // Statically allocated vow tree definitions to eliminate dynamic allocations at 60 FPS
    static const std::vector<BranchDef> branches = {
      {
        "DELVER (MINING)",
        Color{248, 190, 80, 255},
        {
          {"strong_arms", "Strong Arms", "+3% Mining Power / rank", false},
          {"keen_eye", "Keen Eye", "+1% Crit Chance / rank", false},
          {"prospector", "Prospector", "+1 Luck / rank", false},
          {"deep_breath", "Deep Breath", "Weak Spot 5% faster", false},
          {"shatterer", "Shatterer", "Crit every 25th hit (Lv30)", true}
        }
      },
      {
        "WARRIOR (COMBAT)",
        Color{245, 105, 105, 255},
        {
          {"stout_heart", "Stout Heart", "+4% Max HP / rank", false},
          {"whetstone", "Whetstone", "+3% Attack / rank", false},
          {"ironskin", "Ironskin", "+2% Defense / rank", false},
          {"swift_boots", "Swift Boots", "+1 Speed / rank", false},
          {"second_wind", "Second Wind", "Revive @ 30% HP (Lv30)", true}
        }
      },
      {
        "SCHOLAR (ECONOMY)",
        Color{140, 215, 255, 255},
        {
          {"quick_study", "Quick Study", "+3% XP / rank", false},
          {"haggler", "Haggler", "+3% Sell Gold / rank", false},
          {"steady_hand", "Steady Hand", "+1% Focus / rank", false},
          {"dust_collector", "Dust Collector", "+2% Dust Drop / rank", false},
          {"gem_whisperer", "Gem Whisperer", "+3 Hollow Swings (Lv30)", true}
        }
      }
    };

    for (std::size_t c = 0; c < branches.size(); ++c) {
      const auto& branch = branches[c];
      const int colX = 520 + static_cast<int>(c) * 236;

      DrawText(branch.title.c_str(),
               colX + (226 - MeasureText(branch.title.c_str(), 14)) / 2,
               246, 14, branch.headerColor);

      for (std::size_t r = 0; r < branch.vows.size(); ++r) {
        const auto& v = branch.vows[r];
        const int rowY = 268 + static_cast<int>(r) * 78;
        const int curRank = ed::currentVowRank(app.game, v.id);
        const int maxRank = ed::maxVowRank(v.id);
        const bool isMax = (curRank >= maxRank);
        const bool canUp = ed::canUpgradeVow(app.game, v.id);
        const bool capLocked = (v.isCapstone && player.level < 30);

        Color cardBg = Color{28, 27, 38, 255};
        Color border = v.isCapstone ? Color{180, 140, 50, 255} : Color{55, 52, 70, 255};
        if (curRank > 0) {
          cardBg = Color{34, 38, 48, 255};
          border = Color{80, 110, 150, 255};
        }

        DrawRectangleRounded({static_cast<float>(colX), static_cast<float>(rowY), 226, 72}, 0.15F, 4, cardBg);
        DrawRectangleRoundedLines({static_cast<float>(colX), static_cast<float>(rowY), 226, 72}, 0.15F, 4, border);

        // Vow title & rank
        DrawText(v.name.c_str(), colX + 10, rowY + 8, 14, v.isCapstone ? Color{255, 215, 110, 255} : RAYWHITE);
        std::string rankStr = isMax ? "MAX" : ("Rank " + std::to_string(curRank) + "/" + std::to_string(maxRank));
        Color rankCol = isMax ? Color{120, 240, 150, 255} : (curRank > 0 ? Color{140, 200, 255, 255} : Color{140, 140, 155, 255});
        DrawText(rankStr.c_str(), colX + 10, rowY + 28, 12, rankCol);

        // Effect
        DrawText(v.effect.c_str(), colX + 10, rowY + 48, 11, Color{175, 175, 190, 255});

        // Upgrade button / state badge
        const Rectangle upBtn{static_cast<float>(colX + 182), static_cast<float>(rowY + 20), 34, 32};
        if (isMax) {
          DrawRectangleRounded(upBtn, 0.2F, 4, Color{30, 60, 40, 255});
          DrawRectangleRoundedLines(upBtn, 0.2F, 4, Color{60, 140, 80, 255});
          DrawText("OK", colX + 191, rowY + 28, 13, Color{120, 240, 150, 255});
        } else if (capLocked) {
          DrawRectangleRounded(upBtn, 0.2F, 4, Color{45, 30, 30, 255});
          DrawRectangleRoundedLines(upBtn, 0.2F, 4, Color{100, 50, 50, 255});
          DrawText("L30", colX + 188, rowY + 29, 11, Color{220, 120, 120, 255});
        } else {
          const bool hoverUp = CheckCollisionPointRec(mouse, upBtn);
          DrawRectangleRounded(upBtn, 0.2F, 4, canUp ? (hoverUp ? Color{60, 130, 180, 255} : Color{45, 95, 140, 255})
                                                     : Color{40, 40, 50, 255});
          DrawRectangleRoundedLines(upBtn, 0.2F, 4, canUp ? Color{110, 200, 255, 255} : Color{65, 65, 75, 255});
          DrawText("+", colX + 194, rowY + 26, 18, canUp ? WHITE : Color{120, 120, 130, 255});
        }
      }
    }
    return;
  }

  const bool unlocked = (player.bossesCleared >= 10 || player.ascensions > 0);

  DrawText(("Ember Marks: " + ed::formatCount(player.emberMarks)).c_str(),
           525, 205, 18, Color{255, 180, 50, 255});
  DrawText(("Chrono Cores: " + ed::formatCount(player.chronoCores)).c_str(),
           760, 205, 18, Color{120, 230, 250, 255});

  if (!unlocked) {
    DrawRectangleRounded({525, 250, 695, 380}, 0.05F, 8, Color{25, 24, 34, 255});
    drawTextCentered("ASCENSION LOCKED", 380, 24, Color{180, 180, 195, 255});
    drawTextCentered("Defeat The Hollow King (Boss 10) to unlock prestige & Ember Perks.", 430, 17, Color{140, 140, 160, 255});
    return;
  }

  const int pendingMarks = ed::calculateAscensionEmberMarks(player.goldEarnedThisRun, player.echoRank);
  const int pendingCores = ed::calculateAscensionChronoCores(player.ascensions, pendingMarks);
  DrawText(("Run Gold: " + ed::formatCount(player.goldEarnedThisRun) + "  ->  +" + ed::formatCount(pendingMarks) + " Marks, +" + std::to_string(pendingCores) + " Cores on Ascend").c_str(),
           525, 228, 14, Color{180, 220, 180, 255});

  const Rectangle ascendBtn{1060, 203, 160, 40};
  const bool hoverAscend = CheckCollisionPointRec(mouse, ascendBtn);
  DrawRectangleRounded(ascendBtn, 0.2F, 4, hoverAscend ? Color{230, 140, 30, 255} : Color{180, 100, 20, 255});
  DrawRectangleRoundedLines(ascendBtn, 0.2F, 4, Color{255, 210, 120, 255});
  DrawText("ASCEND (ENTER)", 1070, 215, 14, WHITE);

  DrawLine(525, 248, 1220, 248, Color{60, 58, 80, 255});

  // Auto-Clicker (Chrono Automaton) Row
  const Rectangle autoCard{520, 252, 705, 54};
  DrawRectangleRounded(autoCard, 0.15F, 4, Color{30, 36, 48, 255});
  DrawRectangleRoundedLines(autoCard, 0.15F, 4, Color{65, 105, 145, 255});

  const int autoLvl = player.autoClickerLevel;
  const double autoSpd = ed::autoClickerSpeed(autoLvl);
  std::string autoTitle = "Chrono Automaton (Auto-Clicker) - Lv " + std::to_string(autoLvl);
  DrawText(autoTitle.c_str(), 535, 259, 16, Color{120, 230, 250, 255});

  if (autoLvl == 0) {
    DrawText("Unlock to automatically mine blocks continuously, even when offline.", 535, 281, 13, Color{165, 175, 190, 255});
    const Rectangle unlockBtn{1040, 260, 175, 38};
    const int cost = ed::autoClickerUpgradeCost(0);
    const bool canAfford = player.chronoCores >= cost;
    const bool hoverUnlock = CheckCollisionPointRec(mouse, unlockBtn);
    DrawRectangleRounded(unlockBtn, 0.2F, 4, canAfford ? (hoverUnlock ? Color{45, 110, 130, 255} : Color{35, 85, 105, 255})
                                                      : Color{45, 50, 60, 255});
    DrawRectangleRoundedLines(unlockBtn, 0.2F, 4, canAfford ? Color{110, 220, 245, 255} : Color{70, 75, 85, 255});
    DrawText("Unlock (1 Core)", 1068, 270, 15, canAfford ? WHITE : Color{140, 140, 150, 255});
  } else {
    char spdBuf[64];
    std::snprintf(spdBuf, sizeof(spdBuf), "Speed: %.1f clicks/sec  |  Earns materials & XP offline", autoSpd);
    DrawText(spdBuf, 535, 281, 13, Color{170, 215, 230, 255});

    // Toggle button
    const Rectangle toggleBtn{925, 260, 105, 38};
    const bool hoverToggle = CheckCollisionPointRec(mouse, toggleBtn);
    DrawRectangleRounded(toggleBtn, 0.2F, 4, player.autoClickerEnabled ? (hoverToggle ? Color{45, 115, 75, 255} : Color{35, 95, 60, 255})
                                                                      : (hoverToggle ? Color{65, 65, 78, 255} : Color{48, 48, 58, 255}));
    DrawRectangleRoundedLines(toggleBtn, 0.2F, 4, player.autoClickerEnabled ? Color{100, 220, 140, 255} : Color{80, 80, 95, 255});
    DrawText(player.autoClickerEnabled ? "ACTIVE" : "OFF", 952, 270, 15, player.autoClickerEnabled ? Color{180, 255, 205, 255} : Color{170, 170, 185, 255});

    // Upgrade button
    const Rectangle upBtn{1040, 260, 175, 38};
    const bool isMax = autoLvl >= 10;
    const int cost = isMax ? 0 : ed::autoClickerUpgradeCost(autoLvl);
    const bool canAfford = !isMax && player.chronoCores >= cost;
    const bool hoverUp = CheckCollisionPointRec(mouse, upBtn);

    if (isMax) {
      DrawRectangleRounded(upBtn, 0.2F, 4, Color{45, 55, 50, 255});
      DrawText("MAX LEVEL", 1085, 270, 15, Color{120, 180, 140, 255});
    } else {
      DrawRectangleRounded(upBtn, 0.2F, 4, canAfford ? (hoverUp ? Color{45, 110, 130, 255} : Color{35, 85, 105, 255})
                                                    : Color{45, 50, 60, 255});
      DrawRectangleRoundedLines(upBtn, 0.2F, 4, canAfford ? Color{110, 220, 245, 255} : Color{70, 75, 85, 255});
      std::string upText = "+" + std::to_string(cost) + " Cores (Lv." + std::to_string(autoLvl + 1) + ")";
      DrawText(upText.c_str(), 1055, 270, 15, canAfford ? WHITE : Color{140, 140, 150, 255});
    }
  }

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

  int py = 316;
  for (std::size_t i = 0; i < 6; ++i) {
    const auto& row = rows[i];
    DrawRectangleRounded({520, static_cast<float>(py), 705, 51}, 0.15F, 4, Color{28, 27, 38, 255});
    DrawText(row.name.c_str(), 535, py + 6, 15, row.color);
    DrawText(row.desc.c_str(), 535, py + 26, 12, Color{175, 175, 190, 255});

    DrawText(("Rank " + std::to_string(row.rank) + "/" + std::to_string(row.maxRank)).c_str(),
             835, py + 16, 14, Color{210, 210, 225, 255});

    const Rectangle upgradeBtn{1040, static_cast<float>(py + 7), 175, 36};
    const bool isMax = (row.rank >= row.maxRank);
    const int cost = isMax ? 0 : ed::perkUpgradeCost(row.id, row.rank);
    const bool canAfford = (!isMax && player.emberMarks >= cost);

    if (isMax) {
      DrawRectangleRounded(upgradeBtn, 0.2F, 4, Color{45, 55, 50, 255});
      DrawText("MAX RANK", 1085, py + 16, 14, Color{120, 180, 140, 255});
    } else {
      const bool hover = CheckCollisionPointRec(mouse, upgradeBtn);
      DrawRectangleRounded(upgradeBtn, 0.2F, 4,
                           canAfford ? (hover ? Color{70, 120, 85, 255} : Color{50, 95, 65, 255})
                                     : Color{55, 45, 50, 255});
      DrawRectangleRoundedLines(upgradeBtn, 0.2F, 4,
                                canAfford ? Color{100, 200, 130, 255} : Color{80, 70, 75, 255});
      const std::string btnText = "+" + ed::formatCount(cost) + " Marks (" + std::to_string(i + 1) + ")";
      DrawText(btnText.c_str(), 1055, py + 16, 14, canAfford ? WHITE : Color{140, 130, 135, 255});
    }

    py += 56;
  }

  if (app.ascensionConfirm) {
    DrawRectangle(0, 0, 1280, 720, Color{0, 0, 0, 215});
    const Rectangle modalRec{370, 190, 540, 330};
    DrawRectangleRounded(modalRec, 0.08F, 8, Color{28, 26, 38, 255});
    DrawRectangleRoundedLines(modalRec, 0.08F, 8, Color{248, 160, 40, 255});

    DrawText("CONFIRM ASCENSION", 480, 220, 24, Color{255, 215, 90, 255});
    DrawText(("You will receive: +" + ed::formatCount(pendingMarks) + " Ember Marks, +" + std::to_string(pendingCores) + " Chrono Cores.").c_str(),
             410, 270, 17, Color{255, 230, 140, 255});
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

} // namespace ed::app
