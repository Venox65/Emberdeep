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
#include "ed/formulas.hpp"
#include "ed/postgame.hpp"

namespace ed::app {

namespace {

std::string getCurrentDateString() {
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm* tm = std::localtime(&t);
  char buf[16];
  std::strftime(buf, sizeof(buf), "%Y-%m-%d", tm);
  return std::string(buf);
}

} // namespace
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
    st.gemHollowSwingsRemaining = 40 + st.gemHollowStreak + (app.game.player.vows.gemWhisperer >= 1 ? 3 : 0);
    st.gemHollowTodayGems = 0;
    st.gemHollowTodayDust = 0;
    st.gemHollowTodayGold = 0;

    st.gemHollowVeins.clear();
    for (int i = 0; i < 12; ++i) {
      st.gemHollowVeins.push_back(app.rng.nextRange(1, 3));
    }
  }

  DrawText(("Swings Remaining: " + std::to_string(st.gemHollowSwingsRemaining)).c_str(), 50, 150, 20, RAYWHITE);
  DrawText(("Today's Loot: " + ed::formatCount(st.gemHollowTodayGems) + " Gems | " +
            ed::formatCount(st.gemHollowTodayDust) + " Dust | " +
            ed::formatCount(st.gemHollowTodayGold) + " Gold").c_str(), 50, 180, 20, Color{248, 211, 89, 255});

  // Gem Refinery Button
  const Rectangle refBtn{960, 145, 230, 42};
  const Vector2 mousePos = GetMousePosition();
  const bool hRef = CheckCollisionPointRec(mousePos, refBtn);
  DrawRectangleRounded(refBtn, 0.2F, 4, hRef ? Color{55, 115, 80, 255} : Color{40, 85, 60, 255});
  DrawRectangleRoundedLines(refBtn, 0.2F, 4, Color{110, 230, 140, 255});
  const char* refTxt = "GEM REFINERY (R)";
  DrawText(refTxt, static_cast<int>(refBtn.x + (230 - MeasureText(refTxt, 16)) / 2), static_cast<int>(refBtn.y + 13), 16, WHITE);
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hRef) {
    app.gemRefineModalOpen = true;
  }

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
                int gemRoll = app.rng.nextRange(0, 999);
                std::string gemId;
                if (gemRoll < 550) {
                  gemId = (app.rng.nextRange(0, 1) == 0) ? "amber" : "garnet";
                } else if (gemRoll < 830) {
                  gemId = (app.rng.nextRange(0, 1) == 0) ? "jade" : "moonstone";
                } else if (gemRoll < 950) {
                  gemId = (app.rng.nextRange(0, 1) == 0) ? "sapphire" : "bloodstone";
                } else if (gemRoll < 992) {
                  gemId = (app.rng.nextRange(0, 1) == 0) ? "star-ruby" : "arcane-opal";
                } else {
                  gemId = (app.rng.nextRange(0, 1) == 0) ? "dragon-eye" : "aether-diamond";
                }
                app.inventory.gems[gemId]++;
                app.gemHollowSummaryItems.push_back("Gem: " + gemId);
                setNotice(app, "Found a " + gemId + "!");
              } else if (roll < 80) {
                int dust = 2 + app.rng.nextRange(0, 4);
                st.gemHollowTodayDust += dust;
                st.arcaneDust += dust;
                app.gemHollowSummaryItems.push_back("Arcane Dust x" + ed::formatCount(dust));
                setNotice(app, "Found " + ed::formatCount(dust) + " Arcane Dust!");
              } else {
                int gold = 50 + app.rng.nextRange(0, 49);
                st.gemHollowTodayGold += gold;
                st.gold += gold;
                app.gemHollowSummaryItems.push_back("Gold x" + ed::formatCount(gold));
                setNotice(app, "Found " + ed::formatCount(gold) + " Gold!");
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
    DrawText(("Gems found: " + ed::formatCount(st.gemHollowTodayGems)).c_str(), 330, 250, 18, RAYWHITE);
    DrawText(("Arcane Dust: " + ed::formatCount(st.gemHollowTodayDust)).c_str(), 330, 278, 18, Color{180, 220, 255, 255});
    DrawText(("Gold: " + ed::formatCount(st.gemHollowTodayGold)).c_str(), 330, 306, 18, Color{248, 211, 89, 255});
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

  const int viewX = 55;
  const int viewY = 185;
  const int viewW = 520;
  const int viewH = 450;
  const int totalHeight = static_cast<int>(app.inventory.gear.size()) * 42;
  const int maxScroll = std::max(0, totalHeight - viewH);
  app.enchantingScroll = std::clamp(app.enchantingScroll, 0, maxScroll);

  if (app.selectedGear >= 0 && app.selectedGear < static_cast<int>(app.inventory.gear.size())) {
    const int selTop = app.selectedGear * 42;
    const int selBottom = selTop + 38;
    if (selTop - app.enchantingScroll < 0) {
      app.enchantingScroll = selTop;
    } else if (selBottom - app.enchantingScroll > viewH) {
      app.enchantingScroll = selBottom - viewH;
    }
    app.enchantingScroll = std::clamp(app.enchantingScroll, 0, maxScroll);
  }

  BeginScissorMode(viewX, viewY, viewW, viewH);
  for (size_t i = 0; i < app.inventory.gear.size(); ++i) {
    const auto& piece = app.inventory.gear[i];
    const bool selected = app.selectedGear == static_cast<int>(i);
    const int itemY = viewY + static_cast<int>(i) * 42 - app.enchantingScroll;
    if (itemY + 38 < viewY || itemY > viewY + viewH) continue;

    Color bg = selected ? Color{112, 76, 45, 255} : Color{48, 47, 61, 255};
    DrawRectangleRounded({65, static_cast<float>(itemY), 495, 34}, 0.15F, 4, bg);
    std::string label = piece.name + " +" + std::to_string(piece.upgradeLevel) + "  [" + piece.slot + "]";
    if (!piece.enchantment.empty()) label += "  *" + piece.enchantment + "*";
    DrawText(label.c_str(), 75, itemY + 8, 15, selected ? RAYWHITE : Color{200, 200, 210, 255});

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse = GetMousePosition();
      if (mouse.x >= 65 && mouse.x <= 560 && mouse.y >= itemY && mouse.y <= itemY + 34 && mouse.y >= viewY && mouse.y <= viewY + viewH) {
        app.selectedGear = static_cast<int>(i);
      }
    }
  }
  EndScissorMode();

  if (maxScroll > 0) {
    const float trackH = static_cast<float>(viewH);
    const float thumbH = std::max(24.0F, trackH * (static_cast<float>(viewH) / static_cast<float>(totalHeight)));
    const float thumbY = static_cast<float>(viewY) + (static_cast<float>(app.enchantingScroll) / static_cast<float>(maxScroll)) * (trackH - thumbH);
    DrawRectangleRounded({565, static_cast<float>(viewY), 6, trackH}, 0.5F, 2, Color{38, 37, 50, 200});
    DrawRectangleRounded({565, thumbY, 6, thumbH}, 0.5F, 2, Color{120, 115, 140, 255});
  }

  DrawText("W / S, wheel, or click to select", 70, 648, 14, Color{130, 130, 145, 255});

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
    DrawText(("  Arcane Dust: " + ed::formatCount(dustCost) +
              "  (have: " + ed::formatCount(app.game.player.arcaneDust) + ")").c_str(),
             630, 345, 16, dustColor);
    DrawText(("  Gold: " + ed::formatCount(goldCost) +
              "  (have: " + ed::formatCount(app.game.player.gold) + ")").c_str(),
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
  return app.inventory.gear.empty() ? gearStart + 30 : gearStart + gearRows * 148 + 45;
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
  const int selectedTop = gearStart + (app.selectedGear / 3) * 148;
  const int selectedBottom = selectedTop + 130;
  if (selectedTop - app.inventoryScroll < viewportTop) {
    app.inventoryScroll = std::max(0, selectedTop - viewportTop);
  } else if (selectedBottom - app.inventoryScroll > viewportBottom) {
    app.inventoryScroll = selectedBottom - viewportBottom;
  }
  clampInventoryScroll(app);
}

