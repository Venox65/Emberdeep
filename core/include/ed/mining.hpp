#pragma once

#include <cstdint>
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
  std::int64_t maxHp = 0;
  int hardness = 0;
  std::int64_t xp = 0;
  int sell = 0;
  int rarityWeight = 0;
  Rarity rarity = Rarity::Common;
  int dropMultiplier = 1;
  int depth = 0;
  int aetherShardChanceBasisPoints = 0;
};

struct MiningProgress {
  std::int64_t currentHp = 0;
  std::int64_t maxHp = 0;
  int hardness = 0;
  std::int64_t xp = 0;
  int sell = 0;
  Rarity rarity = Rarity::Common;
  std::string materialId;
  std::string layerId;
  int depth = 0;
  int aetherShardChanceBasisPoints = 0;
};

int rarityWeightForLuck(int luck);
int weightedRarityRoll(Pcg32& rng, int luck);
BlockSpec generateBlockForLayer(const std::vector<LayerDef>& layers,
                               const std::vector<MaterialDef>& materials,
                               const std::string& selectedLayer,
                               int luck,
                               Pcg32& rng);
BlockSpec generateDeepStrataBlock(const std::vector<LayerDef>& layers,
                                const std::vector<MaterialDef>& materials,
                                int depth,
                                int luck,
                                Pcg32& rng);
BlockSpec generateDeepStrataBlock(int depth, int luck, Pcg32& rng);
MiningProgress createMiningProgress(const BlockSpec& spec);

} // namespace ed
