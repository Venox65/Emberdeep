import re

with open("client/src/app/main.cpp", "r") as f:
    code = f.read()

# 1. Replace drawNavigation function
old_nav = """void drawNavigation(const AppState& app) {
  DrawRectangle(0, 665, kWidth, 55, Color{28, 27, 39, 255});
  const Screen navScreens[] = {Screen::Mine, Screen::Inventory, Screen::Crafting, Screen::Campaign, Screen::Dungeon, Screen::GemHollow, Screen::Profile};
  for (int i = 0; i < 7; ++i) {
    const int x = 10 + i * 180;
    const Screen screen = navScreens[i];
    const bool active = app.screen == screen;
    DrawRectangleRounded({static_cast<float>(x), 675.0F, 160.0F, 34.0F}, 0.25F, 8,
                         active ? Color{112, 76, 45, 255} : Color{48, 47, 61, 255});
    DrawText(screenName(screen), x + 24, 684, 15, active ? RAYWHITE : Color{170, 170, 180, 255});
  }
}"""
new_nav = """void drawNavigation(const AppState& app) {
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
    // Just a small hamburger menu button at top left
    DrawRectangleRounded({10, 10, 40, 40}, 0.2F, 4, Color{48, 47, 61, 200});
    DrawRectangle(18, 20, 24, 4, RAYWHITE);
    DrawRectangle(18, 28, 24, 4, RAYWHITE);
    DrawRectangle(18, 36, 24, 4, RAYWHITE);
  }
}"""
code = code.replace(old_nav, new_nav)

# 2. Update the input handling
old_input = """    for (int i = 0; i < 7; ++i) {
      const Screen navScreens[] = {Screen::Mine, Screen::Inventory, Screen::Crafting, Screen::Campaign, Screen::Dungeon, Screen::GemHollow, Screen::Profile};
      const Rectangle navigationButton = {
          10.0F + static_cast<float>(i * 180),
          675.0F,
          160.0F,
          34.0F,
      };
      if (CheckCollisionPointRec(mouse, navigationButton)) {
        app.screen = navScreens[i];
        break;
      }
    }"""
new_input = """    if (app.sidebarOpen) {
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
      if (CheckCollisionPointRec(mouse, {10, 10, 40, 40})) {
        app.sidebarOpen = true;
      }
    }"""
code = code.replace(old_input, new_input)

# 3. Transparent bottom bar for keybinds in Inventory
# Actually, the user asked for:
# "let's get the bottom transparent bar with the keybinds to stay where it was"
# Previously it was:
# DrawRectangle(0, 630, 1280, 35, Color{28, 27, 39, 0});
# Wait, "transparent" might mean Color{28, 27, 39, 0} ?
# Or it was just at the bottom of the screen (y=685) since there are no tabs anymore!
# I'll move it to 685.
old_bar = """  DrawRectangle(0, 630, 1280, 35, Color{28, 27, 39, 0});
  DrawText(("Gear " + std::to_string(app.selectedGear + 1) + "/" +
            std::to_string(app.inventory.gear.size()))
               .c_str(),
           10, 640, 15, RAYWHITE);
  DrawText("A / D select gear   U upgrade   E equip   G insert gem   R remove gem   N enchant", 390, 640, 15,
           Color{174, 174, 187, 255});"""
new_bar = """  DrawRectangle(0, 685, 1280, 35, Color{28, 27, 39, 0});
  DrawText(("Gear " + std::to_string(app.selectedGear + 1) + "/" +
            std::to_string(app.inventory.gear.size()))
               .c_str(),
           10, 695, 15, RAYWHITE);
  DrawText("A / D select gear   U upgrade   E equip   G insert gem   R remove gem   N enchant", 390, 695, 15,
           Color{174, 174, 187, 255});"""
code = code.replace(old_bar, new_bar)

with open("client/src/app/main.cpp", "w") as f:
    f.write(code)