// Returns the colour used to represent a gem by its rarity string (placeholder)
static Color gemRarityColor(const std::string& rarity) {
  if (rarity == "uncommon") return Color{72, 200, 88, 255};
  if (rarity == "rare")     return Color{60, 130, 240, 255};
  if (rarity == "epic")     return Color{180, 80, 230, 255};
  if (rarity == "legendary") return Color{255, 195, 40, 255};
  return Color{160, 160, 175, 255}; // common
}

void drawInventoryScreen(const AppState& app) {
  drawTextCentered("INVENTORY", 108, 34, Color{224, 187, 106, 255});
  DrawText("Mouse wheel or W/S scroll. A/D select. E equip. U upgrade. Click gem slots. Right-click to remove. N enchant.", 70, 138, 13, Color{174, 174, 187, 255});
  BeginScissorMode(0, 160, 1280, 500);
  const int offset = app.inventoryScroll;
  DrawText("Resources", 70, 175 - offset, 24, Color{235, 224, 191, 255});
  int resHeaderX = 250;
  if (app.game.player.aetherShards > 0) {
    DrawText(("Aether Shards: " + ed::formatCount(app.game.player.aetherShards)).c_str(),
             resHeaderX, 178 - offset, 18, Color{124, 247, 224, 255});
    resHeaderX += 200;
  }
  if (app.game.player.echoSigils > 0) {
    DrawText(("Echo Sigils: " + ed::formatCount(app.game.player.echoSigils)).c_str(),
             resHeaderX, 178 - offset, 18, Color{218, 160, 255, 255});
    resHeaderX += 190;
  }
  if (app.game.player.arcaneDust > 0) {
    DrawText(("Arcane Dust: " + ed::formatCount(app.game.player.arcaneDust)).c_str(),
             resHeaderX, 178 - offset, 18, Color{195, 175, 245, 255});
  }
  int y = 215;
  const auto drawBucket = [&](const char* title, const std::map<std::string, int>& bucket, bool isIngot = false) {
    DrawText(title, 70, y - offset, 18, Color{174, 174, 187, 255});
    if (std::string(title) == "Gems") {
      const Rectangle refBtn{135, static_cast<float>(y - offset - 4), 105, 26};
      const Vector2 mouse = GetMousePosition();
      const bool hRef = CheckCollisionPointRec(mouse, refBtn);
      DrawRectangleRounded(refBtn, 0.2F, 4, hRef ? Color{55, 115, 80, 255} : Color{40, 85, 60, 255});
      DrawRectangleRoundedLines(refBtn, 0.2F, 4, Color{110, 230, 140, 255});
      const char* refTxt = "REFINE (R)";
      DrawText(refTxt, static_cast<int>(refBtn.x + (105 - MeasureText(refTxt, 12)) / 2), static_cast<int>(refBtn.y + 7), 12, WHITE);
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hRef) {
        const_cast<AppState&>(app).gemRefineModalOpen = true;
      }
    }
    int x = 250;
    for (const auto& [id, count] : bucket) {
      const bool isGems = (std::string(title) == "Gems");
      const Rectangle cardRec{static_cast<float>(x), static_cast<float>(y - 5 - offset), 220, 44};
      const Vector2 mouse = GetMousePosition();
      const bool hover = isGems && CheckCollisionPointRec(mouse, cardRec);
      DrawRectangleRounded(cardRec, 0.18F, 6, hover ? Color{65, 75, 85, 255} : Color{52, 52, 67, 255});
      if (hover) {
        DrawRectangleRoundedLines(cardRec, 0.18F, 6, Color{120, 210, 160, 255});
      }

      if (isIngot) {
        std::string safeId = id;
        for (char& c : safeId) if (c == '-') c = '_';
        std::string texPath = "sprites/Materials/" + safeId + "_ingot.png";
        Texture2D tex = getTexture(texPath);
        int frame = static_cast<int>(GetTime() * 4) % 4;
        Rectangle source = {static_cast<float>(frame * 32), 0, 32, 32};
        Rectangle dest = {static_cast<float>(x + 4), static_cast<float>(y + 1 - offset), 32, 32};
        DrawTexturePro(tex, source, dest, {0,0}, 0.0f, WHITE);

        DrawText((id + " x" + ed::formatCount(count)).c_str(), x + 40, y + 8 - offset, 16, RAYWHITE);
      } else {
        DrawText((id + " x" + ed::formatCount(count)).c_str(), x + 12, y + 8 - offset, 16, RAYWHITE);
      }

      if (isGems && hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const_cast<AppState&>(app).gemRefineModalOpen = true;
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
    // Bigger cards to accommodate gem socket slots
    constexpr int cardWidth = 390;
    constexpr int cardHeight = 130;
    constexpr int rowHeight = 148;
    constexpr int kCols = 3;
    for (std::size_t index = 0; index < app.inventory.gear.size(); ++index) {
      const auto& piece = app.inventory.gear[static_cast<std::size_t>(index)];
      const int column = static_cast<int>(index % kCols);
      const int row    = static_cast<int>(index / kCols);
      const bool selected = static_cast<int>(index) == app.selectedGear;
      const bool isMythic = (piece.upgradeLevel >= 11);

      const float cardX = 30.0F + static_cast<float>(column) * (cardWidth + 15);
      const float cardY = static_cast<float>(y + row * rowHeight - offset);

      Color cardBg = selected ? Color{112, 76, 45, 255} : (isMythic ? Color{58, 42, 72, 255} : Color{52, 52, 67, 255});
      DrawRectangleRounded({cardX, cardY, static_cast<float>(cardWidth), static_cast<float>(cardHeight)},
                           0.10F, 6, cardBg);

      if (isMythic) {
        const float pulse = (std::sin(static_cast<float>(GetTime()) * 4.0F) + 1.0F) * 0.5F;
        const Color glow = ColorAlpha(Color{205, 140, 255, 255}, 0.5F + 0.5F * pulse);
        DrawRectangleRoundedLines({cardX, cardY, static_cast<float>(cardWidth), static_cast<float>(cardHeight)},
                                  0.10F, 6, glow);
      }

      // Equipment placeholder: coloured rect with slot label on the left
      // Green if this piece is the currently equipped item for its slot
      const auto& eq = app.inventory.equipped;
      const bool isEquipped = (piece.slot == "pickaxe"   && eq.pickaxeId   == piece.itemId) ||
                              (piece.slot == "helm"       && eq.helmId       == piece.itemId) ||
                              (piece.slot == "chestplate" && eq.chestplateId == piece.itemId) ||
                              (piece.slot == "gauntlets"  && eq.gauntletsId  == piece.itemId) ||
                              (piece.slot == "boots"      && eq.bootsId      == piece.itemId) ||
                              (piece.slot == "amulet"     && eq.amuletId     == piece.itemId);
      const Color equippedTint = isEquipped ? Color{80, 180, 110, 255} : Color{70, 68, 88, 255};
      DrawRectangleRounded({cardX + 8, cardY + 8, 56, 56}, 0.18F, 6, equippedTint);
      const std::string slotLabelStr = piece.slot.size() > 5 ? piece.slot.substr(0, 5) : piece.slot;
      DrawText(slotLabelStr.c_str(), static_cast<int>(cardX + 10), static_cast<int>(cardY + 28), 12, RAYWHITE);

      // Name + tier row
      std::string nameLabel = piece.name + " +" + std::to_string(piece.upgradeLevel);
      if (isMythic) nameLabel += " [M]";
      DrawText(nameLabel.c_str(),
               static_cast<int>(cardX + 72), static_cast<int>(cardY + 10), 16,
               isMythic ? Color{245, 215, 255, 255} : RAYWHITE);

      std::string subLabel = "Tier " + std::to_string(piece.tier) + " | " + piece.slot;
      DrawText(subLabel.c_str(),
               static_cast<int>(cardX + 72), static_cast<int>(cardY + 32), 13,
               Color{180, 180, 192, 255});

      // Enchantment line
      if (!piece.enchantment.empty()) {
        DrawText(piece.enchantment.c_str(),
                 static_cast<int>(cardX + 72), static_cast<int>(cardY + 52), 12,
                 Color{160, 220, 255, 255});
      }

      // Gem socket slots row
      if (piece.sockets > 0) {
        DrawText("Gems:", static_cast<int>(cardX + 8), static_cast<int>(cardY + 75), 12,
                 Color{174, 174, 187, 255});
        constexpr int slotSize = 28;
        constexpr int slotSpacing = 34;
        for (int s = 0; s < piece.sockets; ++s) {
          const int sx = static_cast<int>(cardX + 56 + s * slotSpacing);
          const int sy = static_cast<int>(cardY + 70);
          const bool filled = s < static_cast<int>(piece.socketedGems.size());
          if (filled) {
            // Find gem rarity for colour
            Color gemCol = Color{160, 160, 175, 255};
            const std::string baseId = ed::baseGemId(piece.socketedGems[static_cast<std::size_t>(s)]);
            for (const auto& gd : app.gems) {
              if (gd.id == baseId) {
                gemCol = gemRarityColor(gd.rarity);
                break;
              }
            }
            DrawCircle(sx + slotSize / 2, sy + slotSize / 2, slotSize / 2.0F, gemCol);
            // gem id abbreviated
            const std::string& gid = piece.socketedGems[static_cast<std::size_t>(s)];
            const std::string abbr = gid.size() > 2 ? gid.substr(0, 2) : gid;
            const int tw = MeasureText(abbr.c_str(), 11);
            drawTextWithOutline(abbr.c_str(), sx + (slotSize - tw) / 2, sy + 8, 11, RAYWHITE, BLACK);
          } else {
            // Active slot being chosen
            const bool isActive = app.gemSlotMenuOpen &&
                                  app.gemSlotMenuGearIndex == static_cast<int>(index) &&
                                  app.gemSlotMenuSocketIndex == s;
            DrawCircle(sx + slotSize / 2, sy + slotSize / 2, slotSize / 2.0F,
                       isActive ? Color{80, 100, 160, 255} : Color{42, 42, 55, 255});
            DrawCircleLines(sx + slotSize / 2, sy + slotSize / 2, slotSize / 2.0F,
                            isActive ? Color{120, 160, 255, 255} : Color{90, 90, 110, 255});
            DrawText("+", sx + 10, sy + 7, 13, Color{130, 130, 150, 255});
          }
        }
      } else {
        DrawText("No sockets", static_cast<int>(cardX + 8), static_cast<int>(cardY + 76), 12,
                 Color{110, 110, 130, 255});
      }
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
                     ed::formatCount(ed::upgradeGoldCost(piece)) + "g, " +
                     ed::formatCount(ed::upgradeDustCost(piece)) + " dust, " +
                     ed::formatCount(ed::upgradeAetherShardCost(piece)) + " shard(s), " +
                     ed::formatCount(ed::upgradeEchoSigilCost(piece)) + " sigil(s)";
      } else {
        statusText = piece.name + " +10  [MAX STANDARD] (Mythic upgrades unlock after Boss 10)";
      }
    } else {
      statusText = "Upgrade [U] -> +" + std::to_string(piece.upgradeLevel + 1) + ": " +
                   ed::formatCount(ed::upgradeGoldCost(piece)) + "g, " +
                   ed::formatCount(ed::upgradeDustCost(piece)) + " dust";
    }
    DrawText(statusText.c_str(), 70, 672, 16, Color{225, 210, 160, 255});
  }
}

