#include "input_handler.hpp"

#include <algorithm>
#include <string>

#include "game_actions.hpp"
#include "ui_screens.hpp"
#include "ed/formulas.hpp"
#include "ed/inventory.hpp"

namespace ed::app {

void handleInventoryInput(AppState& app, Vector2 mouse, bool leftClick) {
  if (app.screen == Screen::Inventory) {
    if (app.gemSlotMenuOpen) {
      if (IsKeyPressed(KEY_ESCAPE) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        app.gemSlotMenuOpen = false;
        return;
      }
      if (IsKeyPressed(KEY_TAB)) {
        app.gemRadialFilterTier = (app.gemRadialFilterTier + 1) % 4;
      }
      if (IsKeyPressed(KEY_ONE))   app.gemRadialFilterTier = 0;
      if (IsKeyPressed(KEY_TWO))   app.gemRadialFilterTier = 1;
      if (IsKeyPressed(KEY_THREE)) app.gemRadialFilterTier = 2;
      if (IsKeyPressed(KEY_FOUR))  app.gemRadialFilterTier = 3;

      if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE)) {
        if (ed::app::unsocketGemFromGear(app, app.gemSlotMenuGearIndex, app.gemSlotMenuSocketIndex)) {
          app.gemSlotMenuOpen = false;
        }
        return;
      }

      const float wheel = GetMouseWheelMove();
      if (wheel != 0.0F) {
        if (wheel > 0.0F) {
          app.gemRadialFilterTier = (app.gemRadialFilterTier + 3) % 4;
        } else {
          app.gemRadialFilterTier = (app.gemRadialFilterTier + 1) % 4;
        }
      }
      return;
    }

    if (IsKeyPressed(KEY_R)) {
      app.gemRefineModalOpen = !app.gemRefineModalOpen;
      return;
    }
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

    constexpr int cardWidth = 390;
    constexpr int cardHeight = 130;
    constexpr int rowHeight = 148;
    constexpr int kCols = 3;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !app.inventory.gear.empty()) {
      const int gearY = inventoryGearStartY(app) - app.inventoryScroll;
      const Vector2 mouse = GetMousePosition();
      if (mouse.y >= 160.0F && mouse.y <= 660.0F) {
        bool socketClicked = false;
        for (std::size_t index = 0; index < app.inventory.gear.size(); ++index) {
          const auto& piece = app.inventory.gear[index];
          const int column = static_cast<int>(index % kCols);
          const int row = static_cast<int>(index / kCols);
          const float cardX = 30.0F + static_cast<float>(column) * (cardWidth + 15);
          const float cardY = static_cast<float>(gearY + row * rowHeight);

          if (piece.sockets > 0) {
            constexpr int slotSize = 28;
            constexpr int slotSpacing = 34;
            for (int s = 0; s < piece.sockets; ++s) {
              const float sx = cardX + 56.0F + s * slotSpacing;
              const float sy = cardY + 70.0F;
              const float cx = sx + slotSize / 2.0F;
              const float cy = sy + slotSize / 2.0F;
              const float dx = mouse.x - cx;
              const float dy = mouse.y - cy;
              if (dx * dx + dy * dy <= (slotSize / 2.0F) * (slotSize / 2.0F)) {
                // Clicked socket s!
                app.selectedGear = static_cast<int>(index);
                app.gemSlotMenuOpen = true;
                app.gemSlotMenuJustOpened = true;
                app.gemSlotMenuGearIndex = static_cast<int>(index);
                app.gemSlotMenuSocketIndex = s;
                app.gemRadialSelectedBaseIndex = -1;
                app.gemRadialHoveredTier = -1;
                app.gemSlotMenuPos = {
                  std::clamp(cx, 160.0F, static_cast<float>(kWidth - 160)),
                  std::clamp(cy, 160.0F, static_cast<float>(kHeight - 160))
                };
                socketClicked = true;
                break;
              }
            }
          }
          if (socketClicked) break;

          // Check if card body was clicked to select
          if (mouse.x >= cardX && mouse.x <= cardX + cardWidth &&
              mouse.y >= cardY && mouse.y <= cardY + cardHeight) {
            app.selectedGear = static_cast<int>(index);
            break;
          }
        }
      }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !app.inventory.gear.empty()) {
      const int gearY = inventoryGearStartY(app) - app.inventoryScroll;
      const Vector2 mouse = GetMousePosition();
      if (mouse.y >= 160.0F && mouse.y <= 660.0F) {
        for (std::size_t index = 0; index < app.inventory.gear.size(); ++index) {
          auto& piece = app.inventory.gear[index];
          const int column = static_cast<int>(index % kCols);
          const int row = static_cast<int>(index / kCols);
          const float cardX = 30.0F + static_cast<float>(column) * (cardWidth + 15);
          const float cardY = static_cast<float>(gearY + row * rowHeight);

          if (piece.sockets > 0) {
            constexpr int slotSize = 28;
            constexpr int slotSpacing = 34;
            for (int s = 0; s < piece.sockets; ++s) {
              const float sx = cardX + 56.0F + s * slotSpacing;
              const float sy = cardY + 70.0F;
              const float cx = sx + slotSize / 2.0F;
              const float cy = sy + slotSize / 2.0F;
              const float dx = mouse.x - cx;
              const float dy = mouse.y - cy;
              if (dx * dx + dy * dy <= (slotSize / 2.0F) * (slotSize / 2.0F)) {
                if (s < static_cast<int>(piece.socketedGems.size())) {
                  std::string gemId = piece.socketedGems[static_cast<std::size_t>(s)];
                  ed::removeGem(app.inventory, piece, static_cast<std::size_t>(s));
                  app.game.dirty = true;
                  setNotice(app, "Removed " + gemId + " from socket.");
                }
                break;
              }
            }
          }
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
      app.particles.clear();
      app.floatingText.clear();
      app.shake = 0.0F;
    }
  } else if (app.screen == Screen::GemHollow) {
    if (IsKeyPressed(KEY_R)) {
      app.gemRefineModalOpen = !app.gemRefineModalOpen;
      return;
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
      const float wheel = GetMouseWheelMove();
      if (wheel != 0.0F) {
        app.enchantingScroll -= static_cast<int>(wheel * 42.0F);
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
  }
}

} // namespace ed::app
