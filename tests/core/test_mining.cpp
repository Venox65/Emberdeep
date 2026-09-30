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