static Color gemSpecificColor(const std::string& baseId) {
  if (baseId == "amber")          return Color{245, 165, 35, 255};  // Golden Amber
  if (baseId == "garnet")         return Color{195, 35, 55, 255};   // Deep Garnet Red
  if (baseId == "jade")           return Color{40, 190, 95, 255};   // Vibrant Jade Green
  if (baseId == "moonstone")      return Color{170, 225, 240, 255}; // Pearl Cyan Moonstone
  if (baseId == "sapphire")       return Color{45, 120, 250, 255};  // Royal Sapphire Blue
  if (baseId == "bloodstone")     return Color{150, 25, 40, 255};   // Crimson Bloodstone
  if (baseId == "star-ruby")      return Color{235, 50, 130, 255};  // Star Ruby Pink/Magenta
  if (baseId == "arcane-opal")    return Color{175, 95, 245, 255};  // Arcane Opal Violet
  if (baseId == "dragon-eye")     return Color{250, 95, 25, 255};   // Fiery Dragon's Eye Orange
  if (baseId == "aether-diamond") return Color{130, 250, 245, 255}; // Prismatic Diamond Teal
  return Color{160, 160, 180, 255};
}

static Color gemTierColor(ed::GemCut cut) {
  switch (cut) {
    case ed::GemCut::Rough:    return Color{205, 135, 75, 255};  // Bronze
    case ed::GemCut::Cut:      return Color{90, 195, 255, 255};  // Azure
    case ed::GemCut::Flawless: return Color{255, 215, 60, 255};  // Gold
  }
  return Color{160, 160, 175, 255};
}

