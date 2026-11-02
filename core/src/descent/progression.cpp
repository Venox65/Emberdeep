#include "ed/descent/combat_sim.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

namespace ed::descent {

void applyMetaToPlayer(DescentSimState& sim, const DescentSave& meta) {
  sim.selectedClass = meta.selectedClass;

  // Base Class Stats
  switch (meta.selectedClass) {
    case ClassId::Warrior:
      sim.player.maxHp = 130.0F;
      sim.player.armor = 4.0F;
      sim.player.moveSpeed = 190.0F;
      sim.player.might = 0.10F;
      sim.player.critChance = 0.08F;
      sim.player.critDamage = 0.50F;
      sim.player.area = 0.0F;
      sim.player.maxBarrier = 24.0F;
      break;
    case ClassId::Tank:
      sim.player.maxHp = 180.0F;
      sim.player.armor = 8.0F;
      sim.player.moveSpeed = 170.0F;
      sim.player.might = 0.0F;
      sim.player.critChance = 0.05F;
      sim.player.critDamage = 0.50F;
      sim.player.area = 0.15F;
      sim.player.damageReduction = 0.10F;
      sim.player.maxBarrier = 40.0F;
      break;
    case ClassId::Archer:
      sim.player.maxHp = 100.0F;
      sim.player.armor = 2.0F;
      sim.player.moveSpeed = 210.0F;
      sim.player.might = 0.05F;
      sim.player.critChance = 0.15F;
      sim.player.critDamage = 0.75F;
      sim.player.projectileSpeed = 0.20F;
      sim.player.maxBarrier = 20.0F;
      break;
    case ClassId::Mage:
      sim.player.maxHp = 100.0F;
      sim.player.armor = 2.0F;
      sim.player.moveSpeed = 190.0F;
      sim.player.might = 0.15F;
      sim.player.critChance = 0.05F;
      sim.player.critDamage = 0.50F;
      sim.player.cooldownReduction = 0.10F;
      sim.player.area = 0.20F;
      sim.player.maxBarrier = 25.0F;
      break;
    case ClassId::Summoner:
      sim.player.maxHp = 110.0F;
      sim.player.armor = 3.0F;
      sim.player.moveSpeed = 190.0F;
      sim.player.might = 0.05F;
      sim.player.critChance = 0.05F;
      sim.player.critDamage = 0.50F;
      sim.player.amount = 1;
      sim.player.duration = 0.20F;
      sim.player.maxBarrier = 25.0F;
      break;
  }

  // Anvil Ranks
  sim.player.might += static_cast<float>(meta.anvilMight) * 0.05F;
  sim.player.maxHp += static_cast<float>(meta.anvilVitality) * 10.0F;
  sim.player.moveSpeed *= (1.0F + static_cast<float>(meta.anvilSwiftness) * 0.04F);
  sim.player.pickupRadius += static_cast<float>(meta.anvilMagnetism) * 10.0F;
  sim.player.armor += static_cast<float>(meta.anvilResilience) * 1.0F;
  sim.player.greed += static_cast<float>(meta.anvilGreed) * 0.10F;
  sim.player.rerolls = 1 + meta.anvilRerolls;
  sim.player.banishes = 1 + meta.anvilBanishes;
  sim.player.skips = 1;

  // Provisions
  if (meta.provisionStew) {
    sim.player.maxHp *= 1.20F;
  }
  if (meta.provisionBrew) {
    sim.player.moveSpeed *= 1.15F;
  }
  if (meta.provisionOil) {
    sim.player.might += 0.15F;
  }

  // Sigil Gems
  for (const auto& gem : meta.sigilGems) {
    if (gem == "ruby") sim.player.might += 0.10F;
    else if (gem == "sapphire") sim.player.cooldownReduction = std::min(0.50F, sim.player.cooldownReduction + 0.08F);
    else if (gem == "emerald") sim.player.regen += 2.0F;
    else if (gem == "topaz") sim.player.attackSpeed += 0.10F;
    else if (gem == "diamond") sim.player.armor += 3.0F;
    else if (gem == "amethyst") sim.player.area += 0.15F;
  }

  // Set current HP to full
  sim.player.currentHp = sim.player.maxHp;
  sim.player.barrier = 0.0F;

  // Run length scaling: starts at 20 rooms, +1 room per won attempt up to max 40
  if (!sim.isEndless) {
    sim.totalChambers = std::clamp(20 + meta.victories, 20, 40);
  } else {
    sim.totalChambers = -1;
  }
  sim.chamberIndex = 1;

  // Heat & Pacts
  sim.heatLevel = meta.calculateTotalHeat();
  sim.heatDamageMult = 1.0F + static_cast<float>(meta.pactClaws) * 0.15F;
  sim.heatSpeedMult = 1.0F + static_cast<float>(meta.pactFrenzy) * 0.10F;
  sim.heatHpMult = 1.0F + static_cast<float>(meta.pactFlesh) * 0.20F;
  sim.eliteChance = 0.15F + static_cast<float>(meta.pactShadows) * 0.25F;

  // Set default arm for class
  sim.ownedArms.clear();
  sim.ownedTomes.clear();
  if (meta.selectedClass == ClassId::Warrior) {
    sim.ownedArms.push_back({"boomerang_axe", 1});
  } else if (meta.selectedClass == ClassId::Tank) {
    sim.ownedArms.push_back({"ember_censer", 1});
  } else if (meta.selectedClass == ClassId::Archer) {
    sim.ownedArms.push_back({"seeking_gale", 1});
  } else if (meta.selectedClass == ClassId::Mage) {
    sim.ownedArms.push_back({"chain_lightning_rod", 1});
  } else {
    sim.ownedArms.push_back({"skeletal_horde", 1});
  }
}

void initRoom(DescentSimState& sim, int widthTiles, int heightTiles) {
  sim.roomWidthTiles = widthTiles;
  sim.roomHeightTiles = heightTiles;
  const float worldW = static_cast<float>(widthTiles) * DescentSimState::kTileSize;
  const float worldH = static_cast<float>(heightTiles) * DescentSimState::kTileSize;

  sim.flowField.resize(widthTiles, heightTiles);
  sim.spatialHash.init(worldW, worldH);

  sim.playerPos = Vec2{worldW * 0.5F, worldH * 0.5F};
  sim.playerVel = Vec2{0.0F, 0.0F};
  sim.playerAim = Vec2{1.0F, 0.0F};

  sim.player.currentHp = sim.player.maxHp;
  sim.player.barrier = 0.0F;
  sim.player.dashCharges = sim.player.maxDashCharges;
  sim.player.dashRechargeTimer = 0.0F;
  sim.player.dashDurationTimer = 0.0F;
  sim.player.invulnerabilityTimer = 0.0F;
  sim.player.wrath = 0.0F;
  sim.player.skillCooldown = 0.0F;
  sim.player.primaryCooldown = 0.0F;
  sim.player.arrowCritCount = 0;
  sim.player.whirlTimer = 0.0F;
  sim.player.bulwarkTimer = 0.0F;
  sim.player.rainOfArrowsTimer = 0.0F;
  sim.player.fortressTimer = 0.0F;
  sim.player.frostNovaTimer = 0.0F;
  sim.player.meteorSwarmTimer = 0.0F;
  sim.player.soulHarvestTimer = 0.0F;

  sim.enemies.clear();
  sim.projectiles.clear();
  sim.pickups.clear();
  sim.damageEvents.clear();
  sim.thornPatches.clear();
  sim.friendlyMinions.clear();
  sim.telegraphs.clear();
  sim.frostRings.clear();
  sim.lightningArcs.clear();

  sim.censerTimer = 0.0F;
  sim.axeTimer = 0.0F;
  sim.daggersTimer = 0.0F;
  sim.lightningTimer = 0.0F;
  sim.frostBellTimer = 0.0F;
  sim.brambleTimer = 0.0F;
  sim.lanternTimer = 0.0F;
  sim.gravityWellTimer = 0.0F;

  sim.chamberIndex = 1;
  if (!sim.isEndless) {
    if (sim.totalChambers < 20) {
      sim.totalChambers = 20;
    }
  } else {
    sim.totalChambers = -1;
    sim.endlessLoop = 1;
  }
  sim.currentChamberType = ChamberType::Combat;
  sim.doorChoicesPending = false;
  sim.doorChoices.clear();
  sim.runCompleted = false;

  sim.currentWave = 1;
  sim.totalWaves = 2;
  sim.currentWaveSpawned = 0;
  sim.currentWaveKilled = 0;
  sim.roomCleared = false;
  sim.waveTimer = 0.0F;
  sim.bossDefeated = false;
  sim.bossEntityId = -1;
  sim.bossName.clear();
  sim.bossTitle.clear();
  sim.bossMaxHp = 1.0F;
  sim.bossCurrentHp = 0.0F;
  sim.bossPhase = 1;

  if (sim.ownedArms.empty()) {
    if (sim.selectedClass == ClassId::Warrior) {
      sim.ownedArms.push_back({"boomerang_axe", 1});
    } else if (sim.selectedClass == ClassId::Tank) {
      sim.ownedArms.push_back({"ember_censer", 1});
    } else if (sim.selectedClass == ClassId::Archer) {
      sim.ownedArms.push_back({"seeking_gale", 1});
    } else if (sim.selectedClass == ClassId::Mage) {
      sim.ownedArms.push_back({"chain_lightning_rod", 1});
    } else {
      sim.ownedArms.push_back({"skeletal_horde", 1});
    }
  }

  const int ptx = static_cast<int>(sim.playerPos.x / DescentSimState::kTileSize);
  const int pty = static_cast<int>(sim.playerPos.y / DescentSimState::kTileSize);
  sim.flowField.compute(ptx, pty);

  spawnWave(sim, 1);
}

void generateLevelUpChoices(DescentSimState& sim) {
  sim.levelUpPending = true;
  sim.levelUpChoices.clear();

  const std::vector<LevelUpCard> pool = {
    // Tomes
    {"heft", "Tome of Heft", "+8% Might (Damage)", "Tome", 0, 5},
    {"quickening", "Tome of Quickening", "+6% Attack Speed", "Tome", 0, 5},
    {"vitality", "Tome of Vitality", "+10% Max HP & Heals", "Tome", 0, 5},
    {"swiftness", "Tome of Swiftness", "+5% Move Speed", "Tome", 0, 5},
    {"plating", "Tome of Plating", "+1 Armor", "Tome", 0, 5},
    {"lodestone", "Tome of Lodestone", "+15% Pickup Radius", "Tome", 0, 5},

    // Arms
    {"ember_censer", "Ember Censer", "Burns nearby enemies with fire aura", "Arm", 0, 5},
    {"throwing_daggers", "Throwing Daggers", "Hurls rapid piercing daggers", "Arm", 0, 5},
    {"orbiting_runeblades", "Runeblades", "Spinning blades shield and shred", "Arm", 0, 5},
    {"chain_lightning_rod", "Lightning Rod", "Strikes enemies with bouncing bolts", "Arm", 0, 5},
    {"frost_bell", "Frost Bell", "Rings with chilling waves, freezing foes", "Arm", 0, 5},
    {"thorn_bramble", "Thorn Bramble", "Leaves damaging thorn patches on ground", "Arm", 0, 5},
    {"boomerang_axe", "Boomerang Axe", "Hurls heavy returning battleaxe", "Arm", 0, 5},
    {"holy_lantern", "Holy Lantern", "Sweeps holy light that burns corruption", "Arm", 0, 5},
    {"gravity_well", "Gravity Well", "Pulls enemies together and crushes them", "Arm", 0, 5},
    {"skeletal_horde", "Skeletal Horde", "Summons loyal skeletons to fight by your side", "Arm", 0, 5},
    {"soul_siphon", "Soul Siphon", "Pulses soul siphon wards, damaging foes & stealing HP", "Arm", 0, 5},
    {"arcane_orb", "Arcane Orb", "Fires a massive piercing mana orb that detonates into bolts", "Arm", 0, 5},
    {"seeking_gale", "Seeking Gale", "Rapid homing wind arrows track down distant foes", "Arm", 0, 5},
    {"earth_tremor", "Earth Tremor", "Slams the ground with shockwaves that knock enemies back", "Arm", 0, 5}
  };

  std::vector<LevelUpCard> available;
  for (const auto& card : pool) {
    if (std::find(sim.banishedIds.begin(), sim.banishedIds.end(), card.id) != sim.banishedIds.end()) {
      continue;
    }
    LevelUpCard c = card;
    for (const auto& owned : sim.ownedTomes) {
      if (owned.id == c.id) c.currentLevel = owned.level;
    }
    for (const auto& owned : sim.ownedArms) {
      if (owned.id == c.id) c.currentLevel = owned.level;
    }
    if (c.currentLevel < c.maxLevel) {
      available.push_back(c);
    }
  }

  // Weight class-aligned skills so each class reliably discovers their archetype
  std::vector<LevelUpCard> weightedAvailable;
  for (const auto& card : available) {
    int weight = 1;
    if (sim.selectedClass == ClassId::Summoner) {
      if (card.id == "skeletal_horde" || card.id == "soul_siphon" || card.id == "thorn_bramble" || card.id == "zeal") weight = 4;
    } else if (sim.selectedClass == ClassId::Mage) {
      if (card.id == "arcane_orb" || card.id == "chain_lightning_rod" || card.id == "frost_bell" || card.id == "gravity_well") weight = 4;
    } else if (sim.selectedClass == ClassId::Archer) {
      if (card.id == "seeking_gale" || card.id == "throwing_daggers" || card.id == "boomerang_axe" || card.id == "quickening") weight = 4;
    } else if (sim.selectedClass == ClassId::Warrior) {
      if (card.id == "earth_tremor" || card.id == "boomerang_axe" || card.id == "orbiting_runeblades" || card.id == "heft") weight = 4;
    } else if (sim.selectedClass == ClassId::Tank) {
      if (card.id == "earth_tremor" || card.id == "ember_censer" || card.id == "holy_lantern" || card.id == "plating") weight = 4;
    }
    for (int w = 0; w < weight; ++w) {
      weightedAvailable.push_back(card);
    }
  }

  // Shuffle weighted pool and pick up to 3 distinct cards
  for (std::size_t i = weightedAvailable.size(); i > 1; --i) {
    const std::size_t j = static_cast<std::size_t>(rand()) % i;
    std::swap(weightedAvailable[i - 1], weightedAvailable[j]);
  }

  for (const auto& card : weightedAvailable) {
    if (sim.levelUpChoices.size() >= 3) break;
    bool alreadyChosen = false;
    for (const auto& existing : sim.levelUpChoices) {
      if (existing.id == card.id) {
        alreadyChosen = true;
        break;
      }
    }
    if (!alreadyChosen) {
      sim.levelUpChoices.push_back(card);
    }
  }

  // Fallback healing card
  while (sim.levelUpChoices.size() < 3) {
    sim.levelUpChoices.push_back({"heal", "First Aid", "Restore 30% Max HP", "Heal", 1, 1});
  }
}

bool selectLevelUpChoice(DescentSimState& sim, int index) {
  if (index < 0 || static_cast<std::size_t>(index) >= sim.levelUpChoices.size()) return false;
  const auto& card = sim.levelUpChoices[static_cast<std::size_t>(index)];

  if (card.id == "heft") {
    sim.player.might += 0.08F;
  } else if (card.id == "quickening") {
    sim.player.attackSpeed += 0.06F;
  } else if (card.id == "vitality") {
    sim.player.maxHp *= 1.10F;
    sim.player.currentHp += sim.player.maxHp * 0.10F;
  } else if (card.id == "swiftness") {
    sim.player.moveSpeed *= 1.05F;
  } else if (card.id == "plating") {
    sim.player.armor += 1.0F;
  } else if (card.id == "lodestone") {
    sim.player.pickupRadius += 12.0F;
  } else if (card.id == "heal") {
    sim.player.currentHp = std::min(sim.player.maxHp, sim.player.currentHp + sim.player.maxHp * 0.30F);
  }

  if (card.type == "Tome") {
    bool found = false;
    for (auto& t : sim.ownedTomes) {
      if (t.id == card.id) { t.level++; found = true; break; }
    }
    if (!found) sim.ownedTomes.push_back({card.id, 1});
  } else if (card.type == "Arm") {
    bool found = false;
    for (auto& a : sim.ownedArms) {
      if (a.id == card.id) { a.level++; found = true; break; }
    }
    if (!found) sim.ownedArms.push_back({card.id, 1});
  }

  sim.levelUpPending = false;
  sim.levelUpChoices.clear();
  return true;
}

bool rerollLevelUpChoices(DescentSimState& sim) {
  if (sim.player.rerolls <= 0) return false;
  sim.player.rerolls--;
  generateLevelUpChoices(sim);
  return true;
}

bool banishLevelUpChoice(DescentSimState& sim, int index) {
  if (sim.player.banishes <= 0) return false;
  if (index < 0 || static_cast<std::size_t>(index) >= sim.levelUpChoices.size()) return false;
  sim.player.banishes--;
  sim.banishedIds.push_back(sim.levelUpChoices[static_cast<std::size_t>(index)].id);
  generateLevelUpChoices(sim);
  return true;
}

bool skipLevelUpChoice(DescentSimState& sim) {
  if (sim.player.skips <= 0) return false;
  sim.player.skips--;
  sim.player.currentHp = std::min(sim.player.maxHp, sim.player.currentHp + sim.player.maxHp * 0.10F);
  sim.levelUpPending = false;
  sim.levelUpChoices.clear();
  return true;
}

void generateChamberChoices(DescentSimState& sim) {
  sim.doorChoices.clear();
  const float worldW = static_cast<float>(sim.roomWidthTiles) * DescentSimState::kTileSize;
  const float doorY = 70.0F;

  const bool isStandardBoss = (!sim.isEndless && sim.chamberIndex + 1 >= sim.totalChambers);
  const bool isEndlessBoss = (sim.isEndless && (sim.chamberIndex + 1) % 10 == 0);

  if (isStandardBoss || isEndlessBoss) {
    // Boss stage: only 1 entrance, no buffs to the player
    ChamberChoice bossDoor;
    bossDoor.type = ChamberType::Boss;
    bossDoor.iconType = "boss";
    bossDoor.name = sim.isEndless ? ("Catacomb Guardian's Sanctum (Loop " + std::to_string(sim.endlessLoop) + ")") : "Grave Warden's Sanctum";
    bossDoor.description = sim.isEndless ? "A colossal Catacomb Overlord guards the threshold to the next loop." : "The final threshold. Grave Warden Osric awaits.";
    bossDoor.rewardPreview = "Boss Stage";
    bossDoor.rewardType = "none";
    bossDoor.doorPos = Vec2{worldW * 0.50F, doorY};
    sim.doorChoices.push_back(bossDoor);
    return;
  }

  // 4 standard chamber types:
  // 1. Sword icon for combat stage
  ChamberChoice combatDoor;
  combatDoor.type = ChamberType::Combat;
  combatDoor.iconType = "sword";
  combatDoor.name = "Combat Chamber";
  combatDoor.description = "Face the horde.";
  combatDoor.rewardPreview = "Combat Stage";
  combatDoor.rewardType = "none";

  // 2. Green plus for healing -> enters full combat stage
  ChamberChoice healDoor;
  healDoor.type = ChamberType::Combat;
  healDoor.iconType = "heal";
  healDoor.name = "Healing Sanctuary";
  healDoor.description = "Restores 35% Max HP.";
  healDoor.rewardPreview = "Healing Stage";
  healDoor.rewardType = "heal";

  // 3. Arrow pointing up for level up -> enters full combat stage
  ChamberChoice levelDoor;
  levelDoor.type = ChamberType::Combat;
  levelDoor.iconType = "levelup";
  levelDoor.name = "Ascension Chamber";
  levelDoor.description = "Grants +1 instant level up.";
  levelDoor.rewardPreview = "Level Up Stage";
  levelDoor.rewardType = "levelup";

  // 4. Skull for elite stage
  ChamberChoice eliteDoor;
  eliteDoor.type = ChamberType::Elite;
  eliteDoor.iconType = "skull";
  eliteDoor.name = "Elite Trial";
  eliteDoor.description = "Elite foes & Captain Rattlewick.";
  eliteDoor.rewardPreview = "Elite Stage";
  eliteDoor.rewardType = "elite";

  // 5. Chest for rare treasure stage (~1% chance)
  ChamberChoice treasureDoor;
  treasureDoor.type = ChamberType::Treasure;
  treasureDoor.iconType = "chest";
  treasureDoor.name = "Vault of Riches";
  treasureDoor.description = "A sealed chamber containing a locked gem chest.";
  treasureDoor.rewardPreview = "Treasure Stage";
  treasureDoor.rewardType = "treasure";

  // Select 3 distinct doors:
  std::vector<ChamberChoice> selected;
  if (sim.player.currentHp < sim.player.maxHp * 0.35F) {
    selected = {combatDoor, healDoor, (sim.chamberIndex % 2 == 0 ? eliteDoor : levelDoor)};
  } else if (sim.chamberIndex % 3 == 1) {
    selected = {combatDoor, levelDoor, eliteDoor};
  } else if (sim.chamberIndex % 3 == 2) {
    selected = {combatDoor, healDoor, eliteDoor};
  } else {
    selected = {healDoor, combatDoor, levelDoor};
  }

  // Rare treasure vault roll (~1% chance)
  if ((rand() % 100) == 0) {
    selected[1] = treasureDoor;
  }

  selected[0].doorPos = Vec2{worldW * 0.30F, doorY};
  selected[1].doorPos = Vec2{worldW * 0.50F, doorY};
  selected[2].doorPos = Vec2{worldW * 0.70F, doorY};

  for (const auto& ch : selected) {
    sim.doorChoices.push_back(ch);
  }
}

void initNextChamber(DescentSimState& sim, ChamberType type) {
  sim.currentChamberType = type;
  const float worldW = static_cast<float>(sim.roomWidthTiles) * DescentSimState::kTileSize;
  const float worldH = static_cast<float>(sim.roomHeightTiles) * DescentSimState::kTileSize;

  // Reset player position and movement
  sim.playerPos = Vec2{worldW * 0.5F, worldH * 0.5F};
  sim.playerVel = Vec2{0.0F, 0.0F};
  sim.player.dashDurationTimer = 0.0F;
  sim.player.invulnerabilityTimer = 0.0F;

  // Clear per-chamber transient entities
  sim.enemies.clear();
  sim.projectiles.clear();
  sim.damageEvents.clear();
  sim.thornPatches.clear();
  sim.telegraphs.clear();
  sim.pickups.clear();
  sim.frostRings.clear();
  sim.lightningArcs.clear();

  // Reset boss tracking if not boss chamber
  if (type != ChamberType::Boss && (sim.isEndless || sim.chamberIndex < sim.totalChambers)) {
    sim.bossDefeated = false;
    sim.bossEntityId = -1;
    sim.bossName.clear();
    sim.bossTitle.clear();
    sim.bossMaxHp = 1.0F;
    sim.bossCurrentHp = 0.0F;
    sim.bossPhase = 1;
  }

  // Chamber wave configuration
  sim.currentWave = 1;
  sim.currentWaveSpawned = 0;
  sim.currentWaveKilled = 0;
  sim.roomCleared = false;
  sim.doorChoicesPending = false;
  sim.doorChoices.clear();
  sim.waveTimer = 0.0F;

  if (type == ChamberType::Treasure) {
    sim.totalWaves = 0;
    return;
  }

  if (type == ChamberType::Boss || (!sim.isEndless && sim.chamberIndex == sim.totalChambers)) {
    sim.totalWaves = 1;
  } else if (type == ChamberType::Elite) {
    sim.totalWaves = 4;
  } else {
    if (sim.chamberIndex <= 5) {
      sim.totalWaves = 3;
    } else if (sim.chamberIndex <= 12) {
      sim.totalWaves = 4;
    } else {
      sim.totalWaves = 5;
    }
  }

  // Recompute flow field for center
  const int ptx = static_cast<int>(sim.playerPos.x / DescentSimState::kTileSize);
  const int pty = static_cast<int>(sim.playerPos.y / DescentSimState::kTileSize);
  sim.flowField.compute(ptx, pty);

  // Spawn initial wave
  spawnChamberWave(sim, 1, type);
}

bool selectChamberChoice(DescentSimState& sim, int index) {
  if (index < 0 || static_cast<std::size_t>(index) >= sim.doorChoices.size()) {
    return false;
  }

  const ChamberChoice choice = sim.doorChoices[static_cast<std::size_t>(index)];

  // Apply entrance reward (no buffs for combat or boss stage)
  if (choice.rewardType == "heal") {
    sim.player.currentHp = std::min(sim.player.maxHp, sim.player.currentHp + sim.player.maxHp * 0.35F);
  } else if (choice.rewardType == "levelup") {
    sim.runLevel++;
    generateLevelUpChoices(sim);
  } else if (choice.rewardType == "elite") {
    sim.embers += 100;
    sim.soulFragments += 1;
  }

  sim.chamberIndex++;
  initNextChamber(sim, choice.type);
  return true;
}

} // namespace ed::descent
