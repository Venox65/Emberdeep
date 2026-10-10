with open("client/src/app/main.cpp", "r") as f:
    content = f.read()

# Fix inventoryGearStartY advanceBucket
advanceBucket_target = """  const auto advanceBucket = [&y](const std::map<std::string, int>& bucket) {
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
  };"""

advanceBucket_replacement = """  const auto advanceBucket = [&y](const std::map<std::string, int>& bucket) {
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
  };"""

# Fix drawBucket
drawBucket_target = """  const auto drawBucket = [&](const char* title, const std::map<std::string, int>& bucket) {
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
  drawBucket("Gems", app.inventory.gems);"""

drawBucket_replacement = """  const auto drawBucket = [&](const char* title, const std::map<std::string, int>& bucket, bool isIngot = false) {
    DrawText(title, 70, y - offset, 18, Color{174, 174, 187, 255});
    int x = 250;
    for (const auto& [id, count] : bucket) {
      DrawRectangleRounded({static_cast<float>(x), static_cast<float>(y - 5 - offset), 220, 44}, 0.18F, 6,
                           Color{52, 52, 67, 255});
                           
      if (isIngot) {
        std::string texPath = "sprites/Materials/" + id + "_ingot.png";
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
  drawBucket("Gems", app.inventory.gems);"""


button_target = """    // Enchant button
    Color btnColor = canAfford ? Color{112, 76, 45, 255} : Color{60, 55, 70, 255};
    DrawRectangleRounded({625, 450, 585, 48}, 0.2F, 6, btnColor);
    drawTextCentered(piece.enchantment.empty() ? "Enchant (N)" : "Re-enchant (N)", 468, 22,
                     canAfford ? RAYWHITE : Color{120, 115, 130, 255});
    
    // Mouse click on enchant button
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse = GetMousePosition();
      if (mouse.x >= 625 && mouse.x <= 1210 && mouse.y >= 450 && mouse.y <= 498) {"""

button_replacement = """    // Enchant button
    Color btnColor = canAfford ? Color{112, 76, 45, 255} : Color{60, 55, 70, 255};
    DrawRectangleRounded({625, 450, 250, 40}, 0.2F, 6, btnColor);
    DrawText(piece.enchantment.empty() ? "Enchant (N)" : "Re-enchant (N)", 680, 460, 20,
                     canAfford ? RAYWHITE : Color{120, 115, 130, 255});
    
    // Mouse click on enchant button
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      Vector2 mouse = GetMousePosition();
      if (mouse.x >= 625 && mouse.x <= 875 && mouse.y >= 450 && mouse.y <= 490) {"""

if advanceBucket_target in content: content = content.replace(advanceBucket_target, advanceBucket_replacement)
if drawBucket_target in content: content = content.replace(drawBucket_target, drawBucket_replacement)
if button_target in content: content = content.replace(button_target, button_replacement)

with open("client/src/app/main.cpp", "w") as f:
    f.write(content)