void drawGemRadialMenu(AppState& app) {
  if (!app.gemSlotMenuOpen) return;
  if (app.gemSlotMenuGearIndex < 0 ||
      app.gemSlotMenuGearIndex >= static_cast<int>(app.inventory.gear.size())) {
    app.gemSlotMenuOpen = false;
    return;
  }

  auto& piece = app.inventory.gear[static_cast<std::size_t>(app.gemSlotMenuGearIndex)];
  const int targetSocket = app.gemSlotMenuSocketIndex;
  if (targetSocket < 0 || targetSocket >= piece.sockets) {
    app.gemSlotMenuOpen = false;
    return;
  }

  const bool hasSocketedGem = (targetSocket < static_cast<int>(piece.socketedGems.size()));
  const std::string socketedGemId = hasSocketedGem ? piece.socketedGems[static_cast<std::size_t>(targetSocket)] : "";
  const std::string socketedGemName = hasSocketedGem ? ed::gemDisplayName(socketedGemId) : "";

  struct RadialTierItem {
    std::string id;
    ed::GemCut cut;
    const char* label;
    int count = 0;
    int multiplier = 1;
    std::string effect;
  };

  struct RadialBaseItem {
    const GemDef* def = nullptr;
    Color color;
    Color rarityColor;
    int totalCount = 0;
    int roughCount = 0;
    int cutCount = 0;
    int flawlessCount = 0;
    std::vector<RadialTierItem> tiers;
  };

  std::vector<RadialBaseItem> baseItems;
  baseItems.reserve(app.gems.size());
  for (const auto& gd : app.gems) {
    const int roughCnt = ed::countRoughGems(app.inventory, gd.id);
    const int cutCnt = ed::countItem(app.inventory, "gems", "cut-" + gd.id);
    const int flawlessCnt = ed::countItem(app.inventory, "gems", "flawless-" + gd.id);

    RadialBaseItem baseItem;
    baseItem.def = &gd;
    baseItem.color = gemSpecificColor(gd.id);
    baseItem.rarityColor = gemRarityColor(gd.rarity);
    baseItem.roughCount = roughCnt;
    baseItem.cutCount = cutCnt;
    baseItem.flawlessCount = flawlessCnt;
    baseItem.tiers.reserve(3);

    if (app.gemRadialFilterTier == 1) { // Rough only
      if (roughCnt > 0) {
        baseItem.totalCount = roughCnt;
        baseItem.tiers.push_back({gd.id, ed::GemCut::Rough, "Rough (x1)", roughCnt, 1, gd.effect});
        baseItems.push_back(std::move(baseItem));
      }
    } else if (app.gemRadialFilterTier == 2) { // Cut only
      if (cutCnt > 0) {
        baseItem.totalCount = cutCnt;
        baseItem.tiers.push_back({"cut-" + gd.id, ed::GemCut::Cut, "Cut (x2)", cutCnt, 2, gd.effect + " (x2 Cut)"});
        baseItems.push_back(std::move(baseItem));
      }
    } else if (app.gemRadialFilterTier == 3) { // Flawless only
      if (flawlessCnt > 0) {
        baseItem.totalCount = flawlessCnt;
        baseItem.tiers.push_back({"flawless-" + gd.id, ed::GemCut::Flawless, "Flawless (x4)", flawlessCnt, 4, gd.effect + " (x4 Flawless)"});
        baseItems.push_back(std::move(baseItem));
      }
    } else { // All gems
      const int tot = roughCnt + cutCnt + flawlessCnt;
      if (tot > 0) {
        baseItem.totalCount = tot;
        if (roughCnt > 0) {
          baseItem.tiers.push_back({gd.id, ed::GemCut::Rough, "Rough (x1)", roughCnt, 1, gd.effect});
        }
        if (cutCnt > 0) {
          baseItem.tiers.push_back({"cut-" + gd.id, ed::GemCut::Cut, "Cut (x2)", cutCnt, 2, gd.effect + " (x2 Cut)"});
        }
        if (flawlessCnt > 0) {
          baseItem.tiers.push_back({"flawless-" + gd.id, ed::GemCut::Flawless, "Flawless (x4)", flawlessCnt, 4, gd.effect + " (x4 Flawless)"});
        }
        baseItems.push_back(std::move(baseItem));
      }
    }
  }

  // Dim background overlay
  DrawRectangle(0, 0, kWidth, kHeight, Color{0, 0, 0, 190});

  const Vector2 mouse = GetMousePosition();
  bool leftClick = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
  if (app.gemSlotMenuJustOpened) {
    app.gemSlotMenuJustOpened = false;
    leftClick = false;
  }

  // Close Button [X] at top-right
  const Rectangle closeBtn{1180, 24, 34, 34};
  const bool hClose = CheckCollisionPointRec(mouse, closeBtn);
  DrawRectangleRounded(closeBtn, 0.25F, 4, hClose ? Color{130, 45, 55, 255} : Color{50, 40, 52, 255});
  DrawRectangleRoundedLines(closeBtn, 0.25F, 4, hClose ? Color{240, 110, 120, 255} : Color{100, 90, 110, 255});
  drawTextWithOutline("X", static_cast<int>(closeBtn.x + 11), static_cast<int>(closeBtn.y + 8), 16, RAYWHITE, BLACK);
  if (leftClick && hClose) {
    app.gemSlotMenuOpen = false;
    return;
  }

  // Header Title & Gear Details
  const char* title = "GEM SOCKETING WORKBENCH";
  drawTextCentered(title, 30, 22, Color{248, 220, 110, 255});

  std::string gearSubtitle = piece.name + " (+" + std::to_string(piece.upgradeLevel) + ")  •  Socket " +
                             std::to_string(targetSocket + 1) + " of " + std::to_string(piece.sockets);
  if (hasSocketedGem) {
    gearSubtitle += "  •  Installed: " + socketedGemName;
  } else {
    gearSubtitle += "  •  Installed: [Empty Socket]";
  }
  drawTextCentered(gearSubtitle.c_str(), 58, 14, Color{195, 195, 215, 255});

  // Filter Tabs
  struct FilterTab { const char* label; int tier; float w; };
  const FilterTab tabs[4] = {
    {"ALL GEMS (1)", 0, 135.0F},
    {"ROUGH (2)", 1, 120.0F},
    {"CUT x2 (3)", 2, 120.0F},
    {"FLAWLESS x4 (4)", 3, 150.0F}
  };
  constexpr float tabH = 26.0F;
  constexpr float tabSpacing = 8.0F;
  float totalTabsW = 0.0F;
  for (const auto& tab : tabs) totalTabsW += tab.w;
  totalTabsW += 3.0F * tabSpacing;
  float tabCurX = (static_cast<float>(kWidth) - totalTabsW) / 2.0F;
  constexpr float tabY = 88.0F;

  for (const auto& tab : tabs) {
    const Rectangle tabRec{tabCurX, tabY, tab.w, tabH};
    const bool isSelected = (app.gemRadialFilterTier == tab.tier);
    const bool hTab = CheckCollisionPointRec(mouse, tabRec);

    Color bg = isSelected ? Color{65, 55, 32, 255} : (hTab ? Color{42, 38, 55, 255} : Color{26, 24, 36, 255});
    Color border = isSelected ? Color{245, 215, 100, 255} : (hTab ? Color{140, 130, 160, 255} : Color{60, 58, 75, 255});
    Color textCol = isSelected ? Color{255, 240, 150, 255} : (hTab ? WHITE : Color{175, 175, 190, 255});

    DrawRectangleRounded(tabRec, 0.25F, 4, bg);
    DrawRectangleRoundedLines(tabRec, 0.25F, 4, border);
    const int tw = MeasureText(tab.label, 12);
    DrawText(tab.label, static_cast<int>(tabRec.x + (tabRec.width - tw) / 2.0F), static_cast<int>(tabRec.y + 7), 12, textCol);

    if (leftClick && hTab) {
      app.gemRadialFilterTier = tab.tier;
      app.gemRadialSelectedBaseIndex = -1;
      return;
    }
    tabCurX += tab.w + tabSpacing;
  }

  // Radial Geometry Setup
  const Vector2 center{640.0F, 385.0F};
  constexpr float kRadius = 180.0F;
  constexpr float kNodeR  = 30.0F;

  // Background Guide Rings
  DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), kRadius, Color{55, 50, 72, 100});
  DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y), kRadius + 34.0F, Color{45, 42, 60, 60});

  // Center Hub Rectangle
  const Rectangle hubRec{center.x - 116.0F, center.y - 88.0F, 232.0F, 176.0F};
  DrawRectangleRounded(hubRec, 0.12F, 6, Color{22, 20, 32, 248});
  DrawRectangleRoundedLines(hubRec, 0.12F, 6, Color{80, 74, 105, 255});

  int hoveredBaseIdx = -1;
  int hoveredSatelliteTier = -1;
  const int n = static_cast<int>(baseItems.size());

  // 1st Pass: Collision Detection for Base Nodes & Satellites
  for (int i = 0; i < n; ++i) {
    const float angle = static_cast<float>(i) / static_cast<float>(n) * 2.0F * PI - PI / 2.0F;
    const float nx = center.x + kRadius * std::cos(angle);
    const float ny = center.y + kRadius * std::sin(angle);

    const float dx = mouse.x - nx;
    const float dy = mouse.y - ny;
    if ((dx * dx + dy * dy) <= (kNodeR + 4.0F) * (kNodeR + 4.0F)) {
      hoveredBaseIdx = i;
    }

    // Check satellites if this gem is active or hovered
    const bool isFocused = (hoveredBaseIdx == i || app.gemRadialSelectedBaseIndex == i);
    const auto& bItem = baseItems[static_cast<std::size_t>(i)];
    if (isFocused && bItem.tiers.size() > 1) {
      const int numTiers = static_cast<int>(bItem.tiers.size());
      for (int t = 0; t < numTiers; ++t) {
        const float satAngle = angle + (static_cast<float>(t) - static_cast<float>(numTiers - 1) / 2.0F) * 0.32F;
        const float sx = nx + 48.0F * std::cos(satAngle);
        const float sy = ny + 48.0F * std::sin(satAngle);
        const float sdx = mouse.x - sx;
        const float sdy = mouse.y - sy;
        if ((sdx * sdx + sdy * sdy) <= 18.0F * 18.0F) {
          hoveredBaseIdx = i;
          hoveredSatelliteTier = t;
          break;
        }
      }
    }
  }

  // Update selected base gem on hover so moving to hub retains selection
  if (hoveredBaseIdx >= 0) {
    app.gemRadialSelectedBaseIndex = hoveredBaseIdx;
  }
  if (app.gemRadialSelectedBaseIndex >= n) {
    app.gemRadialSelectedBaseIndex = -1;
  }

  // Active Index is either currently hovered or previously locked
  const int activeBaseIdx = (hoveredBaseIdx >= 0) ? hoveredBaseIdx : app.gemRadialSelectedBaseIndex;

  // Detect hover over Center Hub tier buttons
  int hoveredHubTier = -1;
  if (activeBaseIdx >= 0 && activeBaseIdx < n) {
    const auto& act = baseItems[static_cast<std::size_t>(activeBaseIdx)];
    const int numTiers = static_cast<int>(act.tiers.size());
    const float btnH = (numTiers >= 3) ? 26.0F : 30.0F;
    const float btnSpacing = (numTiers >= 3) ? 4.0F : 6.0F;
    for (int t = 0; t < numTiers; ++t) {
      const Rectangle tierBtn{hubRec.x + 14.0F, hubRec.y + 48.0F + static_cast<float>(t) * (btnH + btnSpacing),
                              hubRec.width - 28.0F, btnH};
      if (CheckCollisionPointRec(mouse, tierBtn)) {
        hoveredHubTier = t;
        break;
      }
    }
  }

  // 2nd Pass: Render Nodes and Connections
  for (int i = 0; i < n; ++i) {
    const auto& item = baseItems[static_cast<std::size_t>(i)];
    const float angle = static_cast<float>(i) / static_cast<float>(n) * 2.0F * PI - PI / 2.0F;
    const float nx = center.x + kRadius * std::cos(angle);
    const float ny = center.y + kRadius * std::sin(angle);

    const bool isHovered = (hoveredBaseIdx == i);
    const bool isActive  = (activeBaseIdx == i);

    // Connector Line to Center
    DrawLineV(center, {nx, ny}, isActive ? Color{190, 170, 95, 180} : Color{50, 48, 65, 110});

    // Outer Glow / Ring
    if (isActive) {
      DrawCircle(static_cast<int>(nx), static_cast<int>(ny), kNodeR + 5.0F, Color{255, 220, 80, 50});
      DrawCircleLines(static_cast<int>(nx), static_cast<int>(ny), kNodeR + 4.0F, Color{255, 235, 120, 255});
    }

    // Gem Body
    DrawCircle(static_cast<int>(nx), static_cast<int>(ny), kNodeR, item.color);
    DrawCircleLines(static_cast<int>(nx), static_cast<int>(ny), kNodeR,
                    isActive ? Color{255, 255, 200, 255} : item.rarityColor);

    // Specular Highlight
    DrawCircle(static_cast<int>(nx - 8.0F), static_cast<int>(ny - 8.0F), 8.0F, Color{255, 255, 255, 65});

    // Gem 3-letter Abbreviation
    const std::string abbr = item.def->name.size() > 3 ? item.def->name.substr(0, 3) : item.def->name;
    const int abbrW = MeasureText(abbr.c_str(), 13);
    drawTextWithOutline(abbr.c_str(), static_cast<int>(nx) - abbrW / 2, static_cast<int>(ny) - 7, 13, WHITE, BLACK);

    // Top-Right Count Badge
    const std::string cntStr = "x" + std::to_string(item.totalCount);
    const int cntW = MeasureText(cntStr.c_str(), 10);
    const Rectangle badgeRec{nx + 13.0F, ny - 24.0F, static_cast<float>(cntW + 8), 16.0F};
    DrawRectangleRounded(badgeRec, 0.3F, 3, Color{20, 18, 30, 235});
    DrawRectangleRoundedLines(badgeRec, 0.3F, 3, Color{245, 215, 110, 200});
    DrawText(cntStr.c_str(), static_cast<int>(badgeRec.x + 4), static_cast<int>(badgeRec.y + 3), 10, Color{255, 235, 140, 255});

    // Below Node: Gem Name
    const int nw = MeasureText(item.def->name.c_str(), 12);
    drawTextWithOutline(item.def->name.c_str(), static_cast<int>(nx) - nw / 2, static_cast<int>(ny + kNodeR + 4.0F), 12,
                        isActive ? Color{255, 235, 130, 255} : RAYWHITE, BLACK);

    // 3 Cut Pips (Rough, Cut, Flawless)
    const float pipY = ny + kNodeR + 19.0F;
    const Color roughPip = (item.roughCount > 0) ? gemTierColor(ed::GemCut::Rough) : Color{50, 48, 60, 200};
    const Color cutPip   = (item.cutCount > 0) ? gemTierColor(ed::GemCut::Cut) : Color{50, 48, 60, 200};
    const Color flawPip  = (item.flawlessCount > 0) ? gemTierColor(ed::GemCut::Flawless) : Color{50, 48, 60, 200};

    DrawCircle(static_cast<int>(nx - 11.0F), static_cast<int>(pipY), 3.0F, roughPip);
    DrawCircle(static_cast<int>(nx), static_cast<int>(pipY), 3.0F, cutPip);
    DrawCircle(static_cast<int>(nx + 11.0F), static_cast<int>(pipY), 3.0F, flawPip);

    // Render Satellite Buttons if Active with Multiple Tiers
    if (isActive && item.tiers.size() > 1) {
      const int numTiers = static_cast<int>(item.tiers.size());
      for (int t = 0; t < numTiers; ++t) {
        const auto& tier = item.tiers[static_cast<std::size_t>(t)];
        const float satAngle = angle + (static_cast<float>(t) - static_cast<float>(numTiers - 1) / 2.0F) * 0.32F;
        const float sx = nx + 48.0F * std::cos(satAngle);
        const float sy = ny + 48.0F * std::sin(satAngle);
        const bool satH = (hoveredBaseIdx == i && hoveredSatelliteTier == t) || (activeBaseIdx == i && hoveredHubTier == t);

        const Color tCol = gemTierColor(tier.cut);
        DrawCircle(static_cast<int>(sx), static_cast<int>(sy), 14.0F, satH ? ColorBrightness(tCol, 0.25F) : Color{30, 28, 42, 245});
        DrawCircleLines(static_cast<int>(sx), static_cast<int>(sy), 14.0F, satH ? WHITE : tCol);

        const char* initial = (tier.cut == ed::GemCut::Rough) ? "R" : ((tier.cut == ed::GemCut::Cut) ? "C" : "F");
        drawTextWithOutline(initial, static_cast<int>(sx) - 4, static_cast<int>(sy) - 6, 12, satH ? BLACK : tCol, BLACK);

        // Click Satellite Button to Socket!
        if (leftClick && (hoveredBaseIdx == i && hoveredSatelliteTier == t)) {
          socketGemIntoGear(app, app.gemSlotMenuGearIndex, app.gemSlotMenuSocketIndex, tier.id);
          app.gemSlotMenuOpen = false;
          return;
        }
      }
    }

    // Direct Click on Base Node
    if (leftClick && isHovered && hoveredSatelliteTier == -1) {
      if (item.tiers.size() == 1) {
        // Only 1 tier owned: socket it instantly!
        socketGemIntoGear(app, app.gemSlotMenuGearIndex, app.gemSlotMenuSocketIndex, item.tiers[0].id);
        app.gemSlotMenuOpen = false;
        return;
      } else {
        // Multiple tiers owned: focus this base gem
        app.gemRadialSelectedBaseIndex = i;
      }
    }
  }

  // 3rd Pass: Center Hub Contents
  if (baseItems.empty()) {
    drawTextCentered("No Gems Available", static_cast<int>(hubRec.y + 35), 15, Color{225, 180, 180, 255});
    drawTextCentered("Mine in Gem Hollow or explore", static_cast<int>(hubRec.y + 60), 11, Color{165, 165, 180, 255});
    drawTextCentered("dungeons to acquire gems.", static_cast<int>(hubRec.y + 76), 11, Color{165, 165, 180, 255});

    if (hasSocketedGem) {
      const Rectangle unsockBtn{hubRec.x + 30.0F, hubRec.y + 115.0F, hubRec.width - 60.0F, 30.0F};
      const bool hUnsock = CheckCollisionPointRec(mouse, unsockBtn);
      DrawRectangleRounded(unsockBtn, 0.25F, 4, hUnsock ? Color{150, 45, 55, 255} : Color{85, 30, 40, 255});
      DrawRectangleRoundedLines(unsockBtn, 0.25F, 4, hUnsock ? Color{255, 140, 150, 255} : Color{180, 70, 80, 255});
      drawTextCentered("UNSOCKET CURRENT GEM", static_cast<int>(unsockBtn.y + 8), 12, RAYWHITE);
      if (leftClick && hUnsock) {
        unsocketGemFromGear(app, app.gemSlotMenuGearIndex, app.gemSlotMenuSocketIndex);
        app.gemSlotMenuOpen = false;
        return;
      }
    }
  } else if (activeBaseIdx >= 0 && activeBaseIdx < static_cast<int>(baseItems.size())) {
    const auto& act = baseItems[static_cast<std::size_t>(activeBaseIdx)];

    // Gem Header in Center Hub
    const std::string titleStr = act.def->name + " (" + act.def->rarity + ")";
    drawTextCentered(titleStr.c_str(), static_cast<int>(hubRec.y + 12), 15, act.rarityColor);

    const int numTiers = static_cast<int>(act.tiers.size());
    const int effTier = (hoveredSatelliteTier >= 0) ? hoveredSatelliteTier : hoveredHubTier;
    const std::string effStr = (effTier >= 0 && effTier < numTiers)
                                   ? act.tiers[static_cast<std::size_t>(effTier)].effect
                                   : act.def->effect;
    drawTextCentered(effStr.c_str(), static_cast<int>(hubRec.y + 30), 11,
                     (effTier >= 0) ? Color{255, 235, 120, 255} : Color{185, 225, 195, 255});

    // Clickable Tier Options in Center Hub
    float btnY = hubRec.y + 48.0F;
    const float btnH = (numTiers >= 3) ? 26.0F : 30.0F;
    const float btnSpacing = (numTiers >= 3) ? 4.0F : 6.0F;

    for (int t = 0; t < numTiers; ++t) {
      const auto& tier = act.tiers[static_cast<std::size_t>(t)];
      const Rectangle tierBtn{hubRec.x + 14.0F, btnY, hubRec.width - 28.0F, btnH};
      const bool hTier = (hoveredHubTier == t) || (hoveredSatelliteTier == t);
      const Color tCol = gemTierColor(tier.cut);

      DrawRectangleRounded(tierBtn, 0.22F, 3, hTier ? Color{55, 50, 75, 255} : Color{34, 30, 46, 255});
      DrawRectangleRoundedLines(tierBtn, 0.22F, 3, hTier ? Color{255, 235, 120, 255} : tCol);

      // Icon Dot
      DrawCircle(static_cast<int>(tierBtn.x + 14.0F), static_cast<int>(tierBtn.y + btnH / 2.0F), 4.5F, tCol);

      // Label & Count
      DrawText(tier.label, static_cast<int>(tierBtn.x + 26.0F), static_cast<int>(tierBtn.y + (btnH - 12.0F) / 2.0F), 11, hTier ? WHITE : Color{235, 235, 245, 255});
      const std::string ownCount = "x" + std::to_string(tier.count);
      const int cw = MeasureText(ownCount.c_str(), 11);
      DrawText(ownCount.c_str(), static_cast<int>(tierBtn.x + tierBtn.width - cw - 10.0F), static_cast<int>(tierBtn.y + (btnH - 12.0F) / 2.0F), 11, Color{250, 215, 110, 255});

      if (leftClick && (hoveredHubTier == t)) {
        socketGemIntoGear(app, app.gemSlotMenuGearIndex, app.gemSlotMenuSocketIndex, tier.id);
        app.gemSlotMenuOpen = false;
        return;
      }

      btnY += btnH + btnSpacing;
    }

    // Unsocket Option at bottom if slot has existing gem and space permits
    if (hasSocketedGem && btnY <= hubRec.y + hubRec.height - 24.0F) {
      const Rectangle unsockBtn{hubRec.x + 28.0F, hubRec.y + hubRec.height - 24.0F, hubRec.width - 56.0F, 18.0F};
      const bool hUnsock = CheckCollisionPointRec(mouse, unsockBtn);
      DrawRectangleRounded(unsockBtn, 0.2F, 3, hUnsock ? Color{120, 35, 45, 255} : Color{60, 25, 32, 255});
      DrawRectangleRoundedLines(unsockBtn, 0.2F, 3, hUnsock ? Color{240, 110, 120, 255} : Color{140, 50, 60, 255});
      drawTextCentered("UNSOCKET GEM", static_cast<int>(unsockBtn.y + 3), 10, RAYWHITE);
      if (leftClick && hUnsock) {
        unsocketGemFromGear(app, app.gemSlotMenuGearIndex, app.gemSlotMenuSocketIndex);
        app.gemSlotMenuOpen = false;
        return;
      }
    }
  } else {
    // Idle / Default State: Gear & Socket Information
    drawTextCentered(piece.name.c_str(), static_cast<int>(hubRec.y + 20), 14, RAYWHITE);
    const std::string sockStr = "Socket Slot " + std::to_string(targetSocket + 1) + " of " + std::to_string(piece.sockets);
    drawTextCentered(sockStr.c_str(), static_cast<int>(hubRec.y + 40), 12, Color{180, 180, 200, 255});

    if (hasSocketedGem) {
      const std::string curLabel = "Current: " + socketedGemName;
      drawTextCentered(curLabel.c_str(), static_cast<int>(hubRec.y + 64), 12, Color{245, 215, 120, 255});

      const Rectangle unsockBtn{hubRec.x + 30.0F, hubRec.y + 92.0F, hubRec.width - 60.0F, 28.0F};
      const bool hUnsock = CheckCollisionPointRec(mouse, unsockBtn);
      DrawRectangleRounded(unsockBtn, 0.25F, 4, hUnsock ? Color{150, 45, 55, 255} : Color{85, 30, 40, 255});
      DrawRectangleRoundedLines(unsockBtn, 0.25F, 4, hUnsock ? Color{255, 140, 150, 255} : Color{180, 70, 80, 255});
      drawTextCentered("UNSOCKET CURRENT GEM", static_cast<int>(unsockBtn.y + 8), 11, RAYWHITE);
      if (leftClick && hUnsock) {
        unsocketGemFromGear(app, app.gemSlotMenuGearIndex, app.gemSlotMenuSocketIndex);
        app.gemSlotMenuOpen = false;
        return;
      }

      drawTextCentered("Or click any gem to replace it", static_cast<int>(hubRec.y + 134), 11, Color{150, 150, 170, 255});
    } else {
      drawTextCentered("[ Empty Socket Slot ]", static_cast<int>(hubRec.y + 75), 13, Color{140, 140, 160, 255});
      drawTextCentered("Click a gem to socket it", static_cast<int>(hubRec.y + 115), 12, Color{245, 215, 110, 255});
    }
  }

  // Click Outside to Dismiss
  if (leftClick && hoveredBaseIdx == -1 && !CheckCollisionPointRec(mouse, hubRec)) {
    // Check if clicked outside all UI regions
    bool inTabs = false;
    float tx = (static_cast<float>(kWidth) - totalTabsW) / 2.0F;
    for (const auto& tab : tabs) {
      if (CheckCollisionPointRec(mouse, {tx, tabY, tab.w, tabH})) { inTabs = true; break; }
      tx += tab.w + tabSpacing;
    }
    if (!inTabs && !hClose) {
      app.gemSlotMenuOpen = false;
      return;
    }
  }

  // Footer / Keyboard Legend
  const char* footer = "[Left-Click] Socket Gem   •   [1-4 / Tab] Filter Cuts   •   [Delete] Unsocket   •   [Right-Click / ESC] Close";
  drawTextCentered(footer, 680, 13, Color{175, 175, 195, 255});
}

