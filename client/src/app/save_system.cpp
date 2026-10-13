#include "save_system.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <fstream>
#include <mutex>
#include <string>
#include <utility>

#include "game_actions.hpp"

namespace ed::app {

void configureSaveStorage(AppState& app) {
  std::ifstream cfg("last_save.cfg");
  if (cfg.is_open()) {
    std::string line;
    if (std::getline(cfg, line)) {
      while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
        line.pop_back();
      }
      if (!line.empty() && std::filesystem::exists(line)) {
        app.savePath = line;
        return;
      }
    }
  }
  if (std::filesystem::exists("emberdeep.sav")) {
    app.savePath = "emberdeep.sav";
  } else if (std::filesystem::exists("emberdeep-admin.sav")) {
    app.savePath = "emberdeep-admin.sav";
  } else {
    app.savePath = "emberdeep.sav";
  }
}

void setLastPlayedSave(const std::filesystem::path& path) {
  std::ofstream cfg("last_save.cfg", std::ios::trunc);
  if (cfg.is_open()) {
    cfg << path.filename().string() << "\n";
  }
}

std::vector<SaveFileInfo> scanSaveFiles(const std::filesystem::path& currentSavePath) {
  std::vector<SaveFileInfo> saves;
  std::error_code ec;
  for (const auto& entry : std::filesystem::directory_iterator(".", ec)) {
    if (entry.is_regular_file(ec) && entry.path().extension() == ".sav") {
      SaveFileInfo info;
      info.path = entry.path().lexically_normal();
      info.filename = entry.path().filename().string();
      info.isActive = (std::filesystem::equivalent(info.path, currentSavePath, ec) ||
                       info.filename == currentSavePath.filename().string());
      ed::SaveSnapshot snap;
      if (ed::readSave(info.path, snap)) {
        info.valid = true;
        info.level = snap.game.player.level;
        info.gold = snap.game.player.gold;
        info.bossesCleared = snap.game.player.bossesCleared;
        info.ascensions = snap.game.player.ascensions;
        info.deepestDepth = snap.game.player.deepestDepth;
        info.blocksBroken = snap.game.player.blocksBroken;
      }
      saves.push_back(info);
    }
  }

  bool foundActive = false;
  for (const auto& s : saves) {
    if (s.isActive) {
      foundActive = true;
      break;
    }
  }
  if (!foundActive) {
    SaveFileInfo info;
    info.path = currentSavePath;
    info.filename = currentSavePath.filename().string();
    info.isActive = true;
    ed::SaveSnapshot snap;
    if (ed::readSave(info.path, snap)) {
      info.valid = true;
      info.level = snap.game.player.level;
      info.gold = snap.game.player.gold;
      info.bossesCleared = snap.game.player.bossesCleared;
      info.ascensions = snap.game.player.ascensions;
      info.deepestDepth = snap.game.player.deepestDepth;
      info.blocksBroken = snap.game.player.blocksBroken;
    }
    saves.insert(saves.begin(), info);
  }

  std::sort(saves.begin(), saves.end(), [](const SaveFileInfo& a, const SaveFileInfo& b) {
    if (a.isActive != b.isActive) return a.isActive > b.isActive;
    return a.filename < b.filename;
  });
  return saves;
}

void requestSave(AppState& app) {
  {
    std::lock_guard lock(app.saveMutex);
    app.pendingSave = makeSnapshot(app);
  }
  app.saveCondition.notify_one();
  app.game.dirty = false;
}

void saveNow(AppState& app) {
  const auto snapshot = makeSnapshot(app);
  if (!ed::writeSaveAtomic(app.savePath, snapshot)) {
    setNotice(app, "Save failed: local save could not be written.");
    return;
  }
  setNotice(app, "Local save stored.");
  app.game.dirty = false;
}

