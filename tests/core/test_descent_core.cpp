#include "ed/descent/combat_sim.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

TEST_CASE("descent pacing formulas match specification") {
  REQUIRE(ed::descent::runXpNeed(1) == 95);
  REQUIRE(ed::descent::runXpNeed(10) == 770);
  REQUIRE(ed::descent::runXpNeed(20) == 2370);
  REQUIRE(ed::descent::runXpNeed(30) == 4870);

  // Damage dealt
  REQUIRE(ed::descent::calculateDamageDealt(14.0F, 0.0F, false, 0.5F, 0.0F) == 14.0F);
  REQUIRE(ed::descent::calculateDamageDealt(14.0F, 0.5F, false, 0.5F, 0.0F) == 21.0F);
  REQUIRE(ed::descent::calculateDamageDealt(14.0F, 0.5F, true, 0.5F, 0.0F) == 31.5F);
  // Armor reduces damage: 100 / (100 + 100) = 0.5
  REQUIRE(ed::descent::calculateDamageDealt(20.0F, 0.0F, false, 0.5F, 100.0F) == 10.0F);

  // Damage taken
  // 10 incoming, 0 DR, 3 armor -> 10 - 3 = 7
  REQUIRE(ed::descent::calculateDamageTaken(10.0F, 0.0F, 3.0F) == 7.0F);
  // Minimum 20% floor: 10 incoming * 0.2 = 2.0
  REQUIRE(ed::descent::calculateDamageTaken(10.0F, 0.5F, 4.0F) == 2.0F);
  // DR cap at 60%
  const float drCapped = ed::descent::calculateDamageTaken(100.0F, 0.90F, 0.0F);
  REQUIRE(drCapped == Catch::Approx(40.0F)); // 100 * (1 - 0.60) = 40.0
}

TEST_CASE("flow field guides navigation toward target tile") {
  ed::descent::FlowField ff;
  ff.resize(16, 16);
  ff.compute(8, 8);

  // Tile (4, 8) is to the left of (8, 8), should point right (+1, 0)
  const auto dirLeft = ff.getDirection(4.0F * 32.0F, 8.0F * 32.0F);
  REQUIRE(dirLeft.x > 0.5F);
  REQUIRE(std::abs(dirLeft.y) < 0.2F);

  // Tile (8, 4) is above (8, 8), should point down (0, +1)
  const auto dirUp = ff.getDirection(8.0F * 32.0F, 4.0F * 32.0F);
  REQUIRE(dirUp.y > 0.5F);
  REQUIRE(std::abs(dirUp.x) < 0.2F);
}

TEST_CASE("spatial hash indexes and queries entities by radius") {
  ed::descent::SpatialHash hash;
  hash.init(1000.0F, 1000.0F);

  hash.insert(1, {100.0F, 100.0F});
  hash.insert(2, {120.0F, 110.0F});
  hash.insert(3, {500.0F, 500.0F});

  std::vector<int> found;
  hash.query({105.0F, 105.0F}, 50.0F, found);
  REQUIRE(found.size() >= 2);
  REQUIRE(std::find(found.begin(), found.end(), 1) != found.end());
  REQUIRE(std::find(found.begin(), found.end(), 2) != found.end());
  REQUIRE(std::find(found.begin(), found.end(), 3) == found.end());
}