void drawGemRefiningModal(AppState& app) {
  if (!app.gemRefineModalOpen) return;

  // Dim background overlay
  DrawRectangle(0, 0, kWidth, kHeight, Color{0, 0, 0, 175});

  const Rectangle box{180, 65, 920, 590};
  DrawRectangleRounded(box, 0.03F, 6, Color{24, 22, 34, 255});
  DrawRectangleRoundedLines(box, 0.03F, 6, Color{210, 175, 80, 255});

  // Header
  DrawText("GEM REFINERY & WORKSHOP", static_cast<int>(box.x + 30), static_cast<int>(box.y + 20), 22, Color{248, 211, 89, 255});
  DrawText("Combine 3 identical gems of the same cut into the next cut tier (Cut: x2 stats, Flawless: x4 stats).",
           static_cast<int>(box.x + 30), static_cast<int>(box.y + 48), 13, Color{180, 180, 195, 255});

  // Close Button
  const Rectangle closeBtn{box.x + box.width - 45, box.y + 16, 28, 28};
  const Vector2 mouse = GetMousePosition();
  const bool hClose = CheckCollisionPointRec(mouse, closeBtn);
  DrawRectangleRounded(closeBtn, 0.2F, 3, hClose ? Color{120, 45, 55, 255} : Color{60, 35, 45, 255});
  DrawText("X", static_cast<int>(closeBtn.x + 9), static_cast<int>(closeBtn.y + 6), 16, RAYWHITE);
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && hClose) {
    app.gemRefineModalOpen = false;
    return;
  }

  // Currency Row
  std::string dustStr = "Arcane Dust: " + ed::formatCount(app.game.player.arcaneDust);
  std::string goldStr = "Gold: " + ed::formatCount(app.game.player.gold);
  DrawText(dustStr.c_str(), static_cast<int>(box.x + 30), static_cast<int>(box.y + 70), 14, Color{160, 220, 255, 255});
  DrawText(goldStr.c_str(), static_cast<int>(box.x + 230), static_cast<int>(box.y + 70), 14, Color{248, 211, 89, 255});

  DrawLine(static_cast<int>(box.x + 25), static_cast<int>(box.y + 92),
           static_cast<int>(box.x + box.width - 25), static_cast<int>(box.y + 92), Color{60, 58, 80, 255});

  // Viewport for the 10 gem types
  const int viewX = static_cast<int>(box.x + 20);
  const int viewY = static_cast<int>(box.y + 100);
  const int viewW = static_cast<int>(box.width - 40); // 880
  const int viewH = 450;
  const int cardH = 98;
  const int spacing = 8;
  const int totalHeight = static_cast<int>(app.gems.size()) * (cardH + spacing);
  const int maxScroll = std::max(0, totalHeight - viewH);
  app.gemRefineScroll = std::clamp(app.gemRefineScroll, 0, maxScroll);

  BeginScissorMode(viewX, viewY, viewW, viewH);

  for (std::size_t i = 0; i < app.gems.size(); ++i) {
    const auto& gd = app.gems[i];
    const float cy = static_cast<float>(viewY + static_cast<int>(i) * (cardH + spacing) - app.gemRefineScroll);
    if (cy + cardH < viewY || cy > viewY + viewH) continue;

    const Rectangle cardRec{static_cast<float>(viewX), cy, static_cast<float>(viewW - 16), static_cast<float>(cardH)};
    DrawRectangleRounded(cardRec, 0.10F, 4, Color{34, 32, 46, 255});
    DrawRectangleRoundedLines(cardRec, 0.10F, 4, Color{60, 58, 80, 255});

    // Gem Icon / Badge
    const Color gemCol = gemRarityColor(gd.rarity);
    DrawCircle(static_cast<int>(viewX + 36), static_cast<int>(cy + 49), 22.0F, gemCol);
    DrawCircleLines(static_cast<int>(viewX + 36), static_cast<int>(cy + 49), 22.0F, Color{255, 255, 255, 120});
    std::string abbr = gd.name.size() > 3 ? gd.name.substr(0, 3) : gd.name;
    drawTextWithOutline(abbr.c_str(), viewX + 36 - MeasureText(abbr.c_str(), 12) / 2, static_cast<int>(cy + 43), 12, WHITE, BLACK);

    // Gem Info
    std::string nameTitle = gd.name + " (" + gd.rarity + ")";
    DrawText(nameTitle.c_str(), viewX + 70, static_cast<int>(cy + 12), 15, RAYWHITE);
    DrawText(("Base: " + gd.effect).c_str(), viewX + 70, static_cast<int>(cy + 32), 12, Color{180, 215, 190, 255});

    // Counts
    const int roughCnt = ed::countRoughGems(app.inventory, gd.id);
    const int cutCnt = ed::countItem(app.inventory, "gems", "cut-" + gd.id);
    const int flawlessCnt = ed::countItem(app.inventory, "gems", "flawless-" + gd.id);

    std::string cnts = "Rough: " + std::to_string(roughCnt) + "   |   Cut: " + std::to_string(cutCnt) + "   |   Flawless: " + std::to_string(flawlessCnt);
    DrawText(cnts.c_str(), viewX + 70, static_cast<int>(cy + 54), 12, Color{245, 215, 140, 255});
    std::string statMult = "Multipliers: Cut = x2 (" + std::to_string(cutCnt) + "), Flawless = x4 (" + std::to_string(flawlessCnt) + ")";
    DrawText(statMult.c_str(), viewX + 70, static_cast<int>(cy + 74), 11, Color{150, 150, 170, 255});

    // Refine Button 1: Rough -> Cut
    const Rectangle btnCut{static_cast<float>(viewX + 480), cy + 12, 185, 74};
    const bool canCut = ed::canCombineGems(app.inventory, app.game, gd.id);
    const bool hCut = CheckCollisionPointRec(mouse, btnCut) && canCut;
    Color cutBg = canCut ? (hCut ? Color{55, 120, 80, 255} : Color{40, 90, 60, 255}) : Color{45, 42, 55, 255};
    Color cutBorder = canCut ? Color{110, 230, 140, 255} : Color{70, 65, 85, 255};
    DrawRectangleRounded(btnCut, 0.15F, 4, cutBg);
    DrawRectangleRoundedLines(btnCut, 0.15F, 4, cutBorder);
    DrawText("REFINE TO CUT (x2)", static_cast<int>(btnCut.x + 10), static_cast<int>(btnCut.y + 8), 13, canCut ? WHITE : Color{140, 140, 155, 255});
    DrawText("Req: 3 Rough", static_cast<int>(btnCut.x + 10), static_cast<int>(btnCut.y + 28), 11, (roughCnt >= 3) ? Color{180, 240, 180, 255} : Color{230, 120, 120, 255});
    DrawText("Cost: 10 Dust, 50 Gold", static_cast<int>(btnCut.x + 10), static_cast<int>(btnCut.y + 46), 11, (app.game.player.arcaneDust >= 10 && app.game.player.gold >= 50) ? Color{200, 230, 255, 255} : Color{230, 120, 120, 255});

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, btnCut) && mouse.y >= viewY && mouse.y <= viewY + viewH) {
      if (canCut) {
        ed::combineGems(app.inventory, app.game, gd.id);
        setNotice(app, "Refined 3 Rough " + gd.name + " into 1 Cut " + gd.name + "!");
      } else {
        setNotice(app, "Need 3 Rough " + gd.name + ", 10 Arcane Dust, and 50 Gold.");
      }
    }

    // Refine Button 2: Cut -> Flawless
    const Rectangle btnFlawless{static_cast<float>(viewX + 675), cy + 12, 185, 74};
    const bool canFlawless = ed::canCombineGems(app.inventory, app.game, "cut-" + gd.id);
    const bool hFlawless = CheckCollisionPointRec(mouse, btnFlawless) && canFlawless;
    Color flBg = canFlawless ? (hFlawless ? Color{105, 55, 140, 255} : Color{80, 40, 110, 255}) : Color{45, 42, 55, 255};
    Color flBorder = canFlawless ? Color{210, 140, 255, 255} : Color{70, 65, 85, 255};
    DrawRectangleRounded(btnFlawless, 0.15F, 4, flBg);
    DrawRectangleRoundedLines(btnFlawless, 0.15F, 4, flBorder);
    DrawText("REFINE TO FLAWLESS (x4)", static_cast<int>(btnFlawless.x + 8), static_cast<int>(btnFlawless.y + 8), 12, canFlawless ? WHITE : Color{140, 140, 155, 255});
    DrawText("Req: 3 Cut", static_cast<int>(btnFlawless.x + 10), static_cast<int>(btnFlawless.y + 28), 11, (cutCnt >= 3) ? Color{180, 240, 180, 255} : Color{230, 120, 120, 255});
    DrawText("Cost: 30 Dust, 200 Gold", static_cast<int>(btnFlawless.x + 10), static_cast<int>(btnFlawless.y + 46), 11, (app.game.player.arcaneDust >= 30 && app.game.player.gold >= 200) ? Color{200, 230, 255, 255} : Color{230, 120, 120, 255});

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, btnFlawless) && mouse.y >= viewY && mouse.y <= viewY + viewH) {
      if (canFlawless) {
        ed::combineGems(app.inventory, app.game, "cut-" + gd.id);
        setNotice(app, "Refined 3 Cut " + gd.name + " into 1 Flawless " + gd.name + "!");
      } else {
        setNotice(app, "Need 3 Cut " + gd.name + ", 30 Arcane Dust, and 200 Gold.");
      }
    }
  }

  EndScissorMode();

  // Scrollbar
  if (maxScroll > 0) {
    const float trackH = static_cast<float>(viewH);
    const float thumbH = std::max(28.0F, trackH * (static_cast<float>(viewH) / static_cast<float>(totalHeight)));
    const float thumbY = static_cast<float>(viewY) + (static_cast<float>(app.gemRefineScroll) / static_cast<float>(maxScroll)) * (trackH - thumbH);
    DrawRectangleRounded({box.x + box.width - 16, static_cast<float>(viewY), 6, trackH}, 0.5F, 2, Color{38, 37, 50, 200});
    DrawRectangleRounded({box.x + box.width - 16, thumbY, 6, thumbH}, 0.5F, 2, Color{210, 175, 80, 255});
  }

  DrawText("Scroll with mouse wheel. Press ESC or R to close.", static_cast<int>(box.x + 30), static_cast<int>(box.y + box.height - 24), 12, Color{150, 150, 165, 255});
}


} // namespace ed::app
