#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ed {

struct MaterialDef {
  std::string id;
  std::string name;
  std::string layer;
  std::string color;
  int hp = 0;
  int hardness = 0;
  int xp = 0;
  int sell = 0;
  int minLevel = 1;
};

struct LayerDef {
  std::string id;
  std::string name;
  int tier = 1;
  int minLevel = 1;
  int requiredPickaxeTier = 0;
};

struct GemDef {
  std::string id;
  std::string name;
  std::string rarity;
  std::string effect;
  int priority = 0;
};

struct VowNodeDef {
  std::string branch;
  std::string id;
  std::string label;
  int maxRank = 1;
  std::string description;
};

struct PlayerProgress {
  int level = 1;
  int ascensions = 0;
  int echoRank = 0;
  int bossesCleared = 0;
  int lifetimeGold = 0;
};

struct MiningResult {
  double rawDamage = 0.0;
  double effectiveHardness = 0.0;
  double finalDamage = 0.0;
};

using MaterialList = std::vector<MaterialDef>;
using LayerList = std::vector<LayerDef>;
using GemList = std::vector<GemDef>;
using VowNodeList = std::vector<VowNodeDef>;

} // namespace ed