void startSaveWorker(AppState& app) {
  app.saveWorkerRunning = true;
  app.saveWorker = std::thread([&app] {
    while (true) {
      std::optional<ed::SaveSnapshot> snapshot;
      {
        std::unique_lock lock(app.saveMutex);
        app.saveCondition.wait(lock, [&] {
          return !app.saveWorkerRunning || app.pendingSave.has_value();
        });
        if (!app.saveWorkerRunning && !app.pendingSave.has_value()) return;
        snapshot = std::move(app.pendingSave);
        app.pendingSave.reset();
      }
      if (snapshot.has_value()) {
        ed::writeSaveAtomic(app.savePath, *snapshot);
      }
    }
  });
}

void stopSaveWorker(AppState& app) {
  requestSave(app);
  {
    std::lock_guard lock(app.saveMutex);
    app.saveWorkerRunning = false;
  }
  app.saveCondition.notify_one();
  if (app.saveWorker.joinable()) app.saveWorker.join();
}

void loadSave(AppState& app) {
  ed::SaveSnapshot snapshot;
  if (ed::readSave(app.savePath, snapshot)) {
    app.game = snapshot.game;
    app.inventory = snapshot.inventory;
    app.campaign = snapshot.campaign;
  } else {
    app.game = ed::GameState{};
    app.inventory = ed::InventoryState{};
    app.campaign = ed::createCampaignState(10);
  }
}

void switchSave(AppState& app, const std::filesystem::path& newPath) {
  saveNow(app);
  stopSaveWorker(app);

  app.savePath = newPath;
  setLastPlayedSave(newPath);

  app.floatingText.clear();
  app.particles.clear();
  app.campaignAttempt = false;
  app.campaignIntro = false;
  app.inDungeonCombat = false;
  app.dungeonShowResults = false;
  app.dungeonBattleLog.clear();
  app.dungeonEnemies.clear();
  app.ascensionConfirm = false;
  app.deepStrataDepth = 0;

  loadSave(app);

  app.layerIndex = 0;
  for (std::size_t i = 0; i < app.layers.size(); ++i) {
    if (app.layers[i].id == app.game.player.currentLayer &&
        ed::canUnlockLayer(app.game, app.layers[i])) {
      app.layerIndex = static_cast<int>(i);
      break;
    }
  }
  app.game.player.currentLayer = app.layers[static_cast<std::size_t>(app.layerIndex)].id;
  spawnBlock(app);

  startSaveWorker(app);

  setNotice(app, "Loaded save: " + newPath.filename().string());
  app.saveFilesList = scanSaveFiles(app.savePath);
  app.showSaveSelect = false;
}

void createNewSave(AppState& app, const std::string& rawName) {
  std::string cleanName;
  for (char c : rawName) {
    if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_') {
      cleanName.push_back(c);
    } else if (c == ' ') {
      cleanName.push_back('-');
    }
  }
  if (cleanName.empty()) {
    cleanName = "slot-" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count() % 10000);
  }
  std::string filename = cleanName;
  if (!filename.ends_with(".sav")) {
    if (!filename.starts_with("emberdeep-") && filename != "emberdeep") {
      filename = "emberdeep-" + filename + ".sav";
    } else {
      filename = filename + ".sav";
    }
  }

  std::filesystem::path newPath(filename);
  if (std::filesystem::exists(newPath)) {
    setNotice(app, "Save file already exists: " + filename);
    return;
  }

  saveNow(app);
  stopSaveWorker(app);

  app.savePath = newPath;
  setLastPlayedSave(newPath);

  app.game = ed::GameState{};
  app.inventory = ed::InventoryState{};
  app.campaign = ed::createCampaignState(10);
  app.layerIndex = 0;
  app.deepStrataDepth = 0;
  app.selectedGear = 0;
  app.inventoryScroll = 0;
  app.craftingScroll = 0;
  app.floatingText.clear();
  app.particles.clear();
  app.campaignAttempt = false;
  app.campaignIntro = false;
  app.inDungeonCombat = false;
  app.dungeonShowResults = false;
  app.dungeonBattleLog.clear();
  app.dungeonEnemies.clear();
  app.ascensionConfirm = false;

  spawnBlock(app);
  saveNow(app);
  startSaveWorker(app);

  setNotice(app, "Created and loaded new save: " + filename);
  app.saveFilesList = scanSaveFiles(app.savePath);
  app.newSaveInputName.clear();
  app.newSaveInputFocused = false;
  app.showSaveSelect = false;
}