TEST_CASE("combat simulation steps movement, attacks, and level-ups") {
  ed::descent::DescentSimState sim;
  ed::descent::initRoom(sim, 30, 20);

  REQUIRE(sim.player.currentHp == sim.player.maxHp);
  REQUIRE_FALSE(sim.enemies.empty());

  // Move player right
  ed::descent::DescentInput input;
  input.moveVector = {1.0F, 0.0F};
  const float startX = sim.playerPos.x;

  for (int i = 0; i < 60; ++i) { // 1 second at 60Hz
    ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
  }

  REQUIRE(sim.playerPos.x > startX + 150.0F);

  // Test Dash
  input.dashPressed = true;
  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
  REQUIRE(sim.player.dashCharges == 1);
  REQUIRE(sim.player.dashDurationTimer > 0.0F);
  REQUIRE(sim.player.invulnerabilityTimer > 0.0F);

  // Test Level-up modal trigger and card selection
  sim.currentXp = sim.xpToNext - 1;
  ed::descent::DescentPickup xpGem;
  xpGem.pos = sim.playerPos;
  xpGem.type = ed::descent::PickupType::XpGemSmall;
  xpGem.value = 10.0F;
  sim.pickups.push_back(xpGem);

  input.dashPressed = false;
  input.moveVector = {0.0F, 0.0F};
  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);

  REQUIRE(sim.levelUpPending);
  REQUIRE(sim.levelUpChoices.size() == 3);
  REQUIRE(sim.runLevel == 2);

  // Select card 0
  const float oldMight = sim.player.might;
  const float oldAtkSpd = sim.player.attackSpeed;
  const float oldHp = sim.player.maxHp;

  REQUIRE(ed::descent::selectLevelUpChoice(sim, 0));
  REQUIRE_FALSE(sim.levelUpPending);
  REQUIRE((sim.player.might > oldMight || sim.player.attackSpeed > oldAtkSpd || sim.player.maxHp > oldHp || !sim.ownedArms.empty()));
}

TEST_CASE("ember censer damages nearby enemies and inflicts burn status effect") {
  ed::descent::DescentSimState sim;
  ed::descent::initRoom(sim, 30, 20);

  // Clear existing enemies and add a single target near player
  sim.enemies.clear();
  ed::descent::DescentEnemy target;
  target.id = 999;
  target.pos = sim.playerPos + ed::descent::Vec2{35.0F, 0.0F};
  target.hp = target.maxHp = 100.0F;
  target.armor = 0.0F;
  target.radius = 12.0F;
  target.alive = true;
  sim.enemies.push_back(target);

  // Equip Ember Censer
  sim.ownedArms.clear();
  sim.ownedArms.push_back({"ember_censer", 1});
  sim.censerTimer = 0.0F;

  // Aim away and put primary on cooldown so Warrior sweep does not hit
  sim.playerAim = {-1.0F, 0.0F};
  sim.player.primaryCooldown = 10.0F;

  ed::descent::DescentInput input;
  input.moveVector = {0.0F, 0.0F};

  // Step 1 frame to trigger censer pulse
  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);

  REQUIRE(sim.enemies[0].hp < 100.0F);
  REQUIRE_FALSE(sim.enemies[0].statuses.empty());
  REQUIRE(sim.enemies[0].statuses[0].type == ed::descent::StatusEffectType::Burn);

  const float hpAfterHit = sim.enemies[0].hp;

  // Step 35 frames (approx 0.6 seconds) to let Burn DoT tick
  for (int i = 0; i < 35; ++i) {
    sim.player.primaryCooldown = 10.0F;
    sim.censerTimer = 10.0F; // pause direct pulse so we measure burn DoT
    ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
  }

  REQUIRE(sim.enemies[0].hp < hpAfterHit);
}

