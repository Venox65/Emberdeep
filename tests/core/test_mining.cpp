#include "ed/mining.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("block generation uses the layer tier distribution and rarity math") {
  ed::Pcg32 rng(42);
  const auto layers = ed::defaultLayers();
  const auto materials = ed::defaultMaterials();

  const auto block = ed::generateBlockForLayer(layers, materials, "iron-hollows", 0, rng);

  REQUIRE(block.layerId == "iron-hollows");
  REQUIRE(block.maxHp > 0);
  REQUIRE(block.hardness >= 0);
  REQUIRE(block.xp > 0);
  REQUIRE(block.rarityWeight >= 0);
}

TEST_CASE("mining progress reflects the generated block values") {
  ed::BlockSpec spec;
  spec.materialId = "iron";
  spec.layerId = "iron-hollows";
  spec.maxHp = 135;
  spec.hardness = 10;
  spec.xp = 29;
  spec.sell = 8;
  spec.rarity = ed::Rarity::Rare;

  const auto progress = ed::createMiningProgress(spec);

  REQUIRE(progress.currentHp == 135);
  REQUIRE(progress.maxHp == 135);
  REQUIRE(progress.hardness == 10);
  REQUIRE(progress.xp == 29);
  REQUIRE(progress.sell == 8);
  REQUIRE(progress.rarity == ed::Rarity::Rare);
}

TEST_CASE("all post-Iron layers generate their own materials") {
  const auto layers = ed::defaultLayers();
  const auto materials = ed::defaultMaterials();

  for (const auto& layer : layers) {
    if (layer.tier < 4) {
      continue;
    }

    ed::Pcg32 rng(static_cast<std::uint64_t>(layer.tier));
    bool generatedTargetTier = false;
    for (int attempt = 0; attempt < 100; ++attempt) {
      const auto block =
          ed::generateBlockForLayer(layers, materials, layer.id, 0, rng);
      REQUIRE(block.materialId != "ashen-stone");
      for (const auto& material : materials) {
        if (material.id == block.materialId && material.layer == layer.id) {
          generatedTargetTier = true;
        }
      }
    }
    REQUIRE(generatedTargetTier);
  }
}

TEST_CASE("deep strata block generation scales stats and calculates shard drop chances") {
  ed::Pcg32 rng(12345);
  const auto layers = ed::defaultLayers();
  const auto materials = ed::defaultMaterials();

  // Depth 1
  const auto d1 = ed::generateDeepStrataBlock(layers, materials, 1, 0, rng);
  REQUIRE(d1.depth == 1);
  REQUIRE(d1.layerId == "aether-vault");
  REQUIRE(d1.materialId == "aetherium");
  REQUIRE(d1.maxHp >= 282620); // 108700 * 2.6
  REQUIRE(d1.xp >= 31680);     // 13200 * 2.4
  REQUIRE(d1.dropMultiplier >= 2); // 1 + 0.5*1 = 1.5 -> 2
  REQUIRE(d1.aetherShardChanceBasisPoints == 250); // 1 * 250

  const auto progress = ed::createMiningProgress(d1);
  REQUIRE(progress.depth == 1);
  REQUIRE(progress.currentHp == d1.maxHp);
  REQUIRE(progress.aetherShardChanceBasisPoints == 250);

  // Depth 5
  const auto d5 = ed::generateDeepStrataBlock(layers, materials, 5, 0, rng);
  REQUIRE(d5.depth == 5);
  REQUIRE(d5.maxHp > d1.maxHp);
  REQUIRE(d5.xp > d1.xp);
  REQUIRE(d5.dropMultiplier >= 4); // 1 + 0.5*5 = 3.5 -> 4
  REQUIRE(d5.aetherShardChanceBasisPoints == 1250); // 5 * 250 = 1250 bp (12.5%)

  // Standalone overload without passing layers/materials
  const auto dStandalone = ed::generateDeepStrataBlock(3, 0, rng);
  REQUIRE(dStandalone.depth == 3);
  REQUIRE(dStandalone.materialId == "aetherium");
  REQUIRE(dStandalone.aetherShardChanceBasisPoints == 750);
}