void handleSaveSelectInput(AppState& app) {
  const Vector2 mouse = GetMousePosition();
  const Rectangle modalRec{280, 80, 720, 560};
  const Rectangle closeBtn{modalRec.x + modalRec.width - 45, modalRec.y + 18, 28, 28};
  const Rectangle selectSaveBtn{1090, 14, 160, 34};

  if (IsKeyPressed(KEY_ESCAPE)) {
    app.showSaveSelect = false;
    return;
  }

  const float wheel = GetMouseWheelMove();
  const int visibleSlots = 5;
  const int maxScroll = std::max(0, static_cast<int>(app.saveFilesList.size()) - visibleSlots);
  if (wheel != 0.0F) {
    app.saveSelectScroll = std::clamp(app.saveSelectScroll - static_cast<int>(wheel), 0, maxScroll);
  }
  if (IsKeyPressed(KEY_UP)) {
    app.saveSelectScroll = std::max(0, app.saveSelectScroll - 1);
  }
  if (IsKeyPressed(KEY_DOWN)) {
    app.saveSelectScroll = std::min(maxScroll, app.saveSelectScroll + 1);
  }

  const Rectangle inputRec{modalRec.x + 30, modalRec.y + 514, 480, 36};
  const Rectangle createBtn{inputRec.x + inputRec.width + 15, inputRec.y, 165, 36};

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    if (CheckCollisionPointRec(mouse, closeBtn) || CheckCollisionPointRec(mouse, selectSaveBtn)) {
      app.showSaveSelect = false;
      return;
    }
    if (!CheckCollisionPointRec(mouse, modalRec)) {
      app.showSaveSelect = false;
      return;
    }

    if (CheckCollisionPointRec(mouse, inputRec)) {
      app.newSaveInputFocused = true;
    } else {
      app.newSaveInputFocused = false;
    }

    if (CheckCollisionPointRec(mouse, createBtn)) {
      createNewSave(app, app.newSaveInputName);
      return;
    }

    const Rectangle listArea{modalRec.x + 20, modalRec.y + 88, modalRec.width - 40, 365};
    for (std::size_t i = static_cast<std::size_t>(app.saveSelectScroll);
         i < std::min(app.saveFilesList.size(), static_cast<std::size_t>(app.saveSelectScroll + visibleSlots));
         ++i) {
      const auto& info = app.saveFilesList[i];
      if (info.isActive) continue;

      const int itemY = static_cast<int>(listArea.y) + 8 + static_cast<int>(i - app.saveSelectScroll) * 70;
      const Rectangle slotRec{listArea.x + 8, static_cast<float>(itemY), listArea.width - 16, 64};
      const Rectangle loadBtn{slotRec.x + slotRec.width - 100, slotRec.y + 15, 88, 34};

      if (CheckCollisionPointRec(mouse, loadBtn)) {
        switchSave(app, info.path);
        return;
      }
    }
  }

  if (app.newSaveInputFocused) {
    int key = GetCharPressed();
    while (key > 0) {
      if ((key >= 32) && (key <= 125) && app.newSaveInputName.length() < 24) {
        if (std::isalnum(key) || key == '-' || key == '_' || key == ' ') {
          app.newSaveInputName.push_back(static_cast<char>(key));
        }
      }
      key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && !app.newSaveInputName.empty()) {
      app.newSaveInputName.pop_back();
    }
    if (IsKeyPressed(KEY_ENTER)) {
      createNewSave(app, app.newSaveInputName);
      return;
    }
  }
}

