import re

with open("client/src/app/main.cpp", "r") as f:
    code = f.read()

old_keys = """      app.game.dirty = equipped;
      setNotice(app, equipped ? "Gear equipped. Stats updated." : "Cannot equip gear.");
    }
  } else if (app.screen == Screen::Crafting) {"""

new_keys = """      app.game.dirty = equipped;
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
  } else if (app.screen == Screen::Crafting) {"""

code = code.replace(old_keys, new_keys)

with open("client/src/app/main.cpp", "w") as f:
    f.write(code)

