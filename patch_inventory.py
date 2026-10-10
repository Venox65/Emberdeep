import re

with open("client/src/app/main.cpp", "r") as f:
    content = f.read()

target = """  EndScissorMode();
  DrawRectangle(0, 630, 1280, 35, Color{28, 27, 39, 255});
  DrawText(("Gear " + std::to_string(app.selectedGear + 1) + "/" +
            std::to_string(app.inventory.gear.size()) + "   Scroll: mouse wheel or W/S")
               .c_str(),
           70, 640, 15, Color{174, 174, 187, 255});
  DrawText("A / D select gear   U upgrade selected   E equip selected", 420, 640, 15,"""

replacement = """  EndScissorMode();
  DrawRectangle(0, 685, 1280, 35, Color{28, 27, 39, 0});
  DrawText(("Gear " + std::to_string(app.selectedGear + 1) + "/" +
            std::to_string(app.inventory.gear.size()) + "   Scroll: mouse wheel or W/S")
               .c_str(),
           70, 695, 15, Color{174, 174, 187, 255});
  DrawText("A / D select gear   U upgrade   E equip   G insert gem   R remove gem   N enchant", 420, 695, 15,"""

if target in content:
    content = content.replace(target, replacement)
    print("Patched inventory successfully")
else:
    print("Target inventory not found")

target2 = """    // Hamburger menu button on the right, below the sign out button
    DrawRectangleRounded({1220, 60, 40, 40}, 0.2F, 4, Color{48, 47, 61, 200});
    DrawRectangle(1228, 70, 24, 4, RAYWHITE);
    DrawRectangle(1228, 78, 24, 4, RAYWHITE);
    DrawRectangle(1228, 86, 24, 4, RAYWHITE);"""

replacement2 = """    // Hamburger menu button on the right, below the sign out button
    DrawRectangleRounded({1220, 100, 40, 40}, 0.2F, 4, Color{48, 47, 61, 200});
    DrawRectangle(1228, 110, 24, 4, RAYWHITE);
    DrawRectangle(1228, 118, 24, 4, RAYWHITE);
    DrawRectangle(1228, 126, 24, 4, RAYWHITE);"""

if target2 in content:
    content = content.replace(target2, replacement2)
    print("Patched menu button visual successfully")
else:
    print("Target menu button not found")

target3 = """    } else {
      if (CheckCollisionPointRec(mouse, {1220, 60, 40, 40})) {
        app.sidebarOpen = true;
      }
    }"""

replacement3 = """    } else {
      if (CheckCollisionPointRec(mouse, {1220, 100, 40, 40})) {
        app.sidebarOpen = true;
      }
    }"""

if target3 in content:
    content = content.replace(target3, replacement3)
    print("Patched menu button hit successfully")
else:
    print("Target menu hit not found")

with open("client/src/app/main.cpp", "w") as f:
    f.write(content)