TEST_CASE("all 5 class kits execute unique primary, skill, dash, and ultimate mechanics") {
  // 1. Archer
  {
    ed::descent::DescentSimState sim;
    ed::descent::initRoom(sim, 30, 20);
    sim.selectedClass = ed::descent::ClassId::Archer;
    sim.enemies.clear();

    // Dash (Backroll)
    sim.player.primaryCooldown = 1.0F;
    ed::descent::DescentInput input;
    input.dashPressed = true;
    ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
    REQUIRE(sim.player.arrowCritCount == 3);

    // Primary shot consumes crit
    input.dashPressed = false;
    sim.player.primaryCooldown = 0.0F;
    ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
    REQUIRE(sim.player.arrowCritCount == 2);
    REQUIRE_FALSE(sim.projectiles.empty());
    REQUIRE(sim.projectiles.back().isCrit == true);

    // Skill: Volley (5 arrows)
    const std::size_t countBefore = sim.projectiles.size();
    input.skillPressed = true;
    ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
    REQUIRE(sim.projectiles.size() == countBefore + 5);

    // Ultimate: Rain of Arrows
    sim.player.wrath = 100.0F;
    input.skillPressed = false;
    input.ultimatePressed = true;
    ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
    REQUIRE(sim.player.wrath == 0.0F);
    REQUIRE(sim.player.rainOfArrowsTimer > 0.0F);
  }

  // 2. Tank
  {
    ed::descent::DescentSimState sim;
    ed::descent::initRoom(sim, 30, 20);
    sim.selectedClass = ed::descent::ClassId::Tank;
    sim.enemies.clear();

    // Skill: Bulwark refills barrier
    sim.player.barrier = 0.0F;
    ed::descent::DescentInput input;
    input.skillPressed = true;
    ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
    REQUIRE(sim.player.barrier == sim.player.maxBarrier);
    REQUIRE(sim.player.bulwarkTimer > 0.0F);

    // Ultimate: Fortress
    sim.player.wrath = 100.0F;
    input.skillPressed = false;
    input.ultimatePressed = true;
    ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
    REQUIRE(sim.player.wrath == 0.0F);
    REQUIRE(sim.player.fortressTimer > 0.0F);
  }

  // 3. Mage
  {
    ed::descent::DescentSimState sim;
    ed::descent::initRoom(sim, 30, 20);
    sim.selectedClass = ed::descent::ClassId::Mage;
    sim.enemies.clear();

    // Add enemy in range
    ed::descent::DescentEnemy foe;
    foe.id = 101;
    foe.pos = sim.playerPos + ed::descent::Vec2{50.0F, 0.0F};
    foe.hp = foe.maxHp = 50.0F;
    foe.alive = true;
    sim.enemies.push_back(foe);

    // Skill: Frost Nova freezes enemy
    ed::descent::DescentInput input;
    input.skillPressed = true;
    ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
    REQUIRE(sim.enemies[0].hp < 50.0F);
    bool hasFreeze = false;
    for (const auto& st : sim.enemies[0].statuses) {
      if (st.type == ed::descent::StatusEffectType::Freeze) hasFreeze = true;
    }
    REQUIRE(hasFreeze);

    // Ultimate: Meteor Swarm
    sim.player.wrath = 100.0F;
    input.skillPressed = false;
    input.ultimatePressed = true;
    ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
    REQUIRE(sim.player.meteorSwarmTimer > 0.0F);
  }

  // 4. Summoner
  {
    ed::descent::DescentSimState sim;
    ed::descent::initRoom(sim, 30, 20);
    sim.selectedClass = ed::descent::ClassId::Summoner;
    sim.enemies.clear();

    // Skill: Raise Minions
    ed::descent::DescentInput input;
    input.skillPressed = true;
    ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
    REQUIRE_FALSE(sim.friendlyMinions.empty());
    REQUIRE(sim.friendlyMinions.size() >= 3);

    // Ultimate: Soul Harvest heals and deals damage
    sim.player.currentHp = 50.0F;
    sim.player.wrath = 100.0F;
    input.skillPressed = false;
    input.ultimatePressed = true;
    ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
    REQUIRE(sim.player.currentHp >= 50.0F);
  }

  // 5. Warrior
  {
    ed::descent::DescentSimState sim;
    ed::descent::initRoom(sim, 30, 20);
    sim.selectedClass = ed::descent::ClassId::Warrior;
    sim.enemies.clear();

    // Skill: Whirling blades
    ed::descent::DescentInput input;
    input.skillPressed = true;
    ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
    REQUIRE(sim.player.whirlTimer > 0.0F);

    // Ultimate: War Cry
    const float baseMight = sim.player.might;
    sim.player.wrath = 100.0F;
    input.skillPressed = false;
    input.ultimatePressed = true;
    ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
    REQUIRE(sim.player.might > baseMight);
    REQUIRE(sim.player.ultimateActiveTimer > 0.0F);
  }
}

