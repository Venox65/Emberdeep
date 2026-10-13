#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "app_state.hpp"

namespace ed::app {

void configureSaveStorage(AppState& app);
void setLastPlayedSave(const std::filesystem::path& path);
std::vector<SaveFileInfo> scanSaveFiles(const std::filesystem::path& currentSavePath);
void requestSave(AppState& app);
void saveNow(AppState& app);
void startSaveWorker(AppState& app);
void stopSaveWorker(AppState& app);
void loadSave(AppState& app);
void switchSave(AppState& app, const std::filesystem::path& newPath);
void createNewSave(AppState& app, const std::string& rawName);

void handleSaveSelectInput(AppState& app);
void drawSaveSelectModal(const AppState& app);

} // namespace ed::app
