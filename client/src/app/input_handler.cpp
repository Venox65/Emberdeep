#include "input_handler.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <utility>

#include "campaign_minigames.hpp"
#include "game_actions.hpp"
#include "save_system.hpp"
#include "ui_screens.hpp"

namespace ed::app {

void handleInput(AppState& app) {
  if (app.showSaveSelect) {
    handleSaveSelectInput(app);
    return;
  }
  if (handleActiveCampaignInput(app)) return;

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
      app.showSaveSelect = true;
      app.saveFilesList = scanSaveFiles(app.savePath);
      app.saveSelectScroll = 0;
      app.newSaveInputFocused = false;
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
    const bool isLayer10 = (app.layers[static_cast<std::size_t>(app.layerIndex)].id == "aether-vault");
    const bool strataUnlocked = (isLayer10 && ed::isDeepStrataUnlocked(app.game));

    if (IsKeyPressed(KEY_Q)) {
      changeLayer(app, -1);
    }
    if (IsKeyPressed(KEY_E)) {
      changeLayer(app, 1);
    }

    if (strataUnlocked) {
      const int maxDepth = std::max(1, app.game.player.deepestDepth + 1);
      if (IsKeyPressed(KEY_LEFT_BRACKET) || IsKeyPressed(KEY_MINUS)) {
        if (app.deepStrataDepth > 0) {
          --app.deepStrataDepth;
          spawnBlock(app);
        }
      }
      if (IsKeyPressed(KEY_RIGHT_BRACKET) || IsKeyPressed(KEY_EQUAL)) {
        if (app.deepStrataDepth < maxDepth) {
          ++app.deepStrataDepth;
          spawnBlock(app);
        }
      }
      if (IsKeyPressed(KEY_T)) {
        app.deepStrataAutoAdvance = !app.deepStrataAutoAdvance;
        setNotice(app, app.deepStrataAutoAdvance ? "Deep Strata auto-advance enabled." : "Deep Strata auto-advance disabled.");
      }
    }

    if (IsKeyPressed(KEY_SPACE)) mine(app, {640, 305});
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      const Vector2 mouse = GetMousePosition();
      bool buttonClicked = false;
      if (strataUnlocked) {
        const Rectangle minusBtn{410, 164, 34, 22};
        const Rectangle plusBtn{835, 164, 34, 22};
        const Rectangle autoBtn{885, 164, 95, 22};
        const int maxDepth = std::max(1, app.game.player.deepestDepth + 1);
        if (CheckCollisionPointRec(mouse, minusBtn)) {
          if (app.deepStrataDepth > 0) {
            --app.deepStrataDepth;
            spawnBlock(app);
          }
          buttonClicked = true;
        } else if (CheckCollisionPointRec(mouse, plusBtn)) {
          if (app.deepStrataDepth < maxDepth) {
            ++app.deepStrataDepth;
            spawnBlock(app);
          }
          buttonClicked = true;
        } else if (CheckCollisionPointRec(mouse, autoBtn)) {
          app.deepStrataAutoAdvance = !app.deepStrataAutoAdvance;
          setNotice(app, app.deepStrataAutoAdvance ? "Deep Strata auto-advance enabled." : "Deep Strata auto-advance disabled.");
          buttonClicked = true;
        }
      }
      if (!buttonClicked && CheckCollisionPointRec(mouse, {400, 220, 480, 260})) {
        mine(app, mouse);
      }
    }
  } else if (app.screen == Screen::Inventory) {
    if (IsKeyPressed(KEY_A) && !app.inventory.gear.empty()) {
      app.selectedGear = std::max(0, app.selectedGear - 1);
      syncGearScroll(app);
    }
    if (IsKeyPressed(KEY_D) && !app.inventory.gear.empty()) {
      app.selectedGear = std::min(static_cast<int>(app.inventory.gear.size()) - 1,
                                  app.selectedGear + 1);
      syncGearScroll(app);
    }
    if (IsKeyPressed(KEY_W)) {
      app.inventoryScroll -= 120;
      clampInventoryScroll(app);
    }
    if (IsKeyPressed(KEY_S)) {
      app.inventoryScroll += 120;
      clampInventoryScroll(app);
    }
    const float wheel = GetMouseWheelMove();
    if (wheel != 0.0F) {
      app.inventoryScroll -= static_cast<int>(wheel * 80.0F);
      clampInventoryScroll(app);
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !app.inventory.gear.empty()) {
      const int gearY = inventoryGearStartY(app) - app.inventoryScroll;
      const Vector2 mouse = GetMousePosition();
      const int gearRows = (static_cast<int>(app.inventory.gear.size()) + 2) / 3;
      if (mouse.y >= 160.0F && mouse.y <= 660.0F &&
          mouse.y >= gearY && mouse.y < gearY + gearRows * 70 &&
          mouse.x >= 70.0F && mouse.x < 1180.0F) {
        const int column = std::clamp(static_cast<int>((mouse.x - 70.0F) / 370.0F), 0, 2);
        const int row = static_cast<int>((mouse.y - gearY) / 70.0F);
        const int index = row * 3 + column;
        if (index >= 0 && index < static_cast<int>(app.inventory.gear.size())) {
          app.selectedGear = index;
        }
      }
    }
    if (!app.inventory.gear.empty() && IsKeyPressed(KEY_U)) {
      auto& piece = app.inventory.gear[static_cast<std::size_t>(app.selectedGear)];
      if (ed::upgradeGear(app.game, piece)) {
        if (piece.upgradeLevel >= 11) {
          setNotice(app, "Mythic upgrade successful! (" + piece.name + " +" +
                             std::to_string(piece.upgradeLevel) + ")");
        } else {
          setNotice(app, "Gear upgraded to +" + std::to_string(piece.upgradeLevel) + ".");
        }
      } else {
        if (piece.upgradeLevel >= ed::maxGearUpgradeLevel(app.game)) {
          setNotice(app, "This gear is already at maximum level (" +
                             std::to_string(piece.upgradeLevel) + ").");
        } else if (piece.upgradeLevel >= 10 && !ed::isMythicUpgradeUnlocked(app.game)) {
          setNotice(app, "Mythic upgrades (+11 to +20) require defeating The Hollow King.");
        } else {
          const int gold = ed::upgradeGoldCost(piece);
          const int dust = ed::upgradeDustCost(piece);
          const int shards = ed::upgradeAetherShardCost(piece);
          const int sigils = ed::upgradeEchoSigilCost(piece);
          std::string costMsg = "Need " + std::to_string(gold) + " gold, " + std::to_string(dust) + " dust";
          if (shards > 0) costMsg += ", " + std::to_string(shards) + " Aether Shard(s)";
          if (sigils > 0) costMsg += ", " + std::to_string(sigils) + " Echo Sigil(s)";
          setNotice(app, costMsg + ".");
        }
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
  } else if (app.screen == Screen::Enchanting) {
    if (!app.inventory.gear.empty()) {
      if (IsKeyPressed(KEY_W)) {
        app.selectedGear = std::max(0, app.selectedGear - 1);
      }
      if (IsKeyPressed(KEY_S)) {
        app.selectedGear = std::min(static_cast<int>(app.inventory.gear.size()) - 1,
                                    app.selectedGear + 1);
      }
      if (IsKeyPressed(KEY_N) && app.selectedGear < static_cast<int>(app.inventory.gear.size())) {
        auto& piece = app.inventory.gear[static_cast<size_t>(app.selectedGear)];
        bool ok = ed::enchantGearPiece(app.game, app.inventory, piece.itemId, app.rng);
        if (ok) {
          setNotice(app, "Enchanted: " + piece.enchantment);
        } else {
          setNotice(app, "Not enough resources to enchant.");
        }
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
    if (app.inBossCombat) {
      if (app.bossCombatShowResults) {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) ||
            (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(GetMousePosition(), {540, 560, 200, 45}))) {
          finishBossCombat(app);
        }
      } else {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
          finishBossCombat(app);
          setNotice(app, "Retreated from boss combat.");
        }
      }
      return;
    }
    if (!app.campaignAttempt && (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_TAB))) {
      if (ed::isEchoCampaignUnlocked(app.game)) {
        app.echoCampaignMode = !app.echoCampaignMode;
        app.campaignScore = 0;
        app.campaignIntro = false;
        setNotice(app, app.echoCampaignMode ? "Echo Campaign active! Face empowered boss echoes."
                                            : "Standard Campaign active.");
      } else {
        setNotice(app, "Echo Campaign unlocks after defeating The Hollow King (Boss 10).");
      }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !app.campaignAttempt) {
      const Vector2 mouse = GetMousePosition();
      if (CheckCollisionPointRec(mouse, {380, 160, 240, 36})) {
        if (app.echoCampaignMode) {
          app.echoCampaignMode = false;
          app.campaignScore = 0;
          app.campaignIntro = false;
          setNotice(app, "Standard Campaign active.");
        }
      } else if (CheckCollisionPointRec(mouse, {660, 160, 240, 36})) {
        if (!app.echoCampaignMode) {
          if (ed::isEchoCampaignUnlocked(app.game)) {
            app.echoCampaignMode = true;
            app.campaignScore = 0;
            app.campaignIntro = false;
            setNotice(app, "Echo Campaign active! Face empowered boss echoes.");
          } else {
            setNotice(app, "Echo Campaign unlocks after defeating The Hollow King (Boss 10).");
          }
        }
      }
    }
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
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE)) {
      if (app.campaignIntro) {
        app.campaignIntro = false;
      } else if (app.campaignAttempt) {
        app.campaignAttempt = false;
        app.campaignScore = 0;
        app.campaignHeartsLost = 0;
        setNotice(app, "Boss attempt abandoned.");
      }
    }
  } else if (app.screen == Screen::Dungeon) {
    if (app.inDungeonCombat) return;

    const bool abyssUnlocked = (app.game.player.bossesCleared >= 10 || app.game.player.ascensions > 0 || app.game.player.abyssBestFloor > 0);
    if (abyssUnlocked) {
      if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_TAB)) {
        app.abyssDungeonMode = !app.abyssDungeonMode;
        setNotice(app, app.abyssDungeonMode ? "The Abyss selected (Endless Mode)." : "Standard Expeditions selected.");
      }
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        const Vector2 mouse = GetMousePosition();
        const Rectangle stdTab{400, 155, 220, 36};
        const Rectangle abyssTab{660, 155, 220, 36};
        if (CheckCollisionPointRec(mouse, stdTab)) {
          app.abyssDungeonMode = false;
          setNotice(app, "Standard Expeditions selected.");
        } else if (CheckCollisionPointRec(mouse, abyssTab)) {
          app.abyssDungeonMode = true;
          setNotice(app, "The Abyss selected (Endless Mode).");
        }
      }
    }

    if (!app.abyssDungeonMode) {
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
    }

    if (IsKeyPressed(KEY_ENTER)) {
      if (app.abyssDungeonMode) {
        runDungeon(app);
      } else {
        if (!dungeonUnlocked(app, app.selectedZone)) {
          setNotice(app, dungeonLockReason(app, app.selectedZone));
        } else {
          runDungeon(app);
        }
      }
    }
  } else if (app.screen == Screen::Profile) {
    const Vector2 mouse = GetMousePosition();
    const bool unlocked = (app.game.player.bossesCleared >= 10 || app.game.player.ascensions > 0);

    if (app.ascensionConfirm) {
      const Rectangle confirmBtn{410, 430, 210, 48};
      const Rectangle cancelBtn{670, 430, 210, 48};
      if (IsKeyPressed(KEY_Y) ||
          (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, confirmBtn))) {
        const int marks = ed::applyAscension(app.game, &app.inventory);
        app.ascensionConfirm = false;
        app.layerIndex = 0;
        app.deepStrataDepth = 0;
        app.game.player.currentLayer = "ashen-warrens";
        spawnBlock(app);
        requestSave(app);
        setNotice(app, "Ascended! Gained " + std::to_string(marks) + " Ember Marks.");
      } else if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE) ||
                 (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, cancelBtn))) {
        app.ascensionConfirm = false;
      }
      return;
    }

    if (unlocked) {
      const Rectangle ascendBtn{1060, 173, 160, 42};
      if ((IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, ascendBtn)) ||
          IsKeyPressed(KEY_ENTER)) {
        app.ascensionConfirm = true;
      }

      const char* perkIds[] = {"potential", "power", "wealth", "wisdom", "armory", "smelting"};
      const KeyboardKey keys[] = {KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE, KEY_SIX};

      int py = 262;
      for (int i = 0; i < 6; ++i) {
        const Rectangle upgradeBtn{1040, static_cast<float>(py + 9), 175, 40};
        if (IsKeyPressed(keys[i]) ||
            (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, upgradeBtn))) {
          if (ed::upgradeEmberPerk(app.game, perkIds[i])) {
            requestSave(app);
            setNotice(app, "Upgraded " + std::string(perkIds[i]) + " perk!");
          } else {
            setNotice(app, "Cannot upgrade " + std::string(perkIds[i]) + " (not enough marks or max rank).");
          }
        }
        py += 65;
      }
    }
  }
}

} // namespace ed::app