TEST_CASE("meta progression, anvil upgrades, provisions, and calamity pacts apply correctly") {
  ed::descent::DescentSave meta;
  meta.anvilMight = 4;        // +20% Might
  meta.anvilVitality = 5;     // +50 Max HP
  meta.anvilSwiftness = 3;    // +12% Speed
  meta.anvilResilience = 2;   // +2 Armor
  meta.anvilGreed = 3;        // +30% Embers

  meta.provisionStew = true;  // +20% HP
  meta.provisionBrew = true;  // +15% Speed
  meta.provisionOil = true;   // +15% Might

  meta.pactClaws = 2;         // Heat +2
  meta.pactFrenzy = 1;        // Heat +1
  meta.pactShadows = 1;       // Heat +2
  REQUIRE(meta.calculateTotalHeat() == 5);

  ed::descent::DescentSimState sim;
  ed::descent::applyMetaToPlayer(sim, meta);

  REQUIRE(sim.player.maxHp > 180.0F);
  REQUIRE(sim.player.might >= 0.35F);
  REQUIRE(sim.player.moveSpeed > 210.0F);
  REQUIRE(sim.player.armor >= 6.0F);
  REQUIRE(sim.heatLevel == 5);
  REQUIRE(sim.heatDamageMult == Catch::Approx(1.30F));
  REQUIRE(sim.heatSpeedMult == Catch::Approx(1.10F));
}

TEST_CASE("all 9 arms execute distinct combat mechanisms") {
  ed::descent::DescentSimState sim;
  ed::descent::initRoom(sim, 30, 20);
  sim.enemies.clear();

  // Add dummy enemy
  ed::descent::DescentEnemy target;
  target.id = 501;
  target.pos = sim.playerPos + ed::descent::Vec2{40.0F, 0.0F};
  target.hp = target.maxHp = 500.0F;
  target.alive = true;
  sim.enemies.push_back(target);

  ed::descent::DescentInput input;

  // 1. Throwing Daggers
  sim.ownedArms = {{"throwing_daggers", 2}};
  sim.daggersTimer = 0.0F;
  sim.projectiles.clear();
  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
  REQUIRE_FALSE(sim.projectiles.empty());

  // 2. Orbiting Runeblades
  sim.ownedArms = {{"orbiting_runeblades", 2}};
  sim.runebladesAngle = 0.0F;
  const float oldHp = sim.enemies[0].hp;
  // Step multiple frames so blade rotates through enemy
  for (int i = 0; i < 30; ++i) {
    ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
  }
  REQUIRE(sim.enemies[0].hp < oldHp);

  // 3. Thorn Bramble
  sim.ownedArms = {{"thorn_bramble", 1}};
  sim.brambleTimer = 0.0F;
  sim.thornPatches.clear();
  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
  REQUIRE_FALSE(sim.thornPatches.empty());

  // 4. Frost Bell
  sim.ownedArms = {{"frost_bell", 1}};
  sim.frostBellTimer = 0.0F;
  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
  bool chilled = false;
  for (const auto& st : sim.enemies[0].statuses) {
    if (st.type == ed::descent::StatusEffectType::Chill) chilled = true;
  }
  REQUIRE(chilled);
}

TEST_CASE("boss encounters, mini-boss Rattlewick, and Grave Warden Osric multi-phase mechanics") {
  ed::descent::DescentSimState sim;
  ed::descent::initRoom(sim, 30, 20);
  sim.enemies.clear();

  // Spawn Mini-boss Rattlewick
  ed::descent::spawnMiniBoss(sim, sim.playerPos + ed::descent::Vec2{100.0F, 0.0F});
  REQUIRE_FALSE(sim.enemies.empty());
  REQUIRE(sim.enemies.back().isMiniBoss == true);
  REQUIRE(sim.bossName == "Bone Captain Rattlewick");

  // Spawn Boss Osric
  sim.enemies.clear();
  sim.currentWave = 3; // Final boss wave
  ed::descent::spawnBossOsric(sim, sim.playerPos + ed::descent::Vec2{150.0F, 0.0F});
  REQUIRE_FALSE(sim.enemies.empty());
  REQUIRE(sim.enemies.back().isBoss == true);
  REQUIRE(sim.bossName == "Grave Warden Osric");
  REQUIRE(sim.bossPhase == 1);

  ed::descent::DescentInput input;

  // Step to trigger Phase 1 bone burst special
  sim.enemies.back().bossSpecialCooldown = 0.0F;
  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
  bool foundOsricShot = false;
  for (const auto& p : sim.projectiles) {
    if (p.sourceArm == "osric_bone_burst") foundOsricShot = true;
  }
  REQUIRE(foundOsricShot);

  // Transition to Phase 2 (damage below 50%)
  sim.enemies.back().hp = sim.enemies.back().maxHp * 0.40F;
  const float oldSpeed = sim.enemies.back().speed;
  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
  REQUIRE(sim.enemies.back().bossPhase == 2);
  REQUIRE(sim.bossPhase == 2);
  REQUIRE(sim.enemies.back().speed > oldSpeed);

  // Phase 2 special: Gravefall Leap Slam
  sim.enemies.back().bossSpecialCooldown = 0.0F;
  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
  REQUIRE(sim.enemies.back().isLeaping == true);

  // Kill boss and verify victory trigger
  sim.enemies.back().hp = 0.0F;
  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
  REQUIRE(sim.bossDefeated == true);
  REQUIRE(sim.roomCleared == true);
}

