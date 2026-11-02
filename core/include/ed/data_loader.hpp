#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "ed/campaign.hpp"
#include "ed/data_model.hpp"
#include "ed/dungeon.hpp"

namespace ed {

bool loadMaterialsFromJsonFile(const std::filesystem::path& path, MaterialList& out);
bool loadLayersFromJsonFile(const std::filesystem::path& path, LayerList& out);
bool loadBossesFromJsonFile(const std::filesystem::path& path, std::vector<BossDef>& out);
bool loadDungeonZonesFromJsonFile(const std::filesystem::path& path, std::vector<DungeonZoneDef>& out);
bool loadGemsFromJsonFile(const std::filesystem::path& path, GemList& out);

struct LoadedGameData {
  MaterialList materials;
  LayerList layers;
  std::vector<BossDef> bosses;
  std::vector<DungeonZoneDef> zones;
  GemList gems;
  bool loadedFromDisk = false;
};

LoadedGameData loadGameData(const std::filesystem::path& dataDir = "data");

} // namespace ed
