#pragma once

#include <string>
#include <vector>

#include "ed/data_model.hpp"
#include "ed/pcg.hpp"

namespace ed {

enum class Rarity {
  Common,
  Uncommon,
  Rare,
  Epic,
};

struct BlockSpec {
  std::string layerId;
  std::string materialId;
  std::string materialName;
  std::string color;
  int maxHp = 0;
  int hardness = 0;
  int xp = 0;
  int sell = 0;
  int rarityWeight = 0;
  Rarity rarity = Rarity::Common;
  int dropMultiplier = 1;
};

struct MiningProgress {
  int currentHp = 0;
  int maxHp = 0;
  int hardness = 0;
  int xp = 0;
  int sell = 0;
  Rarity rarity = Rarity::Common;
  std::string materialId;
  std::string layerId;
};

int rarityWeightForLuck(int luck);
int weightedRarityRoll(Pcg32& rng, int luck);
BlockSpec generateBlockForLayer(const std::vector<LayerDef>& layers,
                               const std::vector<MaterialDef>& materials,
                               const std::string& selectedLayer,
                               int luck,
                               Pcg32& rng);
MiningProgress createMiningProgress(const BlockSpec& spec);

} // namespace ed