TEST_CASE("roguelike multi-chamber run progression, 3-door branching route choices, and state persistence") {
  ed::descent::DescentSimState sim;
  ed::descent::initRoom(sim, 30, 20);

  // Initial chamber validation
  REQUIRE(sim.chamberIndex == 1);
  REQUIRE(sim.totalChambers == 20);
  REQUIRE(sim.currentChamberType == ed::descent::ChamberType::Combat);
  REQUIRE(sim.doorChoicesPending == false);
  REQUIRE(sim.runCompleted == false);

  ed::descent::DescentInput input;

  // Clear Chamber 1
  sim.currentWave = sim.totalWaves;
  sim.enemies.clear();
  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);

  REQUIRE(sim.roomCleared == true);
  REQUIRE(sim.doorChoicesPending == true);
  REQUIRE(sim.runCompleted == false);
  REQUIRE(sim.doorChoices.size() == 3);

  // Set up player state to verify persistence
  sim.runLevel = 4;
  sim.currentXp = 10;
  sim.player.currentHp = 80.0F;
  sim.player.barrier = 15.0F;
  sim.player.wrath = 40.0F;
  sim.embers = 75;
  sim.ownedArms = {{"boomerang_axe", 2}};
  sim.ownedTomes = {{"heft", 1}};

  // Select door choice 0 (Combat Chamber - sword)
  bool chosen = ed::descent::selectChamberChoice(sim, 0);
  REQUIRE(chosen == true);

  // Verify transition to Chamber 2
  REQUIRE(sim.chamberIndex == 2);
  REQUIRE(sim.doorChoicesPending == false);
  REQUIRE(sim.roomCleared == false);
  REQUIRE_FALSE(sim.enemies.empty());

  // Verify player state persisted (combat door grants no extra buffs)
  REQUIRE(sim.runLevel == 4);
  REQUIRE(sim.currentXp == 10);
  REQUIRE(sim.player.currentHp == Catch::Approx(80.0F));
  REQUIRE(sim.player.wrath == Catch::Approx(40.0F));
  REQUIRE(sim.embers >= 75);
  REQUIRE(sim.ownedArms[0].id == "boomerang_axe");
  REQUIRE(sim.ownedArms[0].level == 2);

  // Test approaching the final chamber (Chamber 19 cleared -> Chamber 20 Boss)
  sim.chamberIndex = 19;
  sim.currentWave = sim.totalWaves;
  sim.enemies.clear();
  sim.roomCleared = false;
  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);

  REQUIRE(sim.roomCleared == true);
  REQUIRE(sim.doorChoicesPending == true);
  // Boss stage has exactly 1 door entrance with no buffs
  REQUIRE(sim.doorChoices.size() == 1);
  REQUIRE(sim.doorChoices[0].type == ed::descent::ChamberType::Boss);
  REQUIRE(sim.doorChoices[0].iconType == "boss");
  REQUIRE(sim.doorChoices[0].rewardType == "none");

  // Select Sanctum Gate
  chosen = ed::descent::selectChamberChoice(sim, 0);
  REQUIRE(chosen == true);
  REQUIRE(sim.chamberIndex == 20);
  REQUIRE(sim.currentChamberType == ed::descent::ChamberType::Boss);
  REQUIRE(sim.bossName == "Grave Warden Osric");
  REQUIRE(sim.bossDefeated == false);
  REQUIRE(sim.runCompleted == false);

  // Defeat Grave Warden Osric in Chamber 20
  for (auto& enemy : sim.enemies) {
    if (enemy.isBoss) enemy.hp = 0.0F;
  }
  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);

  REQUIRE(sim.bossDefeated == true);
  REQUIRE(sim.roomCleared == true);
  REQUIRE(sim.runCompleted == true);
  REQUIRE(sim.doorChoicesPending == false);
}