void drawSaveSelectModal(const AppState& app) {
  if (!app.showSaveSelect) return;

  DrawRectangle(0, 0, kWidth, kHeight, Color{0, 0, 0, 215});

  const Rectangle modalRec{280, 80, 720, 560};
  DrawRectangleRounded(modalRec, 0.04F, 8, Color{24, 23, 34, 255});
  DrawRectangleRoundedLines(modalRec, 0.04F, 8, Color{93, 78, 55, 255});

  DrawText("LOCAL SAVE FILES", static_cast<int>(modalRec.x) + 30, static_cast<int>(modalRec.y) + 22, 22,
           Color{248, 211, 89, 255});
  const std::string activeText = "Active Save: " + app.savePath.filename().string();
  DrawText(activeText.c_str(), static_cast<int>(modalRec.x) + 30, static_cast<int>(modalRec.y) + 52, 14,
           Color{170, 170, 190, 255});

  const Rectangle closeBtn{modalRec.x + modalRec.width - 45, modalRec.y + 18, 28, 28};
  const Vector2 mouse = GetMousePosition();
  const bool hoverClose = CheckCollisionPointRec(mouse, closeBtn);
  DrawRectangleRounded(closeBtn, 0.2F, 4,
                       hoverClose ? Color{180, 50, 50, 255} : Color{50, 48, 62, 255});
  DrawText("X", static_cast<int>(closeBtn.x) + 8, static_cast<int>(closeBtn.y) + 5, 18, RAYWHITE);

  DrawLine(static_cast<int>(modalRec.x) + 20, static_cast<int>(modalRec.y) + 78,
           static_cast<int>(modalRec.x + modalRec.width) - 20, static_cast<int>(modalRec.y) + 78,
           Color{60, 58, 76, 255});

  const Rectangle listArea{modalRec.x + 20, modalRec.y + 88, modalRec.width - 40, 365};
  DrawRectangleRounded(listArea, 0.03F, 6, Color{18, 17, 26, 255});
  DrawRectangleRoundedLines(listArea, 0.03F, 6, Color{45, 43, 60, 255});

  const int visibleSlots = 5;
  const int maxScroll = std::max(0, static_cast<int>(app.saveFilesList.size()) - visibleSlots);

  if (app.saveFilesList.empty()) {
    DrawText("No save files found.", static_cast<int>(listArea.x) + 30, static_cast<int>(listArea.y) + 30,
             16, Color{160, 160, 170, 255});
  } else {
    for (std::size_t i = static_cast<std::size_t>(app.saveSelectScroll);
         i < std::min(app.saveFilesList.size(), static_cast<std::size_t>(app.saveSelectScroll + visibleSlots));
         ++i) {
      const auto& info = app.saveFilesList[i];
      const int itemY = static_cast<int>(listArea.y) + 8 + static_cast<int>(i - app.saveSelectScroll) * 70;
      const Rectangle slotRec{listArea.x + 8, static_cast<float>(itemY), listArea.width - 16, 64};

      if (info.isActive) {
        DrawRectangleRounded(slotRec, 0.08F, 4, Color{32, 42, 38, 255});
        DrawRectangleRoundedLines(slotRec, 0.08F, 4, Color{70, 160, 100, 255});
      } else {
        DrawRectangleRounded(slotRec, 0.08F, 4, Color{28, 27, 38, 255});
        DrawRectangleRoundedLines(slotRec, 0.08F, 4, Color{55, 52, 70, 255});
      }

      DrawText(info.filename.c_str(), static_cast<int>(slotRec.x) + 16, itemY + 11, 17,
               info.isActive ? Color{255, 235, 140, 255} : RAYWHITE);
      if (info.isActive) {
        const int nameW = MeasureText(info.filename.c_str(), 17);
        DrawText("[ACTIVE]", static_cast<int>(slotRec.x) + 24 + nameW, itemY + 13, 13,
                 Color{110, 240, 150, 255});
      }

      if (info.valid) {
        const std::string details = "Lv " + std::to_string(info.level) +
                                    "   |   Gold " + std::to_string(info.gold) +
                                    "   |   Bosses " + std::to_string(info.bossesCleared) + "/10" +
                                    "   |   Ascensions " + std::to_string(info.ascensions);
        DrawText(details.c_str(), static_cast<int>(slotRec.x) + 16, itemY + 36, 13,
                 Color{175, 175, 190, 255});
      } else {
        DrawText("[Corrupted or empty save]", static_cast<int>(slotRec.x) + 16, itemY + 36, 13,
                 Color{230, 100, 100, 255});
      }

      if (info.isActive) {
        const Rectangle statusRec{slotRec.x + slotRec.width - 100, slotRec.y + 15, 88, 34};
        DrawRectangleRounded(statusRec, 0.2F, 4, Color{30, 65, 42, 255});
        DrawText("CURRENT", static_cast<int>(statusRec.x) + 12, static_cast<int>(statusRec.y) + 9, 13,
                 Color{150, 240, 170, 255});
      } else {
        const Rectangle loadBtn{slotRec.x + slotRec.width - 100, slotRec.y + 15, 88, 34};
        const bool hoverLoad = CheckCollisionPointRec(mouse, loadBtn);
        DrawRectangleRounded(loadBtn, 0.2F, 4,
                             hoverLoad ? Color{65, 95, 160, 255} : Color{45, 68, 120, 255});
        DrawRectangleRoundedLines(loadBtn, 0.2F, 4, Color{110, 155, 240, 255});
        DrawText("LOAD", static_cast<int>(loadBtn.x) + 25, static_cast<int>(loadBtn.y) + 9, 14,
                 RAYWHITE);
      }
    }
  }

  if (maxScroll > 0) {
    const std::string scrollHint = "Scroll: Mouse Wheel (" +
                                   std::to_string(app.saveSelectScroll + 1) + "-" +
                                   std::to_string(std::min(static_cast<int>(app.saveFilesList.size()),
                                                           app.saveSelectScroll + visibleSlots)) +
                                   " of " + std::to_string(app.saveFilesList.size()) + ")";
    DrawText(scrollHint.c_str(), static_cast<int>(modalRec.x) + 30, static_cast<int>(modalRec.y) + 460,
             13, Color{140, 140, 160, 255});
  }

  DrawLine(static_cast<int>(modalRec.x) + 20, static_cast<int>(modalRec.y) + 482,
           static_cast<int>(modalRec.x + modalRec.width) - 20, static_cast<int>(modalRec.y) + 482,
           Color{60, 58, 76, 255});

  DrawText("Create New Save Slot:", static_cast<int>(modalRec.x) + 30,
           static_cast<int>(modalRec.y) + 494, 14, Color{200, 200, 215, 255});

  const Rectangle inputRec{modalRec.x + 30, modalRec.y + 514, 480, 36};
  DrawRectangleRounded(inputRec, 0.15F, 4,
                       app.newSaveInputFocused ? Color{32, 31, 46, 255} : Color{24, 23, 34, 255});
  DrawRectangleRoundedLines(inputRec, 0.15F, 4,
                            app.newSaveInputFocused ? Color{248, 211, 89, 255}
                                                    : Color{70, 68, 88, 255});

  if (app.newSaveInputName.empty() && !app.newSaveInputFocused) {
    DrawText("Enter slot or character name...", static_cast<int>(inputRec.x) + 12,
             static_cast<int>(inputRec.y) + 9, 14, Color{120, 120, 135, 255});
  } else {
    const std::string cursor =
        app.newSaveInputFocused && (static_cast<int>(GetTime() * 2) % 2 == 0) ? "_" : "";
    const std::string disp = app.newSaveInputName + cursor;
    DrawText(disp.c_str(), static_cast<int>(inputRec.x) + 12, static_cast<int>(inputRec.y) + 9, 15,
             RAYWHITE);
  }

  const Rectangle createBtn{inputRec.x + inputRec.width + 15, inputRec.y, 165, 36};
  const bool hoverCreate = CheckCollisionPointRec(mouse, createBtn);
  DrawRectangleRounded(createBtn, 0.15F, 4,
                       hoverCreate ? Color{55, 120, 80, 255} : Color{40, 90, 60, 255});
  DrawRectangleRoundedLines(createBtn, 0.15F, 4, Color{100, 210, 140, 255});
  DrawText("+ CREATE SAVE", static_cast<int>(createBtn.x) + 20, static_cast<int>(createBtn.y) + 10,
           14, WHITE);
}

} // namespace ed::app
