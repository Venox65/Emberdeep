#pragma once

#include <filesystem>
#include <string>

#include "ed/campaign.hpp"
#include "ed/game_state.hpp"
#include "ed/inventory.hpp"

namespace ed {

struct SaveSnapshot {
  GameState game;
  InventoryState inventory;
  CampaignState campaign;
};

std::string serializeSaveSnapshot(const SaveSnapshot& snapshot);
bool deserializeSaveSnapshot(const std::string& text, SaveSnapshot& snapshot);
bool validateSaveSnapshot(const SaveSnapshot& snapshot);
void sanitizeSaveSnapshot(SaveSnapshot& snapshot);
bool writeSaveAtomic(const std::filesystem::path& path, const SaveSnapshot& snapshot);
bool readSave(const std::filesystem::path& path, SaveSnapshot& snapshot);

} // namespace ed