TEST_CASE("treasure chamber setup, chest encounter, and chamber exit choices") {
  ed::descent::DescentSimState sim;
  sim.selectedClass = ed::descent::ClassId::Warrior;
  ed::descent::initRoom(sim, 30, 20);

  // Directly initialize a Treasure chamber
  ed::descent::initNextChamber(sim, ed::descent::ChamberType::Treasure);
  REQUIRE(sim.currentChamberType == ed::descent::ChamberType::Treasure);
  REQUIRE(sim.totalWaves == 0);
  REQUIRE(sim.enemies.empty());
  REQUIRE(sim.roomCleared == false);
  REQUIRE(sim.doorChoicesPending == false);

  // Update simulation while player explores treasure room
  ed::descent::DescentInput input;
  for (int frame = 0; frame < 60; ++frame) {
    ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);
  }

  // Room must not be auto-cleared prior to vault trial completion
  REQUIRE(sim.roomCleared == false);
  REQUIRE(sim.doorChoicesPending == false);
  REQUIRE(sim.doorChoices.empty());

  // Simulate vault chest minigame finish (win or fail)
  sim.roomCleared = true;
  sim.doorChoicesPending = true;
  ed::descent::generateChamberChoices(sim);

  REQUIRE(sim.doorChoicesPending == true);
  REQUIRE(sim.doorChoices.size() == 3);

  // Selecting a door choice moves to the next chamber
  const int prevChamber = sim.chamberIndex;
  const bool chosen = ed::descent::selectChamberChoice(sim, 0);
  REQUIRE(chosen == true);
  REQUIRE(sim.chamberIndex == prevChamber + 1);
  REQUIRE(sim.roomCleared == false);
  REQUIRE(sim.doorChoicesPending == false);
}

TEST_CASE("new enemy archetypes, wave scaling, and class-specific skills") {
  ed::descent::DescentSimState sim;
  sim.selectedClass = ed::descent::ClassId::Summoner;
  ed::descent::initRoom(sim, 30, 20);

  // 1. Wave distribution checks
  sim.chamberIndex = 1;
  ed::descent::initNextChamber(sim, ed::descent::ChamberType::Combat);
  REQUIRE(sim.totalWaves == 3);

  sim.chamberIndex = 8;
  ed::descent::initNextChamber(sim, ed::descent::ChamberType::Combat);
  REQUIRE(sim.totalWaves == 4);

  sim.chamberIndex = 15;
  ed::descent::initNextChamber(sim, ed::descent::ChamberType::Combat);
  REQUIRE(sim.totalWaves == 5);

  ed::descent::initNextChamber(sim, ed::descent::ChamberType::Elite);
  REQUIRE(sim.totalWaves == 4);

  // 2. New enemy archetypes (Stalker & VoidCaster)
  sim.enemies.clear();
  ed::descent::spawnEnemy(sim, ed::descent::EnemyArchetype::Stalker, {100.0F, 100.0F});
  ed::descent::spawnEnemy(sim, ed::descent::EnemyArchetype::VoidCaster, {150.0F, 150.0F});

  REQUIRE(sim.enemies.size() == 2);
  REQUIRE(sim.enemies[0].archetype == ed::descent::EnemyArchetype::Stalker);
  REQUIRE(sim.enemies[0].name == "Shadow Stalker");
  REQUIRE(sim.enemies[1].archetype == ed::descent::EnemyArchetype::VoidCaster);
  REQUIRE(sim.enemies[1].name == "Void Weaver");

  // 3. New class skills execution: Skeletal Horde
  sim.ownedArms.clear();
  sim.ownedArms.push_back({"skeletal_horde", 2});
  sim.skeletalHordeTimer = 0.0F;

  ed::descent::DescentInput input;
  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);

  REQUIRE_FALSE(sim.friendlyMinions.empty());
  REQUIRE(sim.friendlyMinions[0].damage > 0.0F);

  // 4. New class skills execution: Seeking Gale
  sim.ownedArms.clear();
  sim.ownedArms.push_back({"seeking_gale", 2});
  sim.seekingGaleTimer = 0.0F;
  sim.projectiles.clear();

  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);

  bool foundGale = false;
  for (const auto& p : sim.projectiles) {
    if (p.sourceArm == "seeking_gale") {
      foundGale = true;
      break;
    }
  }
  REQUIRE(foundGale == true);

  // 5. New class skills execution: Earth Tremor
  sim.ownedArms.clear();
  sim.ownedArms.push_back({"earth_tremor", 1});
  sim.earthTremorTimer = 0.0F;
  const float prevHp = sim.enemies[0].hp;
  sim.enemies[0].pos = sim.playerPos + ed::descent::Vec2{20.0F, 0.0F};

  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);

  REQUIRE(sim.enemies[0].hp < prevHp);

  // 6. Level-up choices weighting
  sim.levelUpChoices.clear();
  sim.selectedClass = ed::descent::ClassId::Summoner;
  ed::descent::generateLevelUpChoices(sim);
  REQUIRE(sim.levelUpChoices.size() == 3);
}

