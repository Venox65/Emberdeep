#include "ed/mining.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "ed/formulas.hpp"
#include "ed/postgame.hpp"

namespace ed {

namespace {

int clampToRange(int value, int minValue, int maxValue) {
  return std::max(minValue, std::min(value, maxValue));
}

const LayerDef* findLayerById(const std::vector<LayerDef>& layers, const std::string& id) {
  for (const auto& layer : layers) {
    if (layer.id == id) {
      return &layer;
    }
  }
  return nullptr;
}

const MaterialDef* findMaterialForTier(const std::vector<LayerDef>& layers,
                                       const std::vector<MaterialDef>& materials,
                                       int tier) {
  for (const auto& layer : layers) {
    if (layer.tier != tier) {
      continue;
    }
    for (const auto& material : materials) {
      if (material.layer == layer.id) {
        return &material;
      }
    }
  }
  return nullptr;
}

Rarity rarityFromIndex(int index) {
  switch (index) {
    case 0:
      return Rarity::Common;
    case 1:
      return Rarity::Uncommon;
    case 2:
      return Rarity::Rare;
    default:
      return Rarity::Epic;
  }
}

int rarityMultiplier(Rarity rarity) {
  switch (rarity) {
    case Rarity::Common:
      return 1;
    case Rarity::Uncommon:
      return 2;
    case Rarity::Rare:
      return 3;
    case Rarity::Epic:
      return 6;
  }
  return 1;
}

double rarityHpMultiplier(Rarity rarity) {
  switch (rarity) {
    case Rarity::Common:
      return 1.0;
    case Rarity::Uncommon:
      return 1.5;
    case Rarity::Rare:
      return 2.5;
    case Rarity::Epic:
      return 5.0;
  }
  return 1.0;
}

double rarityXpMultiplier(Rarity rarity) {
  switch (rarity) {
    case Rarity::Common:
      return 1.0;
    case Rarity::Uncommon:
      return 1.5;
    case Rarity::Rare:
      return 3.0;
    case Rarity::Epic:
      return 5.0;
  }
  return 1.0;
}

} // namespace

int rarityWeightForLuck(int luck) {
  return clampToRange(luck, 0, 15);
}

int weightedRarityRoll(Pcg32& rng, int luck) {
  const int luckShift = rarityWeightForLuck(luck);
  const int commonBase = 70;
  const int uncommonBase = 22;
  const int rareBase = 6;
  const int epicBase = 1;

  const int commonWeight = commonBase - luckShift;
  const int uncommonWeight = uncommonBase + static_cast<int>(std::round((luckShift * 0.55) * 1.0));
  const int rareWeight = rareBase + static_cast<int>(std::round((luckShift * 0.35) * 1.0));
  const int epicWeight = epicBase + static_cast<int>(std::round((luckShift * 0.10) * 1.0));

  const int total = commonWeight + uncommonWeight + rareWeight + epicWeight;
  const int roll = static_cast<int>(rng.next() % static_cast<std::uint32_t>(std::max(1, total)));

  int threshold = commonWeight;
  if (roll < threshold) {
    return 0;
  }
  threshold += uncommonWeight;
  if (roll < threshold) {
    return 1;
  }
  threshold += rareWeight;
  if (roll < threshold) {
    return 2;
  }
  return 3;
}

BlockSpec generateBlockForLayer(const std::vector<LayerDef>& layers,
                               const std::vector<MaterialDef>& materials,
                               const std::string& selectedLayer,
                               int luck,
                               Pcg32& rng) {
  const LayerDef* layer = findLayerById(layers, selectedLayer);
  if (!layer) {
    throw std::invalid_argument("selected layer not found");
  }

  int tier = layer->tier;
  const int roll = static_cast<int>(rng.next() % 100);
  if (roll < 75) {
    tier = layer->tier;
  } else if (roll < 95 && layer->tier > 1) {
    tier = std::max(1, layer->tier - 1);
  } else if (layer->tier < 10) {
    tier = std::min(10, layer->tier + 1);
  }

  const MaterialDef* material = findMaterialForTier(layers, materials, tier);
  if (!material) {
    throw std::invalid_argument("no material definition exists for selected tier");
  }

  const Rarity rarity = rarityFromIndex(weightedRarityRoll(rng, luck));
  const int hp = static_cast<int>(std::llround(material->hp * rarityHpMultiplier(rarity)));
  const int xp = static_cast<int>(std::llround(material->xp * rarityXpMultiplier(rarity)));

  BlockSpec block;
  block.layerId = layer->id;
  block.materialId = material->id;
  block.materialName = material->name;
  block.color = material->color;
  block.maxHp = hp;
  block.hardness = material->hardness;
  block.xp = xp;
  block.sell = material->sell;
  block.rarityWeight = rarityWeightForLuck(luck);
  block.rarity = rarity;
  block.dropMultiplier = rarityMultiplier(rarity);

  return block;
}

BlockSpec generateDeepStrataBlock(const std::vector<LayerDef>& layers,
                                const std::vector<MaterialDef>& materials,
                                int depth,
                                int luck,
                                Pcg32& rng) {
  const int safeDepth = std::max(1, depth);
  const MaterialDef* material = findMaterialForTier(layers, materials, 10);

  const std::int64_t baseHp = material ? material->hp : 108700;
  const std::int64_t baseXp = material ? material->xp : 13200;
  const int hardness = material ? material->hardness : 300;
  const int sell = material ? material->sell : 4900;
  const std::string color = material ? material->color : "#7cf7e0";

  const auto dsb = deepStrataBlock(safeDepth, baseHp, baseXp);
  const Rarity rarity = rarityFromIndex(weightedRarityRoll(rng, luck));

  BlockSpec block;
  block.layerId = "aether-vault";
  block.materialId = "aetherium";
  block.materialName = "Aetherium";
  block.color = color;
  block.maxHp = static_cast<std::int64_t>(std::llround(dsb.hp * rarityHpMultiplier(rarity)));
  block.hardness = hardness;
  block.xp = static_cast<std::int64_t>(std::llround(dsb.xp * rarityXpMultiplier(rarity)));
  block.sell = sell;
  block.rarityWeight = rarityWeightForLuck(luck);
  block.rarity = rarity;
  block.dropMultiplier = static_cast<int>(dsb.oreQuantity * rarityMultiplier(rarity));
  block.depth = safeDepth;
  block.aetherShardChanceBasisPoints = dsb.aetherShardChanceBasisPoints;

  return block;
}

BlockSpec generateDeepStrataBlock(int depth, int luck, Pcg32& rng) {
  return generateDeepStrataBlock({}, {}, depth, luck, rng);
}

MiningProgress createMiningProgress(const BlockSpec& spec) {
  MiningProgress progress;
  progress.currentHp = spec.maxHp;
  progress.maxHp = spec.maxHp;
  progress.hardness = spec.hardness;
  progress.xp = spec.xp;
  progress.sell = spec.sell;
  progress.rarity = spec.rarity;
  progress.materialId = spec.materialId;
  progress.layerId = spec.layerId;
  progress.depth = spec.depth;
  progress.aetherShardChanceBasisPoints = spec.aetherShardChanceBasisPoints;
  return progress;
}

} // namespace ed