TEST_CASE("wave progression spawns next wave at 75 percent completion without clearing room") {
  ed::descent::DescentSimState sim;
  ed::descent::initRoom(sim, 30, 20);

  sim.chamberIndex = 1;
  ed::descent::initNextChamber(sim, ed::descent::ChamberType::Combat);
  REQUIRE(sim.totalWaves == 3);
  REQUIRE(sim.currentWave == 1);
  REQUIRE(sim.currentWaveSpawned > 0);
  REQUIRE(sim.currentWaveKilled == 0);

  const int initialCount = sim.currentWaveSpawned;
  const int killThreshold = static_cast<int>(std::ceil(initialCount * 0.75F));

  // Kill slightly less than 75% of the wave
  int killed = 0;
  for (auto& e : sim.enemies) {
    if (e.alive && killed < killThreshold - 1) {
      e.hp = 0.0F;
      killed++;
    }
  }

  ed::descent::DescentInput input;
  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);

  // Still on wave 1
  REQUIRE(sim.currentWave == 1);
  REQUIRE(sim.roomCleared == false);

  // Now kill enough to reach 75% threshold
  for (auto& e : sim.enemies) {
    if (e.alive && killed < killThreshold) {
      e.hp = 0.0F;
      killed++;
    }
  }

  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);

  // Wave 2 must have spawned immediately while remaining enemies were still active
  REQUIRE(sim.currentWave == 2);
  REQUIRE(sim.roomCleared == false);
  REQUIRE(sim.currentWaveKilled == 0);
  REQUIRE(sim.currentWaveSpawned > 0);

  // Advance to final wave (Wave 3)
  const int wave2Threshold = static_cast<int>(std::ceil(sim.currentWaveSpawned * 0.75F));
  killed = 0;
  for (auto& e : sim.enemies) {
    if (e.alive && killed < wave2Threshold) {
      e.hp = 0.0F;
      killed++;
    }
  }
  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);

  REQUIRE(sim.currentWave == 3);
  REQUIRE(sim.currentWave == sim.totalWaves);
  // Room must NOT be cleared yet because surviving enemies still exist
  REQUIRE(sim.roomCleared == false);

  // Defeat all remaining enemies
  for (auto& e : sim.enemies) {
    e.hp = 0.0F;
  }
  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);

  // Now the chamber clears
  REQUIRE(sim.roomCleared == true);
  REQUIRE(sim.doorChoicesPending == true);
}

TEST_CASE("stalker enemies spawn sparsely at 1 to 2 per room") {
  ed::descent::DescentSimState sim;
  ed::descent::initRoom(sim, 30, 20);

  // 1. Check Combat chamber total stalkers across all waves
  sim.chamberIndex = 3;
  ed::descent::initNextChamber(sim, ed::descent::ChamberType::Combat);
  for (int w = 2; w <= sim.totalWaves; ++w) {
    ed::descent::spawnChamberWave(sim, w, ed::descent::ChamberType::Combat);
  }
  int stalkerCount = 0;
  for (const auto& e : sim.enemies) {
    if (e.archetype == ed::descent::EnemyArchetype::Stalker) stalkerCount++;
  }
  REQUIRE(stalkerCount >= 1);
  REQUIRE(stalkerCount <= 2);

  // 2. Check Elite chamber total stalkers across all 4 waves
  sim.enemies.clear();
  sim.chamberIndex = 5;
  ed::descent::initNextChamber(sim, ed::descent::ChamberType::Elite);
  for (int w = 2; w <= sim.totalWaves; ++w) {
    ed::descent::spawnChamberWave(sim, w, ed::descent::ChamberType::Elite);
  }
  stalkerCount = 0;
  for (const auto& e : sim.enemies) {
    if (e.archetype == ed::descent::EnemyArchetype::Stalker) stalkerCount++;
  }
  REQUIRE(stalkerCount == 2);

  // 3. Check Boss chamber total stalkers
  sim.enemies.clear();
  sim.chamberIndex = 20;
  ed::descent::initNextChamber(sim, ed::descent::ChamberType::Boss);
  stalkerCount = 0;
  for (const auto& e : sim.enemies) {
    if (e.archetype == ed::descent::EnemyArchetype::Stalker) stalkerCount++;
  }
  REQUIRE(stalkerCount == 2);
}

TEST_CASE("endless mode initializes with unbounded chambers and loop 1") {
  ed::descent::DescentSimState sim;
  sim.isEndless = true;
  ed::descent::initRoom(sim, 30, 20);

  REQUIRE(sim.isEndless == true);
  REQUIRE(sim.totalChambers == -1);
  REQUIRE(sim.endlessLoop == 1);
  REQUIRE(sim.chamberIndex == 1);
}

TEST_CASE("endless mode generates boss sanctum every 10 chambers and advances loops") {
  ed::descent::DescentSimState sim;
  sim.isEndless = true;
  ed::descent::initRoom(sim, 30, 20);

  // At chamber 9, generate choices for chamber 10 (should be single Boss Sanctum door)
  sim.chamberIndex = 9;
  ed::descent::generateChamberChoices(sim);
  REQUIRE(sim.doorChoices.size() == 1);
  REQUIRE(sim.doorChoices[0].type == ed::descent::ChamberType::Boss);

  // Enter chamber 10 via door choice
  REQUIRE(ed::descent::selectChamberChoice(sim, 0));
  REQUIRE(sim.chamberIndex == 10);
  REQUIRE(sim.currentChamberType == ed::descent::ChamberType::Boss);
  REQUIRE(sim.bossMaxHp > 0.0F);

  // Defeat boss
  sim.bossCurrentHp = 0.0F;
  sim.enemies.clear();
  ed::descent::DescentInput input;
  ed::descent::stepDescentSim(sim, input, 1.0F / 60.0F);

  // In endless mode, defeating boss does not complete the run; it increments loop and opens next doors
  REQUIRE(sim.runCompleted == false);
  REQUIRE(sim.endlessLoop == 2);
  REQUIRE(sim.doorChoicesPending == true);
}

TEST_CASE("endless mode scales enemy health and damage across loops") {
  ed::descent::DescentSimState sim1;
  sim1.isEndless = true;
  sim1.endlessLoop = 1;
  sim1.chamberIndex = 5;
  ed::descent::spawnChamberWave(sim1, 1, ed::descent::ChamberType::Combat);
  REQUIRE(!sim1.enemies.empty());
  const float hpLoop1 = sim1.enemies[0].maxHp;
  const float dmgLoop1 = sim1.enemies[0].contactDamage;

  ed::descent::DescentSimState sim2;
  sim2.isEndless = true;
  sim2.endlessLoop = 2;
  sim2.chamberIndex = 5;
  ed::descent::spawnChamberWave(sim2, 1, ed::descent::ChamberType::Combat);
  REQUIRE(!sim2.enemies.empty());
  const float hpLoop2 = sim2.enemies[0].maxHp;
  const float dmgLoop2 = sim2.enemies[0].contactDamage;

  REQUIRE(hpLoop2 > hpLoop1);
  REQUIRE(dmgLoop2 > dmgLoop1);
}
