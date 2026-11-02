#include "ui_screens.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "campaign_minigames.hpp"
#include "game_actions.hpp"
#include "ed/descent/combat_sim.hpp"
#include "ed/formulas.hpp"
#include "ed/postgame.hpp"

namespace ed::app {
void drawDescentHubScreen(AppState& app) {
  if (app.descentEndlessMode) {
    drawTextCentered("THE HOLLOW DESCENT - ENDLESS CATACOMBS", 45, 34, Color{248, 211, 89, 255});
    drawTextCentered("Survive infinite shifting depths of cursed horrors and scaling bosses", 88, 16, Color{190, 190, 205, 255});
  } else {
    drawTextCentered("THE HOLLOW DESCENT", 45, 34, Color{248, 211, 89, 255});
    drawTextCentered("Survive the cursed realm of Vaelor, the Crowned Hunger", 88, 16, Color{190, 190, 205, 255});
  }

  const Vector2 mouse = GetMousePosition();

  // LEFT PANEL: Actions & Tabs
  DrawRectangleRounded({60, 130, 320, 540}, 0.05F, 8, Color{36, 35, 48, 255});
  DrawRectangleRoundedLines({60, 130, 320, 540}, 0.05F, 8, Color{60, 58, 80, 255});

  // Begin Run Button
  const Rectangle beginBtn{80, 155, 280, 52};
  const bool hoverBegin = CheckCollisionPointRec(mouse, beginBtn);
  DrawRectangleRounded(beginBtn, 0.2F, 6, hoverBegin ? Color{220, 145, 30, 255} : Color{180, 100, 20, 255});
  DrawRectangleRoundedLines(beginBtn, 0.2F, 6, Color{255, 225, 120, 255});
  const char* beginTxt = app.descentEndlessMode ? "BEGIN ENDLESS RUN (ENTER)" : "BEGIN RUN (ENTER)";
  DrawText(beginTxt, 80 + (280 - MeasureText(beginTxt, 18)) / 2, 172, 18, WHITE);

  // Tab Buttons
  struct TabBtnInfo {
    DescentHubTab tab;
    const char* label;
  };
  const TabBtnInfo tabs[] = {
    {DescentHubTab::Main, "1. Class Overview"},
    {DescentHubTab::Anvil, "2. Ember Anvil (Upgrades)"},
    {DescentHubTab::Preparations, "3. Preparations & Sigils"},
    {DescentHubTab::Codex, "4. Descent Codex"},
    {DescentHubTab::Pacts, "5. Difficulty & Pacts"}
  };

  for (int i = 0; i < 5; ++i) {
    const Rectangle btn{80, static_cast<float>(222 + i * 50), 280, 42};
    const bool isSelected = (app.descentHubTab == tabs[i].tab);
    const bool hover = CheckCollisionPointRec(mouse, btn);
    Color bg = isSelected ? Color{65, 60, 85, 255} : (hover ? Color{52, 50, 68, 255} : Color{40, 38, 52, 255});
    Color border = isSelected ? Color{248, 211, 89, 255} : Color{75, 72, 95, 255};
    DrawRectangleRounded(btn, 0.2F, 4, bg);
    DrawRectangleRoundedLines(btn, 0.2F, 4, border);
    DrawText(tabs[i].label, 80 + (280 - MeasureText(tabs[i].label, 15)) / 2, 234 + i * 50, 15, isSelected ? Color{255, 225, 120, 255} : Color{220, 220, 235, 255});
  }

  // Back Button
  const Rectangle backBtn{80, 595, 280, 46};
  const bool hoverBack = CheckCollisionPointRec(mouse, backBtn);
  DrawRectangleRounded(backBtn, 0.2F, 4, hoverBack ? Color{70, 45, 55, 255} : Color{48, 36, 42, 255});
  DrawRectangleRoundedLines(backBtn, 0.2F, 4, Color{150, 80, 90, 255});
  const char* backTxt = "BACK TO CAMP (ESC)";
  DrawText(backTxt, 80 + (280 - MeasureText(backTxt, 15)) / 2, 610, 15, Color{240, 180, 190, 255});

  // CENTER PANEL
  DrawRectangleRounded({400, 130, 480, 540}, 0.05F, 8, Color{36, 35, 48, 255});
  DrawRectangleRoundedLines({400, 130, 480, 540}, 0.05F, 8, Color{60, 58, 80, 255});

  if (app.descentHubTab == DescentHubTab::Main) {
    // --- CLASS CAROUSEL ---
    const ed::descent::ClassId cls = app.descentMeta.selectedClass;

    // Carousel Arrow Buttons
    const Rectangle prevBtn{415, 150, 36, 36};
    const Rectangle nextBtn{830, 150, 36, 36};
    const bool hPrev = CheckCollisionPointRec(mouse, prevBtn);
    const bool hNext = CheckCollisionPointRec(mouse, nextBtn);
    DrawRectangleRounded(prevBtn, 0.2F, 4, hPrev ? Color{60, 55, 80, 255} : Color{45, 42, 60, 255});
    DrawRectangleRounded(nextBtn, 0.2F, 4, hNext ? Color{60, 55, 80, 255} : Color{45, 42, 60, 255});
    DrawText("<", 427, 158, 20, RAYWHITE);
    DrawText(">", 842, 158, 20, RAYWHITE);

    std::string className, classTags, pName, pDesc, sName, sDesc, dName, dDesc, uName, uDesc, armName, armDesc;
    Color badgeColor = Color{200, 60, 60, 255};

    if (cls == ed::descent::ClassId::Warrior) {
      className = "ASHBLADE (WARRIOR)";
      classTags = "[Melee]  [Area]  [Crit]";
      badgeColor = Color{210, 60, 60, 255};
      pName = "PRIMARY: Sweep"; pDesc = "Auto-swings a 140 deg arc dealing 14 base damage.";
      sName = "SKILL (Q): Whirling Blades"; sDesc = "Spins for 3.0s, high area melee damage to nearby foes.";
      dName = "DASH (SPACE): Charge Strike"; dDesc = "2 charges, 200px burst with invulnerability frames.";
      uName = "ULTIMATE (R): War Cry"; uDesc = "+30% Might, +25% Speed, heals 25% HP over 6 seconds.";
      armName = "STARTING ARM: Boomerang Axe"; armDesc = "Piercing axe flies out 220px and returns to you.";
    } else if (cls == ed::descent::ClassId::Tank) {
      className = "BASTION KNIGHT (TANK)";
      classTags = "[Defense]  [Melee]  [Area]";
      badgeColor = Color{70, 140, 230, 255};
      pName = "PRIMARY: Shield Slam"; pDesc = "Short frontal cone, 16 damage, grants 4 barrier.";
      sName = "SKILL (Q): Bulwark"; sDesc = "Refills barrier to max, absorbs and reflects 30% damage for 4s.";
      dName = "DASH (SPACE): Stun Rush"; dDesc = "Heavy charge that stuns enemies and absorbs blows.";
      uName = "ULTIMATE (R): Fortress"; uDesc = "Stationary ring: -50% damage taken, holy pulses for 22 dmg.";
      armName = "STARTING ARM: Ember Censer"; armDesc = "Continuous fire aura burning all adjacent enemies.";
    } else if (cls == ed::descent::ClassId::Archer) {
      className = "MOONFEATHER RANGER (ARCHER)";
      classTags = "[Projectile]  [Crit]  [Mobility]";
      badgeColor = Color{60, 200, 120, 255};
      pName = "PRIMARY: Aimed Arrow"; pDesc = "Fires high-velocity piercing arrow towards aim direction.";
      sName = "SKILL (Q): Volley"; sDesc = "Fires a fan of 5 piercing arrows in a 40-deg cone.";
      dName = "DASH (SPACE): Backroll"; dDesc = "Dashes away from aim; next 3 arrows guaranteed CRIT!";
      uName = "ULTIMATE (R): Rain of Arrows"; uDesc = "Barrages target area with falling arrows for 4 seconds.";
      armName = "STARTING ARM: Throwing Daggers"; armDesc = "Hurls rapid piercing daggers towards closest enemy.";
    } else if (cls == ed::descent::ClassId::Mage) {
      className = "RUNEWEAVER (MAGE)";
      classTags = "[Projectile]  [Area]  [Burst]";
      badgeColor = Color{170, 90, 240, 255};
      pName = "PRIMARY: Arcane Bolt"; pDesc = "Homing orb tracking enemies, exploding in 30px radius.";
      sName = "SKILL (Q): Frost Nova"; sDesc = "Instant 130px blast dealing 25 dmg and FREEZING for 2s.";
      dName = "DASH (SPACE): Blink"; dDesc = "Instant teleport 150px in movement direction.";
      uName = "ULTIMATE (R): Meteor Swarm"; uDesc = "Summons 6 devastating meteors impacting enemy clusters.";
      armName = "STARTING ARM: Lightning Rod"; armDesc = "Bounces electrical arcs across clustered enemies.";
    } else {
      className = "SOULSHURD (SUMMONER)";
      classTags = "[Summon]  [Area]  [Curse]";
      badgeColor = Color{60, 210, 190, 255};
      pName = "PRIMARY: Soul Scythe"; pDesc = "Crescent blade piercing foes and applying Hex (+12% dmg).";
      sName = "SKILL (Q): Raise Minions"; sDesc = "Summons 3 ghost wolves that pursue and maul enemies.";
      dName = "DASH (SPACE): Spectral Shift"; dDesc = "Phases forward leaving a trail of poisonous soul vapor.";
      uName = "ULTIMATE (R): Soul Harvest"; uDesc = "Detonates all nearby hexed foes for 50 dmg & heals you.";
      armName = "STARTING ARM: Thorn Bramble"; armDesc = "Plants damaging patches of creeping thorny brambles.";
    }

    const int nameW = MeasureText(className.c_str(), 18);
    DrawText(className.c_str(), 400 + (480 - nameW) / 2, 155, 18, Color{245, 190, 80, 255});
    const int tagW = MeasureText(classTags.c_str(), 13);
    DrawText(classTags.c_str(), 400 + (480 - tagW) / 2, 180, 13, Color{160, 210, 255, 255});

    // Visual Class Badge
    DrawCircle(640, 248, 38, Color{30, 28, 42, 255});
    DrawCircleLines(640, 248, 38, badgeColor);
    DrawCircle(640, 242, 16, Color{220, 175, 130, 255});
    DrawRectangle(626, 258, 28, 20, badgeColor);

    // Abilities Breakdown
    DrawText(pName.c_str(), 425, 305, 15, RAYWHITE);
    DrawText(pDesc.c_str(), 425, 326, 12, Color{175, 175, 190, 255});

    DrawText(sName.c_str(), 425, 355, 15, RAYWHITE);
    DrawText(sDesc.c_str(), 425, 376, 12, Color{175, 175, 190, 255});

    DrawText(dName.c_str(), 425, 405, 15, RAYWHITE);
    DrawText(dDesc.c_str(), 425, 426, 12, Color{175, 175, 190, 255});

    DrawText(uName.c_str(), 425, 455, 15, RAYWHITE);
    DrawText(uDesc.c_str(), 425, 476, 12, Color{175, 175, 190, 255});

    DrawText(armName.c_str(), 425, 510, 15, Color{255, 215, 120, 255});
    DrawText(armDesc.c_str(), 425, 531, 12, Color{175, 175, 190, 255});

    // Select Class Button
    const Rectangle selBtn{520, 605, 240, 42};
    const bool hSel = CheckCollisionPointRec(mouse, selBtn);
    DrawRectangleRounded(selBtn, 0.2F, 4, hSel ? Color{55, 130, 75, 255} : Color{40, 95, 55, 255});
    DrawRectangleRoundedLines(selBtn, 0.2F, 4, Color{120, 240, 140, 255});
    DrawText("READY FOR EXPEDITION", 520 + (240 - MeasureText("READY FOR EXPEDITION", 14)) / 2, 618, 14, WHITE);

  } else if (app.descentHubTab == DescentHubTab::Anvil) {
    // --- EMBER ANVIL UPGRADES ---
    DrawText("EMBER ANVIL - SOUL FORGE", 425, 150, 18, Color{248, 211, 89, 255});
    std::string embTxt = "Available Embers: " + std::to_string(app.descentMeta.embers);
    DrawText(embTxt.c_str(), 425, 175, 14, Color{255, 215, 60, 255});

    struct AnvilCard {
      const char* name;
      const char* desc;
      int rank;
      int maxRank;
      int cost;
    };
    const AnvilCard upgrades[] = {
      {"Might", "+5% Dmg Deals", app.descentMeta.anvilMight, 10, 20 + app.descentMeta.anvilMight * 15},
      {"Vitality", "+10 Max HP", app.descentMeta.anvilVitality, 10, 20 + app.descentMeta.anvilVitality * 15},
      {"Swiftness", "+4% Move Speed", app.descentMeta.anvilSwiftness, 5, 25 + app.descentMeta.anvilSwiftness * 20},
      {"Magnetism", "+15% Pickup Area", app.descentMeta.anvilMagnetism, 5, 15 + app.descentMeta.anvilMagnetism * 10},
      {"Resilience", "+1 Armor", app.descentMeta.anvilResilience, 5, 30 + app.descentMeta.anvilResilience * 25},
      {"Greed", "+10% Embers Drop", app.descentMeta.anvilGreed, 5, 25 + app.descentMeta.anvilGreed * 20},
      {"Rerolls", "+1 Choice Reroll", app.descentMeta.anvilRerolls, 3, 50 + app.descentMeta.anvilRerolls * 50},
      {"Banishes", "+1 Card Banish", app.descentMeta.anvilBanishes, 3, 50 + app.descentMeta.anvilBanishes * 50}
    };

    for (int i = 0; i < 8; ++i) {
      const float x = static_cast<float>(415 + (i % 2) * 225);
      const float y = static_cast<float>(205 + (i / 2) * 88);
      const Rectangle cRec{x, y, 215, 78};
      DrawRectangleRounded(cRec, 0.15F, 4, Color{42, 40, 56, 255});
      DrawRectangleRoundedLines(cRec, 0.15F, 4, Color{70, 68, 92, 255});

      DrawText(upgrades[i].name, static_cast<int>(x + 10), static_cast<int>(y + 8), 14, RAYWHITE);
      DrawText(upgrades[i].desc, static_cast<int>(x + 10), static_cast<int>(y + 26), 11, Color{180, 180, 195, 255});

      std::string rTxt = "Rank: " + std::to_string(upgrades[i].rank) + "/" + std::to_string(upgrades[i].maxRank);
      DrawText(rTxt.c_str(), static_cast<int>(x + 10), static_cast<int>(y + 45), 11, Color{220, 210, 140, 255});

      const Rectangle bBtn{x + 120, y + 42, 85, 26};
      const bool canAfford = (app.descentMeta.embers >= upgrades[i].cost && upgrades[i].rank < upgrades[i].maxRank);
      const bool hBuy = CheckCollisionPointRec(mouse, bBtn);
      Color btnBg = (upgrades[i].rank >= upgrades[i].maxRank) ? Color{50, 50, 60, 255} : (canAfford ? (hBuy ? Color{55, 120, 80, 255} : Color{40, 90, 60, 255}) : Color{50, 45, 55, 255});
      DrawRectangleRounded(bBtn, 0.2F, 3, btnBg);
      std::string bTxt = (upgrades[i].rank >= upgrades[i].maxRank) ? "MAX" : (std::to_string(upgrades[i].cost) + " E");
      DrawText(bTxt.c_str(), static_cast<int>(bBtn.x + (85 - MeasureText(bTxt.c_str(), 11)) / 2), static_cast<int>(bBtn.y + 7), 11, WHITE);
    }

    // Respec All Button
    const Rectangle respecBtn{500, 580, 280, 36};
    const bool hRespec = CheckCollisionPointRec(mouse, respecBtn);
    DrawRectangleRounded(respecBtn, 0.2F, 4, hRespec ? Color{110, 45, 45, 255} : Color{80, 35, 35, 255});
    DrawRectangleRoundedLines(respecBtn, 0.2F, 4, Color{200, 70, 70, 255});
    const char* rTxt = "RESPEC ALL (Refunds 100% Embers)";
    DrawText(rTxt, 500 + (280 - MeasureText(rTxt, 12)) / 2, 592, 12, Color{255, 200, 200, 255});

  } else if (app.descentHubTab == DescentHubTab::Preparations) {
    // --- PREPARATIONS & SIGILS ---
    DrawText("EXPEDITION PREPARATIONS", 425, 150, 18, Color{248, 211, 89, 255});
    DrawText("Consume camp provisions & bind gem sigil resonance", 425, 175, 13, Color{190, 190, 205, 255});

    // Provisions Section
    DrawText("CAMP PROVISIONS", 425, 210, 15, Color{255, 215, 100, 255});

    struct ProvCard {
      const char* name;
      const char* bonus;
      bool active;
    };
    const ProvCard provs[] = {
      {"Hearty Cinder Stew", "+20% Max Health for run", app.descentMeta.provisionStew},
      {"Swift-Foot Brew", "+15% Movement Speed for run", app.descentMeta.provisionBrew},
      {"Blazing Weapon Oil", "+15% Might (Damage) for run", app.descentMeta.provisionOil}
    };

    for (int i = 0; i < 3; ++i) {
      const Rectangle pRec{425, static_cast<float>(238 + i * 54), 430, 46};
      DrawRectangleRounded(pRec, 0.15F, 4, provs[i].active ? Color{35, 55, 45, 255} : Color{40, 38, 52, 255});
      DrawRectangleRoundedLines(pRec, 0.15F, 4, provs[i].active ? Color{100, 210, 130, 255} : Color{65, 62, 85, 255});
      DrawText(provs[i].name, 440, 246 + i * 54, 14, WHITE);
      DrawText(provs[i].bonus, 440, 264 + i * 54, 11, Color{180, 180, 195, 255});

      DrawText(provs[i].active ? "[ON]" : "[OFF]", 805, 253 + i * 54, 13, provs[i].active ? Color{120, 240, 150, 255} : Color{150, 150, 165, 255});
    }

    // Sigils Section
    DrawText("EXPEDITION SIGILS (GEM RESONANCE)", 425, 420, 15, Color{255, 215, 100, 255});
    const char* sigilNames[] = {"Ruby: +10% Might", "Sapphire: +8% CDR", "Emerald: +2 Regen", "Diamond: +3 Armor"};
    for (int i = 0; i < 4; ++i) {
      const float x = static_cast<float>(425 + (i % 2) * 220);
      const float y = static_cast<float>(450 + (i / 2) * 60);
      DrawRectangleRounded({x, y, 210, 48}, 0.15F, 4, Color{36, 42, 56, 255});
      DrawRectangleRoundedLines({x, y, 210, 48}, 0.15F, 4, Color{80, 120, 180, 255});
      DrawText(sigilNames[i], static_cast<int>(x + 12), static_cast<int>(y + 16), 13, Color{190, 220, 255, 255});
    }

  } else if (app.descentHubTab == DescentHubTab::Codex) {
    // --- BESTIARY CODEX ---
    DrawText("DESCENT CODEX - ARCHIVE OF FOES", 425, 150, 18, Color{248, 211, 89, 255});
    DrawText("Known archetypes lurking within the Hollow Realm", 425, 175, 13, Color{190, 190, 205, 255});

    struct CodexFoe {
      const char* name;
      const char* desc;
    };
    const CodexFoe foes[] = {
      {"Bone Rat (Swarmer)", "Fast vermin that hunt in vast packs. Low health, high speed."},
      {"Skeleton Thrall (Grunt)", "Standard animated footman marching straight towards prey."},
      {"Bone Archer (Shooter)", "Maintains distance and looses piercing bone arrows."},
      {"Ghoul (Charger)", "Winds up and surges forward at lethal velocity."},
      {"Crypt Guard (Brute)", "Heavy armored sentinel executing telegraphed ground slams."},
      {"Bone Golem (Splitter)", "Colossal animated bone construct; splits into rats upon death."},
      {"Volatile Corpse (Exploder)", "Rushes targets and detonates in a deadly fiery blast."},
      {"Necrolyte (Support)", "Channels necromantic auras healing nearby wounded comrades."},
      {"Bone Hive (Spawner)", "Stationary nest periodically hatching swarms of bone rats."},
      {"Grave Warden Osric (Boss)", "Ancient guardian wielding bone sweeps and Gravefall leaps."}
    };

    for (int i = 0; i < 10; ++i) {
      const float y = static_cast<float>(202 + i * 44);
      DrawRectangleRounded({420, y, 440, 38}, 0.1F, 3, Color{32, 30, 44, 255});
      DrawText(foes[i].name, 432, static_cast<int>(y + 4), 13, Color{245, 200, 100, 255});
      DrawText(foes[i].desc, 432, static_cast<int>(y + 20), 10, Color{175, 175, 190, 255});
    }

  } else if (app.descentHubTab == DescentHubTab::Pacts) {
    // --- DIFFICULTY & PACTS ---
    DrawText("PACTS OF CALAMITY", 425, 150, 18, Color{248, 211, 89, 255});
    const int totalHeat = app.descentMeta.calculateTotalHeat();
    std::string hTxt = "Active Heat: " + std::to_string(totalHeat) + "  (+ " + std::to_string(totalHeat * 10) + "% Ember & Soul Rewards)";
    DrawText(hTxt.c_str(), 425, 175, 14, Color{255, 130, 80, 255});

    struct PactInfo {
      const char* name;
      const char* effect;
      int heatPer;
      int rank;
      int maxRank;
    };
    const PactInfo pacts[] = {
      {"Pact of Claws", "+15% Enemy Damage dealt", 1, app.descentMeta.pactClaws, 3},
      {"Pact of Frenzy", "+10% Enemy Movement Speed", 1, app.descentMeta.pactFrenzy, 3},
      {"Pact of Flesh", "+20% Enemy Maximum Health", 1, app.descentMeta.pactFlesh, 3},
      {"Pact of Shadows", "+25% Elite Spawn Rate", 2, app.descentMeta.pactShadows, 3},
      {"Pact of Numbers", "+25% Wave Horde Density", 2, app.descentMeta.pactNumbers, 3},
      {"Pact of Wounds", "-30% Healing from all sources", 1, app.descentMeta.pactWounds, 1}
    };

    for (int i = 0; i < 6; ++i) {
      const float y = static_cast<float>(210 + i * 65);
      const Rectangle pRec{425, y, 430, 56};
      DrawRectangleRounded(pRec, 0.15F, 4, Color{40, 36, 50, 255});
      DrawRectangleRoundedLines(pRec, 0.15F, 4, Color{75, 65, 88, 255});

      DrawText(pacts[i].name, 440, static_cast<int>(y + 8), 14, RAYWHITE);
      DrawText(pacts[i].effect, 440, static_cast<int>(y + 28), 11, Color{180, 180, 195, 255});

      // Minus Button
      const Rectangle minBtn{750, y + 14, 28, 28};
      DrawRectangleRounded(minBtn, 0.2F, 3, Color{55, 45, 60, 255});
      DrawText("-", 760, static_cast<int>(y + 18), 16, RAYWHITE);

      std::string rTxt = std::to_string(pacts[i].rank) + "/" + std::to_string(pacts[i].maxRank);
      DrawText(rTxt.c_str(), 790, static_cast<int>(y + 20), 13, Color{255, 215, 80, 255});

      // Plus Button
      const Rectangle plusBtn{820, y + 14, 28, 28};
      DrawRectangleRounded(plusBtn, 0.2F, 3, Color{55, 45, 60, 255});
      DrawText("+", 829, static_cast<int>(y + 18), 16, RAYWHITE);
    }
  }

  // RIGHT PANEL: Shards & Records
  DrawRectangleRounded({900, 130, 320, 540}, 0.05F, 8, Color{36, 35, 48, 255});
  DrawRectangleRoundedLines({900, 130, 320, 540}, 0.05F, 8, Color{60, 58, 80, 255});

  DrawText("CURRENCIES", 925, 148, 16, Color{248, 211, 89, 255});
  DrawText(("Embers: " + std::to_string(app.descentMeta.embers)).c_str(), 925, 170, 14, Color{255, 215, 60, 255});
  DrawText(("Soul Fragments: " + std::to_string(app.descentMeta.soulFragments)).c_str(), 925, 192, 14, Color{200, 150, 255, 255});
  DrawText(("Crown Shards: " + std::to_string(app.descentMeta.crownShards)).c_str(), 925, 214, 14, Color{120, 230, 250, 255});

  DrawLine(925, 238, 1195, 238, Color{60, 58, 80, 255});

  DrawText("DESCENT RECORDS", 925, 248, 16, Color{248, 211, 89, 255});
  DrawText(("Standard Best: Chamber " + std::to_string(app.game.player.descentBestChamber)).c_str(), 925, 270, 13, RAYWHITE);
  DrawText(("Endless Best: Chamber " + std::to_string(app.descentMeta.bestEndlessChamber)).c_str(), 925, 290, 13, Color{255, 215, 120, 255});
  DrawText(("Best Heat Cleared: " + std::to_string(app.descentMeta.bestHeat)).c_str(), 925, 310, 13, RAYWHITE);
  DrawText(("Victories: " + std::to_string(app.descentMeta.victories)).c_str(), 925, 330, 13, RAYWHITE);
  DrawText(("Lifetime Clears: " + std::to_string(app.descentMeta.lifetimeRoomsCleared)).c_str(), 925, 350, 13, RAYWHITE);
  DrawText(("Lifetime Kills: " + std::to_string(app.descentMeta.lifetimeKills)).c_str(), 925, 370, 13, RAYWHITE);

  DrawLine(925, 394, 1195, 394, Color{60, 58, 80, 255});
  DrawText("EXPEDITION MODES", 925, 406, 16, Color{248, 211, 89, 255});

  // Standard Descent Card
  const Rectangle stdCard{915, 430, 290, 106};
  const bool hStd = CheckCollisionPointRec(mouse, stdCard);
  const bool stdActive = !app.descentEndlessMode;
  DrawRectangleRounded(stdCard, 0.12F, 4, stdActive ? Color{32, 52, 42, 255} : (hStd ? Color{48, 46, 62, 255} : Color{38, 36, 50, 255}));
  DrawRectangleRoundedLines(stdCard, 0.12F, 4, stdActive ? Color{110, 230, 140, 255} : Color{70, 68, 90, 255});
  DrawText(stdActive ? "[ACTIVE] Standard Descent" : "Standard Descent", 930, 440, 14, stdActive ? Color{120, 240, 150, 255} : RAYWHITE);
  DrawText("20-40 Progressive Chambers.", 930, 462, 11, Color{180, 180, 195, 255});
  DrawText("Vanquish Osric for Crown Shards.", 930, 480, 11, Color{180, 180, 195, 255});
  DrawText("Option to continue into Endless loop.", 930, 498, 11, Color{255, 215, 120, 255});
  DrawText(stdActive ? "Selected" : "Click to select", 930, 516, 10, stdActive ? Color{120, 240, 150, 255} : Color{140, 140, 160, 255});

  // Endless Catacombs Card
  const Rectangle endlessCard{915, 545, 290, 112};
  const bool hEndless = CheckCollisionPointRec(mouse, endlessCard);
  const bool endlessActive = app.descentEndlessMode;
  DrawRectangleRounded(endlessCard, 0.12F, 4, endlessActive ? Color{52, 38, 62, 255} : (hEndless ? Color{48, 46, 62, 255} : Color{38, 36, 50, 255}));
  DrawRectangleRoundedLines(endlessCard, 0.12F, 4, endlessActive ? Color{210, 140, 255, 255} : Color{70, 68, 90, 255});
  DrawText(endlessActive ? "[ACTIVE] Endless Catacombs" : "Endless Catacombs", 930, 555, 14, endlessActive ? Color{220, 160, 255, 255} : RAYWHITE);
  DrawText("No chamber cap. Infinite waves.", 930, 577, 11, Color{180, 180, 195, 255});
  DrawText("Boss every 10 chambers with loop scaling.", 930, 595, 11, Color{180, 180, 195, 255});
  DrawText("Retire anytime to secure your spoils!", 930, 613, 11, Color{255, 215, 120, 255});
  DrawText(endlessActive ? "Selected" : "Click to select", 930, 633, 10, endlessActive ? Color{210, 140, 255, 255} : Color{140, 140, 160, 255});
}

void drawDescentRunScreen(AppState& app) {
  auto& sim = app.descentSim;
  const float worldW = static_cast<float>(sim.roomWidthTiles) * ed::descent::DescentSimState::kTileSize;
  const float worldH = static_cast<float>(sim.roomHeightTiles) * ed::descent::DescentSimState::kTileSize;

  // Setup camera
  app.descentCamera.target = {sim.playerPos.x, sim.playerPos.y};
  app.descentCamera.offset = {640.0F, 360.0F};
  app.descentCamera.rotation = 0.0F;
  app.descentCamera.zoom = 1.0F;

  if (sim.screenShake > 0.0F) {
    const float s = sim.screenShake * 10.0F;
    app.descentCamera.offset.x += (-s + static_cast<float>(rand() % 100) / 50.0F * s);
    app.descentCamera.offset.y += (-s + static_cast<float>(rand() % 100) / 50.0F * s);
  }

  BeginMode2D(app.descentCamera);

  // Background Arena Floor
  DrawRectangle(0, 0, static_cast<int>(worldW), static_cast<int>(worldH), Color{22, 20, 30, 255});

  // Floor grid pattern
  for (int x = 0; x < sim.roomWidthTiles; ++x) {
    for (int y = 0; y < sim.roomHeightTiles; ++y) {
      if ((x + y) % 2 == 0) {
        DrawRectangle(x * 32, y * 32, 32, 32, Color{27, 25, 36, 255});
      }
    }
  }

  // Room Walls
  DrawRectangleLinesEx({0, 0, worldW, worldH}, 32.0F, Color{45, 42, 58, 255});
  DrawRectangleLinesEx({32.0F, 32.0F, worldW - 64.0F, worldH - 64.0F}, 2.0F, Color{70, 66, 90, 255});

  // Thorn Patches
  for (const auto& tp : sim.thornPatches) {
    DrawCircle(static_cast<int>(tp.pos.x), static_cast<int>(tp.pos.y), tp.radius, Color{45, 80, 40, 90});
    DrawCircleLines(static_cast<int>(tp.pos.x), static_cast<int>(tp.pos.y), tp.radius, Color{90, 160, 80, 180});
  }

  // Pickups
  for (const auto& p : sim.pickups) {
    if (!p.alive) continue;
    if (p.type == ed::descent::PickupType::XpGemSmall) {
      DrawPoly({p.pos.x, p.pos.y}, 4, 6.0F, 45.0F, Color{80, 200, 255, 255});
    } else if (p.type == ed::descent::PickupType::XpGemLarge) {
      DrawPoly({p.pos.x, p.pos.y}, 4, 10.0F, 45.0F, Color{200, 120, 255, 255});
      DrawPolyLines({p.pos.x, p.pos.y}, 4, 12.0F, 45.0F, Color{235, 180, 255, 200});
    } else if (p.type == ed::descent::PickupType::Ember) {
      DrawCircle(static_cast<int>(p.pos.x), static_cast<int>(p.pos.y), 5.0F, Color{255, 215, 0, 255});
    }
  }

  // Telegraphs
  for (const auto& tg : sim.telegraphs) {
    DrawCircle(static_cast<int>(tg.pos.x), static_cast<int>(tg.pos.y), tg.radius, Color{220, 50, 50, 70});
    DrawCircleLines(static_cast<int>(tg.pos.x), static_cast<int>(tg.pos.y), tg.radius, Color{240, 80, 80, 200});
  }

  // Friendly Minions
  for (const auto& m : sim.friendlyMinions) {
    DrawCircle(static_cast<int>(m.pos.x), static_cast<int>(m.pos.y), m.radius, Color{80, 220, 220, 180});
    DrawCircleLines(static_cast<int>(m.pos.x), static_cast<int>(m.pos.y), m.radius + 2.0F, Color{150, 250, 250, 220});
  }

  // Slash Arc (Warrior Sweep)
  if (sim.lastSlashArcTimer > 0.0F) {
    const float alpha = sim.lastSlashArcTimer / 0.15F;
    DrawCircleSector({sim.playerPos.x, sim.playerPos.y}, 74.0F,
                     (sim.lastSlashArcAngle * 57.2958F) - 70.0F,
                     (sim.lastSlashArcAngle * 57.2958F) + 70.0F,
                     16, Fade(Color{255, 240, 180, 255}, alpha * 0.45F));
    DrawCircleSectorLines({sim.playerPos.x, sim.playerPos.y}, 74.0F,
                          (sim.lastSlashArcAngle * 57.2958F) - 70.0F,
                          (sim.lastSlashArcAngle * 57.2958F) + 70.0F,
                          16, Fade(Color{255, 215, 90, 255}, alpha));
  }

  // Orbiting Runeblades
  for (const auto& arm : sim.ownedArms) {
    if (arm.id == "orbiting_runeblades") {
      const int bladeCount = 2 + arm.level;
      const float r = 65.0F * (1.0F + sim.player.area);
      for (int i = 0; i < bladeCount; ++i) {
        const float angle = sim.runebladesAngle + (static_cast<float>(i) / static_cast<float>(bladeCount)) * 6.283185F;
        const auto bPos = sim.playerPos + ed::descent::Vec2{std::cos(angle), std::sin(angle)} * r;
        DrawCircle(static_cast<int>(bPos.x), static_cast<int>(bPos.y), 7.0F, Color{100, 220, 255, 255});
        DrawCircleLines(static_cast<int>(bPos.x), static_cast<int>(bPos.y), 9.0F, Color{200, 245, 255, 200});
      }
    }
  }

  // Projectiles
  for (const auto& p : sim.projectiles) {
    Color col = p.fromPlayer ? Color{255, 230, 100, 255} : Color{240, 80, 80, 255};
    if (p.sourceArm == "throwing_daggers") col = Color{200, 220, 255, 255};
    else if (p.sourceArm == "arcane_bolt") col = Color{180, 90, 255, 255};
    else if (p.sourceArm == "arcane_orb") col = Color{220, 110, 255, 255};
    else if (p.sourceArm == "seeking_gale") col = Color{120, 240, 170, 255};
    else if (p.sourceArm == "void_orb") col = Color{195, 60, 245, 255};
    else if (p.sourceArm == "soul_scythe") col = Color{60, 240, 200, 255};
    DrawCircle(static_cast<int>(p.pos.x), static_cast<int>(p.pos.y), p.radius, col);
    DrawCircleLines(static_cast<int>(p.pos.x), static_cast<int>(p.pos.y), p.radius + 1.0F, WHITE);
  }

  // Enemies
  for (const auto& e : sim.enemies) {
    if (!e.alive) continue;
    Color eColor = Color{190, 185, 175, 255};
    if (e.archetype == ed::descent::EnemyArchetype::Swarmer) eColor = Color{175, 75, 75, 255};
    else if (e.archetype == ed::descent::EnemyArchetype::Shooter) eColor = Color{150, 190, 140, 255};
    else if (e.archetype == ed::descent::EnemyArchetype::Charger) eColor = Color{210, 120, 50, 255};
    else if (e.archetype == ed::descent::EnemyArchetype::Brute) eColor = Color{130, 120, 150, 255};
    else if (e.archetype == ed::descent::EnemyArchetype::Exploder) eColor = Color{230, 80, 70, 255};
    else if (e.archetype == ed::descent::EnemyArchetype::Support) eColor = Color{90, 180, 220, 255};
    else if (e.archetype == ed::descent::EnemyArchetype::Spawner) eColor = Color{160, 140, 100, 255};
    else if (e.archetype == ed::descent::EnemyArchetype::Stalker) eColor = Color{145, 65, 210, 255};
    else if (e.archetype == ed::descent::EnemyArchetype::VoidCaster) eColor = Color{45, 175, 185, 255};

    if (e.isBoss) eColor = (e.bossPhase == 2) ? Color{240, 50, 50, 255} : Color{180, 60, 70, 255};
    else if (e.isMiniBoss) eColor = Color{220, 110, 40, 255};

    if (e.flashTimer > 0.0F) {
      eColor = WHITE;
    }

    // Elite aura
    if (e.isElite) {
      DrawCircleLines(static_cast<int>(e.pos.x), static_cast<int>(e.pos.y), e.radius + 6.0F, Color{255, 215, 60, 200});
    }

    DrawCircle(static_cast<int>(e.pos.x), static_cast<int>(e.pos.y), e.radius, eColor);
    DrawCircleLines(static_cast<int>(e.pos.x), static_cast<int>(e.pos.y), e.radius, Color{40, 35, 45, 255});

    // Burning, Chilled, or Frozen status visuals
    for (const auto& st : e.statuses) {
      if (st.type == ed::descent::StatusEffectType::Burn) {
        DrawCircleLines(static_cast<int>(e.pos.x), static_cast<int>(e.pos.y), e.radius + 3.0F, Color{255, 110, 30, 200});
        DrawCircle(static_cast<int>(e.pos.x), static_cast<int>(e.pos.y), e.radius + 1.0F, Color{255, 80, 20, 60});
        break;
      } else if (st.type == ed::descent::StatusEffectType::Chill) {
        DrawCircleLines(static_cast<int>(e.pos.x), static_cast<int>(e.pos.y), e.radius + 3.0F, Color{100, 210, 255, 200});
        DrawCircle(static_cast<int>(e.pos.x), static_cast<int>(e.pos.y), e.radius + 1.0F, Color{120, 220, 255, 50});
        break;
      } else if (st.type == ed::descent::StatusEffectType::Freeze) {
        DrawCircleLines(static_cast<int>(e.pos.x), static_cast<int>(e.pos.y), e.radius + 4.0F, Color{180, 240, 255, 255});
        DrawCircle(static_cast<int>(e.pos.x), static_cast<int>(e.pos.y), e.radius + 2.0F, Color{140, 230, 255, 100});
        break;
      }
    }

    // Health bar above enemy
    if (e.hp < e.maxHp && !e.isBoss) {
      const float hpRatio = std::clamp(e.hp / e.maxHp, 0.0F, 1.0F);
      const int barW = static_cast<int>(e.radius * 2.0F);
      DrawRectangle(static_cast<int>(e.pos.x - e.radius), static_cast<int>(e.pos.y - e.radius - 8), barW, 4, Color{30, 25, 30, 200});
      DrawRectangle(static_cast<int>(e.pos.x - e.radius), static_cast<int>(e.pos.y - e.radius - 8), static_cast<int>(barW * hpRatio), 4, Color{220, 60, 60, 255});
    }
  }

  // Frost Bell / Frost Nova Shockwaves
  for (const auto& ring : sim.frostRings) {
    const float alpha = std::clamp(ring.timer / ring.maxTimer, 0.0F, 1.0F);
    DrawCircleLines(static_cast<int>(ring.pos.x), static_cast<int>(ring.pos.y), ring.radius, Fade(Color{120, 220, 255, 255}, alpha));
    DrawCircleLines(static_cast<int>(ring.pos.x), static_cast<int>(ring.pos.y), std::max(0.0F, ring.radius - 3.0F), Fade(Color{210, 250, 255, 255}, alpha * 0.8F));
    DrawCircle(static_cast<int>(ring.pos.x), static_cast<int>(ring.pos.y), ring.radius, Fade(Color{100, 205, 255, 255}, alpha * 0.15F));
    for (int k = 0; k < 8; ++k) {
      const float angle = static_cast<float>(k) * 0.785398F;
      const float cx = ring.pos.x + std::cos(angle) * ring.radius;
      const float cy = ring.pos.y + std::sin(angle) * ring.radius;
      DrawCircle(static_cast<int>(cx), static_cast<int>(cy), 3.0F, Fade(Color{220, 250, 255, 255}, alpha));
    }
  }

  // Chain Lightning Arcs
  for (const auto& arc : sim.lightningArcs) {
    const float alpha = std::clamp(arc.timer / arc.maxTimer, 0.0F, 1.0F);
    const ed::descent::Vec2 mid = (arc.from + arc.to) * 0.5F;
    const ed::descent::Vec2 diff = arc.to - arc.from;
    const float len = std::sqrt(diff.lengthSq());
    const ed::descent::Vec2 perp = (len > 0.001F) ? (ed::descent::Vec2{-diff.y, diff.x} / len) * 12.0F : ed::descent::Vec2{0.0F, 0.0F};

    DrawLineEx({arc.from.x, arc.from.y}, {mid.x + perp.x, mid.y + perp.y}, 3.5F, Fade(Color{70, 200, 255, 255}, alpha));
    DrawLineEx({mid.x + perp.x, mid.y + perp.y}, {arc.to.x, arc.to.y}, 3.5F, Fade(Color{70, 200, 255, 255}, alpha));

    DrawLineEx({arc.from.x, arc.from.y}, {mid.x + perp.x, mid.y + perp.y}, 1.5F, Fade(WHITE, alpha));
    DrawLineEx({mid.x + perp.x, mid.y + perp.y}, {arc.to.x, arc.to.y}, 1.5F, Fade(WHITE, alpha));

    DrawCircle(static_cast<int>(arc.from.x), static_cast<int>(arc.from.y), 4.5F, Fade(Color{180, 240, 255, 255}, alpha));
    DrawCircle(static_cast<int>(arc.to.x), static_cast<int>(arc.to.y), 5.5F, Fade(Color{180, 240, 255, 255}, alpha));
  }

  // In-Room Physical Chamber Doors (when room is cleared)
  if (sim.roomCleared && !sim.runCompleted && sim.doorChoicesPending) {
    const float time = static_cast<float>(GetTime());
    for (std::size_t i = 0; i < sim.doorChoices.size(); ++i) {
      const auto& door = sim.doorChoices[i];
      const float dx = door.doorPos.x;
      const float dy = door.doorPos.y;
      const float dw = door.doorWidth;
      const float dh = door.doorHeight;

      const float pDist = ed::descent::Vec2::distance(sim.playerPos, door.doorPos);
      const bool nearPlayer = (pDist <= 90.0F);

      // Stone archway frame
      DrawRectangleRounded({dx - dw * 0.5F - 6.0F, dy - dh * 0.5F - 6.0F, dw + 12.0F, dh + 12.0F}, 0.25F, 4, Color{32, 30, 44, 255});
      DrawRectangleRoundedLines({dx - dw * 0.5F - 6.0F, dy - dh * 0.5F - 6.0F, dw + 12.0F, dh + 12.0F}, 0.25F, 4,
                                nearPlayer ? Color{255, 230, 120, 255} : Color{85, 80, 110, 255});

      // Dark portal opening
      DrawRectangleRounded({dx - dw * 0.5F, dy - dh * 0.5F, dw, dh}, 0.2F, 4, Color{12, 10, 18, 255});

      // Pulsing energy inside doorway
      const float pulse = (std::sin(time * 3.5F + static_cast<float>(i) * 1.5F) + 1.0F) * 0.5F;
      Color portalGlow = (door.iconType == "heal") ? Color{60, 200, 100, 120} :
                         (door.iconType == "levelup") ? Color{240, 200, 50, 120} :
                         (door.iconType == "skull") ? Color{220, 50, 70, 140} :
                         (door.iconType == "boss") ? Color{180, 40, 220, 160} :
                         (door.iconType == "chest") ? Color{255, 215, 60, 150} : Color{100, 150, 240, 120};
      DrawRectangleRounded({dx - dw * 0.5F + 4.0F, dy - dh * 0.5F + 4.0F, dw - 8.0F, dh - 8.0F}, 0.2F, 4, Fade(portalGlow, 0.25F + pulse * 0.25F));

      // Overhead Signboard / Icon Plaque above the door
      const Rectangle plaqueRec{dx - 24.0F, dy - dh * 0.5F - 36.0F, 48.0F, 30.0F};
      DrawRectangleRounded(plaqueRec, 0.3F, 4, Color{24, 22, 34, 240});
      DrawRectangleRoundedLines(plaqueRec, 0.3F, 4, nearPlayer ? Color{255, 225, 90, 255} : Color{110, 105, 135, 255});

      // Draw minimal iconic glyph on the plaque
      if (door.iconType == "sword") {
        // Sword icon for combat stage
        DrawLineEx({dx - 7.0F, dy - dh * 0.5F - 13.0F}, {dx + 7.0F, dy - dh * 0.5F - 29.0F}, 2.5F, Color{220, 230, 255, 255});
        DrawLineEx({dx - 4.0F, dy - dh * 0.5F - 21.0F}, {dx + 1.0F, dy - dh * 0.5F - 16.0F}, 2.0F, Color{245, 190, 70, 255});
        DrawLineEx({dx + 7.0F, dy - dh * 0.5F - 13.0F}, {dx - 7.0F, dy - dh * 0.5F - 29.0F}, 2.5F, Color{220, 230, 255, 255});
        DrawLineEx({dx + 4.0F, dy - dh * 0.5F - 21.0F}, {dx - 1.0F, dy - dh * 0.5F - 16.0F}, 2.0F, Color{245, 190, 70, 255});
      } else if (door.iconType == "heal") {
        // Green plus for healing
        const Color plusGreen = Color{70, 235, 110, 255};
        DrawRectangle(static_cast<int>(dx - 9), static_cast<int>(dy - dh * 0.5F - 23), 18, 5, plusGreen);
        DrawRectangle(static_cast<int>(dx - 2.5F), static_cast<int>(dy - dh * 0.5F - 30), 5, 19, plusGreen);
      } else if (door.iconType == "levelup") {
        // Arrow pointing up for level up
        const Color arrowCol = Color{255, 215, 60, 255};
        DrawRectangle(static_cast<int>(dx - 2.5F), static_cast<int>(dy - dh * 0.5F - 21), 5, 11, arrowCol);
        DrawTriangle({dx, dy - dh * 0.5F - 32.0F},
                     {dx - 9.0F, dy - dh * 0.5F - 21.0F},
                     {dx + 9.0F, dy - dh * 0.5F - 21.0F}, arrowCol);
      } else if (door.iconType == "skull" || door.iconType == "boss") {
        // Skull for elite / boss stage
        const Color skullCol = (door.iconType == "boss") ? Color{255, 70, 100, 255} : Color{240, 100, 120, 255};
        DrawCircle(static_cast<int>(dx), static_cast<int>(dy - dh * 0.5F - 23), 8.0F, skullCol);
        DrawRectangle(static_cast<int>(dx - 5), static_cast<int>(dy - dh * 0.5F - 18), 10, 6, skullCol);
        DrawCircle(static_cast<int>(dx - 3), static_cast<int>(dy - dh * 0.5F - 24), 2.0F, Color{20, 16, 26, 255});
        DrawCircle(static_cast<int>(dx + 3), static_cast<int>(dy - dh * 0.5F - 24), 2.0F, Color{20, 16, 26, 255});
      } else if (door.iconType == "chest") {
        // Treasure chest icon
        const Color goldCol = Color{255, 215, 60, 255};
        DrawRectangleRounded({dx - 10.0F, dy - dh * 0.5F - 26.0F, 20.0F, 16.0F}, 0.2F, 3, goldCol);
        DrawRectangle(static_cast<int>(dx - 10.0F), static_cast<int>(dy - dh * 0.5F - 21.0F), 20, 3, Color{50, 35, 20, 255});
        DrawCircle(static_cast<int>(dx), static_cast<int>(dy - dh * 0.5F - 17.0F), 2.0F, Color{255, 240, 180, 255});
      }

      if (nearPlayer) {
        DrawText("WALK IN", static_cast<int>(dx - 24), static_cast<int>(dy + dh * 0.5F + 6), 11, Color{255, 240, 160, 255});
      }
    }
  }

  // Treasure Room Vault Dais and Chest
  if (sim.currentChamberType == ed::descent::ChamberType::Treasure) {
    const float cx = worldW * 0.5F;
    const float cy = worldH * 0.45F;

    // Golden runic dais
    DrawCircle(static_cast<int>(cx), static_cast<int>(cy), 55.0F, Color{36, 32, 50, 255});
    DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), 55.0F, Color{255, 215, 60, 160});
    DrawCircleLines(static_cast<int>(cx), static_cast<int>(cy), 48.0F, Color{200, 160, 40, 100});

    // Vault chest
    if (!app.descentChestOpened) {
      // Golden locked chest
      DrawRectangleRounded({cx - 26.0F, cy - 20.0F, 52.0F, 40.0F}, 0.25F, 4, Color{218, 165, 32, 255});
      DrawRectangleRoundedLines({cx - 26.0F, cy - 20.0F, 52.0F, 40.0F}, 0.25F, 4, Color{255, 240, 140, 255});
      DrawRectangle(static_cast<int>(cx - 26.0F), static_cast<int>(cy - 4.0F), 52, 6, Color{120, 85, 25, 255});
      DrawCircle(static_cast<int>(cx), static_cast<int>(cy + 6.0F), 4.5F, Color{45, 30, 15, 255});

      const float dist = ed::descent::Vec2::distance(sim.playerPos, ed::descent::Vec2{cx, cy});
      if (dist <= 65.0F && !app.descentMinigameActive) {
        const Rectangle promptRec{cx - 85.0F, cy - 52.0F, 170.0F, 24.0F};
        DrawRectangleRounded(promptRec, 0.3F, 4, Color{20, 18, 30, 230});
        DrawRectangleRoundedLines(promptRec, 0.3F, 4, Color{255, 215, 60, 255});
        DrawText("[E] UNLOCK VAULT CHEST", static_cast<int>(cx - 78.0F), static_cast<int>(cy - 47.0F), 11, Color{255, 245, 170, 255});
      }
    } else {
      // Opened chest
      DrawRectangleRounded({cx - 26.0F, cy - 10.0F, 52.0F, 30.0F}, 0.2F, 4, Color{150, 110, 30, 255});
      DrawRectangleRoundedLines({cx - 26.0F, cy - 10.0F, 52.0F, 30.0F}, 0.2F, 4, Color{190, 150, 50, 255});
      DrawRectangleRounded({cx - 28.0F, cy - 28.0F, 56.0F, 18.0F}, 0.25F, 4, Color{180, 135, 30, 255});
      DrawRectangleRoundedLines({cx - 28.0F, cy - 28.0F, 56.0F, 18.0F}, 0.25F, 4, Color{230, 185, 80, 255});
    }
  }

  // Ember Censer Fire Aura
  for (const auto& arm : sim.ownedArms) {
    if (arm.id == "ember_censer") {
      const float auraRadius = (60.0F + static_cast<float>(arm.level - 1) * 14.0F) * (1.0F + sim.player.area);
      const float pulse = std::sin(static_cast<float>(GetTime()) * 6.0F) * 3.0F;
      const float r = auraRadius + pulse;
      DrawCircle(static_cast<int>(sim.playerPos.x), static_cast<int>(sim.playerPos.y), r, Color{255, 90, 20, 35});
      DrawCircleLines(static_cast<int>(sim.playerPos.x), static_cast<int>(sim.playerPos.y), r, Color{255, 140, 30, 160});
      DrawCircleLines(static_cast<int>(sim.playerPos.x), static_cast<int>(sim.playerPos.y), r - 3.0F, Color{255, 210, 60, 90});
      break;
    }
  }

  // Player
  Color pColor = (sim.player.invulnerabilityTimer > 0.0F) ? Color{255, 230, 140, 255} : Color{200, 55, 55, 255};
  if (sim.player.barrier > 0.0F) {
    DrawCircleLines(static_cast<int>(sim.playerPos.x), static_cast<int>(sim.playerPos.y), 19.0F, Color{100, 200, 255, 200});
  }
  DrawCircle(static_cast<int>(sim.playerPos.x), static_cast<int>(sim.playerPos.y), 15.0F, pColor);
  DrawCircleLines(static_cast<int>(sim.playerPos.x), static_cast<int>(sim.playerPos.y), 15.0F, Color{255, 240, 200, 255});
  // Aim reticle
  DrawLine(static_cast<int>(sim.playerPos.x), static_cast<int>(sim.playerPos.y),
           static_cast<int>(sim.playerPos.x + sim.playerAim.x * 24.0F),
           static_cast<int>(sim.playerPos.y + sim.playerAim.y * 24.0F), Color{255, 220, 100, 200});

  // Damage Numbers
  for (const auto& ev : sim.damageEvents) {
    Color col = ev.toPlayer ? Color{255, 90, 90, 255} : (ev.isCrit ? Color{255, 220, 60, 255} : Color{240, 240, 240, 255});
    const float alpha = std::clamp(ev.lifetime / 0.6F, 0.0F, 1.0F);
    std::string text = std::to_string(static_cast<int>(ev.damage)) + (ev.isCrit ? "!" : "");
    DrawText(text.c_str(), static_cast<int>(ev.pos.x - 10), static_cast<int>(ev.pos.y - 20), ev.isCrit ? 18 : 14, Fade(col, alpha));
  }

  EndMode2D();

  // --- IN-RUN HUD (§15.1) ---

  // Top XP Bar
  DrawRectangle(0, 0, kWidth, 22, Color{20, 18, 28, 230});
  const float xpRatio = sim.xpToNext > 0 ? std::clamp(static_cast<float>(sim.currentXp) / static_cast<float>(sim.xpToNext), 0.0F, 1.0F) : 0.0F;
  DrawRectangle(0, 0, static_cast<int>(kWidth * xpRatio), 22, Color{70, 170, 255, 255});
  DrawRectangleLines(0, 0, kWidth, 22, Color{45, 110, 180, 255});
  std::string xpText = "LEVEL " + std::to_string(sim.runLevel) + "   [" + std::to_string(sim.currentXp) + " / " + std::to_string(sim.xpToNext) + " XP]";
  drawTextCentered(xpText, 4, 14, WHITE);

  // Top-Left HP Bar
  DrawRectangleRounded({20, 32, 220, 38}, 0.2F, 4, Color{28, 26, 36, 220});
  DrawRectangleRoundedLines({20, 32, 220, 38}, 0.2F, 4, Color{60, 56, 75, 255});
  const float hpRatio = std::clamp(sim.player.currentHp / sim.player.maxHp, 0.0F, 1.0F);
  DrawRectangle(30, 40, static_cast<int>(140.0F * hpRatio), 22, Color{210, 60, 60, 255});
  DrawRectangleLines(30, 40, 140, 22, Color{120, 35, 35, 255});
  std::string hpTxt = std::to_string(static_cast<int>(sim.player.currentHp)) + "/" + std::to_string(static_cast<int>(sim.player.maxHp));
  DrawText(hpTxt.c_str(), 175, 43, 14, WHITE);

  // Chamber & Wave Indicator
  std::string chTypeStr = "COMBAT";
  Color chCol = Color{248, 160, 60, 255};
  if (sim.currentChamberType == ed::descent::ChamberType::Elite) {
    chTypeStr = "ELITE";
    chCol = Color{240, 70, 70, 255};
  } else if (sim.currentChamberType == ed::descent::ChamberType::Treasure) {
    chTypeStr = "TREASURE";
    chCol = Color{255, 215, 60, 255};
  } else if (sim.currentChamberType == ed::descent::ChamberType::Shrine) {
    chTypeStr = "SHRINE";
    chCol = Color{90, 230, 140, 255};
  } else if (sim.currentChamberType == ed::descent::ChamberType::Shop) {
    chTypeStr = "SHOP";
    chCol = Color{90, 200, 240, 255};
  } else if (sim.currentChamberType == ed::descent::ChamberType::Boss || (!sim.isEndless && sim.chamberIndex == sim.totalChambers)) {
    chTypeStr = "BOSS SANCTUM";
    chCol = Color{220, 70, 255, 255};
  }
  std::string chStr = sim.isEndless
      ? ("ENDLESS CHAMBER " + std::to_string(sim.chamberIndex) + " (LOOP " + std::to_string(sim.endlessLoop) + ")  [" + chTypeStr + "]")
      : ("CHAMBER " + std::to_string(sim.chamberIndex) + " / " + std::to_string(sim.totalChambers) + "  [" + chTypeStr + "]");
  DrawText(chStr.c_str(), 20, 76, 15, chCol);
  DrawText(("WAVE " + std::to_string(sim.currentWave) + " / " + std::to_string(sim.totalWaves)).c_str(), 20, 96, 13, Color{200, 200, 215, 255});

  // Boss Health Bar (if active)
  if (!sim.bossName.empty() && !sim.bossDefeated && sim.bossMaxHp > 0.0F) {
    DrawRectangleRounded({340, 30, 600, 44}, 0.15F, 4, Color{24, 20, 30, 230});
    DrawRectangleRoundedLines({340, 30, 600, 44}, 0.15F, 4, Color{248, 211, 89, 255});
    const float bRatio = std::clamp(sim.bossCurrentHp / sim.bossMaxHp, 0.0F, 1.0F);
    Color barCol = (sim.bossPhase == 2) ? Color{235, 40, 40, 255} : Color{195, 50, 50, 255};
    DrawRectangle(350, 52, static_cast<int>(580.0F * bRatio), 16, barCol);
    DrawRectangleLines(350, 52, 580, 16, Color{100, 30, 30, 255});
    std::string bTitle = sim.bossName + " (" + (sim.bossPhase == 2 ? "PHASE 2 - ENRAGED" : "PHASE 1") + ")";
    drawTextCentered(bTitle, 34, 14, Color{255, 235, 140, 255});
  }

  // Top-Right Embers & Souls
  DrawRectangleRounded({1040, 32, 220, 64}, 0.2F, 4, Color{28, 26, 36, 220});
  DrawRectangleRoundedLines({1040, 32, 220, 64}, 0.2F, 4, Color{60, 56, 75, 255});
  DrawText(("Embers: " + std::to_string(sim.embers)).c_str(), 1055, 42, 16, Color{255, 215, 60, 255});
  DrawText(("Kills: " + std::to_string(sim.enemiesKilled)).c_str(), 1055, 68, 14, Color{190, 190, 205, 255});

  // Bottom Center: Dash, Skill (Q), Wrath (R)
  DrawRectangleRounded({440, 645, 400, 65}, 0.2F, 4, Color{28, 26, 36, 220});
  DrawRectangleRoundedLines({440, 645, 400, 65}, 0.2F, 4, Color{60, 56, 75, 255});

  // Dash
  DrawText("DASH (SPACE)", 455, 653, 11, Color{180, 180, 195, 255});
  for (int i = 0; i < sim.player.maxDashCharges; ++i) {
    Color dCol = (i < sim.player.dashCharges) ? Color{100, 220, 255, 255} : Color{50, 65, 80, 255};
    DrawCircle(470 + i * 22, 683, 7, dCol);
  }

  // Skill (Q)
  DrawText("SKILL (Q)", 555, 653, 11, Color{180, 180, 195, 255});
  const bool skillReady = (sim.player.skillCooldown <= 0.0F);
  DrawRectangleRounded({555, 668, 70, 26}, 0.2F, 3, skillReady ? Color{60, 130, 200, 255} : Color{45, 45, 55, 255});
  std::string skTxt = skillReady ? "READY" : (std::to_string(static_cast<int>(std::ceil(sim.player.skillCooldown))) + "s");
  DrawText(skTxt.c_str(), 562, 674, 12, WHITE);

  // Wrath Meter (R)
  DrawText("WRATH (R)", 665, 653, 11, Color{245, 180, 80, 255});
  DrawRectangle(665, 672, 155, 20, Color{45, 35, 30, 255});
  const float wrathRatio = std::clamp(sim.player.wrath / 100.0F, 0.0F, 1.0F);
  DrawRectangle(665, 672, static_cast<int>(155.0F * wrathRatio), 20, (sim.player.wrath >= 100.0F) ? Color{255, 215, 60, 255} : Color{210, 110, 40, 255});
  DrawRectangleLines(665, 672, 155, 20, Color{140, 80, 40, 255});
  if (sim.player.wrath >= 100.0F) {
    DrawText("ULTIMATE READY!", 685, 675, 12, BLACK);
  }

  // --- OVERLAYS ---

  // Chamber cleared in-world notification
  if (sim.roomCleared && !sim.runCompleted && sim.doorChoicesPending) {
    DrawRectangleRounded({390, 34, 500, 30}, 0.3F, 4, Color{18, 16, 26, 210});
    DrawRectangleRoundedLines({390, 34, 500, 30}, 0.3F, 4, Color{255, 215, 80, 200});
    std::string clrMsg = (sim.isEndless ? "ENDLESS CHAMBER " : "CHAMBER ") + std::to_string(sim.chamberIndex) + " CLEARED - Step into an open doorway";
    drawTextCentered(clrMsg, 41, 13, Color{255, 235, 140, 255});
  }

  // Run Summary / Clear Modal (on Victory or Defeat)
  if (sim.runCompleted || sim.player.currentHp <= 0.0F) {
    DrawRectangle(0, 0, kWidth, kHeight, Color{10, 10, 16, 235});
    const bool victory = (sim.runCompleted && sim.bossDefeated && !sim.isEndless);
    if (sim.isEndless) {
      drawTextCentered("ENDLESS CATACOMBS EXPEDITION CONCLUDED", 150, 30, Color{220, 160, 255, 255});
      std::string endSub = "You survived until Chamber " + std::to_string(sim.chamberIndex) + " (Loop " + std::to_string(sim.endlessLoop) + ").";
      drawTextCentered(endSub, 195, 16, Color{200, 190, 220, 255});
    } else if (victory) {
      drawTextCentered("DESCENT CONQUERED - VICTORY ACHIEVED!", 150, 32, Color{255, 215, 70, 255});
      drawTextCentered("You vanquished Grave Warden Osric and conquered the Hollow!", 195, 16, Color{160, 245, 170, 255});
    } else {
      drawTextCentered("YOU WERE CLAIMED BY THE HOLLOW", 150, 32, Color{235, 60, 60, 255});
      drawTextCentered("Your journey ended in Chamber " + std::to_string(sim.chamberIndex) + ".", 195, 16, Color{220, 180, 190, 255});
    }

    // Run Stats Card
    DrawRectangleRounded({420, 235, 440, 260}, 0.1F, 4, Color{30, 28, 42, 255});
    DrawRectangleRoundedLines({420, 235, 440, 260}, 0.1F, 4, Color{70, 66, 95, 255});

    DrawText(("Time Survived: " + std::to_string(static_cast<int>(sim.runTimeSeconds)) + "s").c_str(), 450, 255, 15, RAYWHITE);
    std::string chProg = sim.isEndless
        ? ("Chambers Cleared: " + std::to_string(sim.chamberIndex) + " (Loop " + std::to_string(sim.endlessLoop) + ")")
        : ("Chambers Conquered: " + std::to_string(victory ? sim.totalChambers : sim.chamberIndex - 1) + " / " + std::to_string(sim.totalChambers));
    DrawText(chProg.c_str(), 450, 285, 15, RAYWHITE);
    DrawText(("Run Level Reached: " + std::to_string(sim.runLevel)).c_str(), 450, 315, 15, RAYWHITE);
    DrawText(("Foes Slain: " + std::to_string(sim.enemiesKilled)).c_str(), 450, 345, 15, RAYWHITE);
    DrawText(("Damage Dealt: " + std::to_string(static_cast<int>(sim.totalDamageDealt))).c_str(), 450, 375, 15, RAYWHITE);
    DrawText(("Embers Gathered: +" + std::to_string(sim.embers)).c_str(), 450, 405, 16, Color{255, 215, 60, 255});
    const int soulsBonus = sim.isEndless ? std::max(1, sim.chamberIndex / 5) : (victory ? 3 : 1);
    DrawText(("Soul Fragments: +" + std::to_string(soulsBonus)).c_str(), 450, 435, 16, Color{200, 150, 255, 255});
    if (victory || (sim.isEndless && sim.endlessLoop > 1)) {
      std::string crw = "Crown Shards: +" + std::to_string(sim.isEndless ? sim.endlessLoop - 1 : 1);
      DrawText(crw.c_str(), 450, 465, 16, Color{100, 220, 255, 255});
    }

    const Vector2 mouse = GetMousePosition();
    if (victory) {
      // Dual Buttons: Claim Victory (Left) vs Continue into Endless (Right)
      const Rectangle retBtn{350, 520, 280, 48};
      const bool hRet = CheckCollisionPointRec(mouse, retBtn);
      DrawRectangleRounded(retBtn, 0.2F, 4, hRet ? Color{65, 125, 80, 255} : Color{45, 95, 60, 255});
      DrawRectangleRoundedLines(retBtn, 0.2F, 4, Color{120, 230, 140, 255});
      const char* retTxt = "CLAIM VICTORY & RETURN (ENTER)";
      DrawText(retTxt, static_cast<int>(retBtn.x + (280 - MeasureText(retTxt, 13)) / 2), 536, 13, WHITE);

      const Rectangle contBtn{650, 520, 280, 48};
      const bool hCont = CheckCollisionPointRec(mouse, contBtn);
      DrawRectangleRounded(contBtn, 0.2F, 4, hCont ? Color{110, 55, 145, 255} : Color{80, 40, 110, 255});
      DrawRectangleRoundedLines(contBtn, 0.2F, 4, Color{210, 140, 255, 255});
      const char* contTxt = "ENTER ENDLESS LOOP (SPACE)";
      DrawText(contTxt, static_cast<int>(contBtn.x + (280 - MeasureText(contTxt, 13)) / 2), 536, 13, Color{245, 215, 255, 255});
    } else {
      // Single Return Button
      const Rectangle retBtn{460, 520, 360, 48};
      const bool hRet = CheckCollisionPointRec(mouse, retBtn);
      DrawRectangleRounded(retBtn, 0.2F, 4, hRet ? Color{65, 125, 80, 255} : Color{45, 95, 60, 255});
      DrawRectangleRoundedLines(retBtn, 0.2F, 4, Color{120, 230, 140, 255});
      std::string retTxt = sim.isEndless ? "RETURN WITH SPOILS (ENTER)" : "RETURN TO DESCENT HUB (ENTER)";
      DrawText(retTxt.c_str(), static_cast<int>(retBtn.x + (360 - MeasureText(retTxt.c_str(), 14)) / 2), 535, 14, WHITE);
    }
  }

  // Level Up Modal
  if (sim.levelUpPending) {
    DrawRectangle(0, 0, kWidth, kHeight, Color{10, 10, 18, 230});
    drawTextCentered("LEVEL UP!", 110, 36, Color{255, 220, 80, 255});
    drawTextCentered("Choose an Arm or Tome upgrade", 160, 16, Color{190, 190, 210, 255});

    const Vector2 mouse = GetMousePosition();

    for (std::size_t i = 0; i < sim.levelUpChoices.size(); ++i) {
      const auto& card = sim.levelUpChoices[i];
      const Rectangle cardRec{static_cast<float>(265 + i * 260), 210, 230, 310};
      const bool hover = CheckCollisionPointRec(mouse, cardRec);

      DrawRectangleRounded(cardRec, 0.12F, 6, hover ? Color{45, 42, 65, 255} : Color{32, 30, 46, 255});
      DrawRectangleRoundedLines(cardRec, 0.12F, 6, hover ? Color{255, 220, 100, 255} : Color{70, 68, 95, 255});

      // Type Badge
      Color typeCol = (card.type == "Arm") ? Color{245, 160, 60, 255} : Color{100, 190, 255, 255};
      DrawText(card.type.c_str(), static_cast<int>(cardRec.x + 20), static_cast<int>(cardRec.y + 20), 13, typeCol);

      // Card Title
      DrawText(card.name.c_str(), static_cast<int>(cardRec.x + 20), static_cast<int>(cardRec.y + 44), 16, RAYWHITE);

      // Level preview
      std::string lvlStr = "Rank " + std::to_string(card.currentLevel + 1) + " / " + std::to_string(card.maxLevel);
      DrawText(lvlStr.c_str(), static_cast<int>(cardRec.x + 20), static_cast<int>(cardRec.y + 70), 13, Color{220, 210, 140, 255});

      // Description
      DrawText(card.description.c_str(), static_cast<int>(cardRec.x + 20), static_cast<int>(cardRec.y + 110), 13, Color{180, 180, 195, 255});

      // Hotkey button
      std::string hotkey = "[" + std::to_string(i + 1) + "] Select";
      DrawRectangleRounded({cardRec.x + 20, cardRec.y + 245, 190, 42}, 0.2F, 4, hover ? Color{70, 130, 180, 255} : Color{50, 48, 68, 255});
      DrawText(hotkey.c_str(), static_cast<int>(cardRec.x + 55), static_cast<int>(cardRec.y + 258), 14, WHITE);
    }

    // Modal Action buttons
    const Rectangle rerollBtn{380, 550, 140, 40};
    DrawRectangleRounded(rerollBtn, 0.2F, 4, Color{45, 42, 60, 255});
    DrawRectangleRoundedLines(rerollBtn, 0.2F, 4, Color{80, 75, 105, 255});
    std::string rTxt = "[R] Reroll (" + std::to_string(sim.player.rerolls) + ")";
    DrawText(rTxt.c_str(), 395, 562, 13, Color{200, 200, 220, 255});

    const Rectangle skipBtn{560, 550, 140, 40};
    DrawRectangleRounded(skipBtn, 0.2F, 4, Color{45, 42, 60, 255});
    DrawRectangleRoundedLines(skipBtn, 0.2F, 4, Color{80, 75, 105, 255});
    DrawText("[S] Skip (+10% HP)", 570, 562, 13, Color{140, 240, 170, 255});

    const Rectangle banishBtn{740, 550, 140, 40};
    DrawRectangleRounded(banishBtn, 0.2F, 4, Color{45, 42, 60, 255});
    DrawRectangleRoundedLines(banishBtn, 0.2F, 4, Color{80, 75, 105, 255});
    std::string bTxt = "[B] Banish (" + std::to_string(sim.player.banishes) + ")";
    DrawText(bTxt.c_str(), 755, 562, 13, Color{240, 120, 120, 255});
  }

  // Minigame Overlay for Treasure Vault Chest
  if (app.descentMinigameActive && app.descentMinigameBossIdx >= 0 &&
      static_cast<std::size_t>(app.descentMinigameBossIdx) < app.bosses.size()) {
    DrawRectangle(0, 0, kWidth, kHeight, Color{10, 10, 18, 235});
    drawTextCentered("VAULT WARDEN'S TRIAL", 75, 26, Color{255, 215, 60, 255});
    drawTextCentered("Crack the vault's arcane seal to claim the gems! (ESC to forfeit)", 110, 14, Color{200, 200, 220, 255});
    drawCampaignMinigames(app, app.bosses[static_cast<std::size_t>(app.descentMinigameBossIdx)]);
  }

  // 2-second Minigame Outcome Popup Banner
  if (app.descentPopupTimer > 0.0F) {
    const float alpha = std::min(1.0F, app.descentPopupTimer / 0.3F);
    const Rectangle popRec{300, 110, 680, 64};
    Color popBg = app.descentPopupSuccess ? Color{25, 60, 35, static_cast<unsigned char>(245 * alpha)}
                                          : Color{65, 25, 30, static_cast<unsigned char>(245 * alpha)};
    Color popBorder = app.descentPopupSuccess ? Color{110, 240, 140, static_cast<unsigned char>(255 * alpha)}
                                              : Color{245, 100, 100, static_cast<unsigned char>(255 * alpha)};
    DrawRectangleRounded(popRec, 0.25F, 4, popBg);
    DrawRectangleRoundedLines(popRec, 0.25F, 4, popBorder);

    std::string title = app.descentPopupSuccess ? "VAULT UNLOCKED!" : "VAULT SEALED - CHALLENGE FAILED";
    Color titleCol = app.descentPopupSuccess ? Color{255, 235, 120, static_cast<unsigned char>(255 * alpha)}
                                            : Color{255, 140, 140, static_cast<unsigned char>(255 * alpha)};
    drawTextCentered(title, 120, 18, titleCol);
    drawTextCentered(app.descentPopupMsg, 146, 13, Fade(WHITE, alpha));
  }

  // Pause Screen Overlay
  if (app.descentPaused) {
    DrawRectangle(0, 0, kWidth, kHeight, Color{10, 10, 18, 235});
    const Rectangle pauseBox{140, 45, 1000, 630};
    DrawRectangleRounded(pauseBox, 0.03F, 6, Color{22, 20, 32, 255});
    DrawRectangleRoundedLines(pauseBox, 0.03F, 6, Color{180, 140, 70, 255});

    // Header
    DrawText("PAUSED - RUN OVERVIEW", static_cast<int>(pauseBox.x + 35), static_cast<int>(pauseBox.y + 20), 22, Color{255, 215, 80, 255});

    std::string chText = sim.isEndless
        ? ("Endless Chamber " + std::to_string(sim.chamberIndex) + " (Loop " + std::to_string(sim.endlessLoop) + ")")
        : ("Chamber " + std::to_string(sim.chamberIndex) + " / " + std::to_string(sim.totalChambers));
    std::string runSummaryTxt = chText +
                                "  |  Run Level " + std::to_string(sim.runLevel) +
                                "  |  Embers: " + std::to_string(sim.embers) +
                                "  |  Kills: " + std::to_string(sim.enemiesKilled);
    DrawText(runSummaryTxt.c_str(), static_cast<int>(pauseBox.x + 450), static_cast<int>(pauseBox.y + 24), 14, Color{190, 190, 210, 255});

    DrawLine(static_cast<int>(pauseBox.x + 30), static_cast<int>(pauseBox.y + 55),
             static_cast<int>(pauseBox.x + pauseBox.width - 30), static_cast<int>(pauseBox.y + 55),
             Color{70, 65, 90, 255});

    // Left Section: Acquired Arms & Tomes
    const float leftX = pauseBox.x + 35.0F;

    // 1. Acquired Arms
    std::string armsHeader = "ACQUIRED ARMS (" + std::to_string(sim.ownedArms.size()) + " / 6)";
    DrawText(armsHeader.c_str(), static_cast<int>(leftX), static_cast<int>(pauseBox.y + 70), 15, Color{245, 175, 75, 255});

    if (sim.ownedArms.empty()) {
      DrawText("No weapons acquired yet.", static_cast<int>(leftX + 15), static_cast<int>(pauseBox.y + 98), 13, Color{140, 140, 155, 255});
    } else {
      for (std::size_t i = 0; i < sim.ownedArms.size(); ++i) {
        const auto& arm = sim.ownedArms[i];
        const float colX = leftX + static_cast<float>(i % 2) * 285.0F;
        const float rowY = pauseBox.y + 95.0F + static_cast<float>(i / 2) * 58.0F;
        const Rectangle armCard{colX, rowY, 275.0F, 52.0F};

        DrawRectangleRounded(armCard, 0.15F, 4, Color{34, 32, 46, 255});
        DrawRectangleRoundedLines(armCard, 0.15F, 4, Color{160, 120, 60, 200});

        std::string aName = (arm.id == "ember_censer") ? "Ember Censer" :
                            (arm.id == "boomerang_axe") ? "Boomerang Axe" :
                            (arm.id == "throwing_daggers") ? "Throwing Daggers" :
                            (arm.id == "orbiting_runeblades") ? "Runeblades" :
                            (arm.id == "chain_lightning_rod") ? "Chain Lightning" :
                            (arm.id == "frost_bell") ? "Frost Bell" :
                            (arm.id == "thorn_bramble") ? "Thorn Bramble" :
                            (arm.id == "holy_lantern") ? "Radiant Lantern" :
                            (arm.id == "gravity_well") ? "Void Well" :
                            (arm.id == "skeletal_horde") ? "Skeletal Horde" :
                            (arm.id == "soul_siphon") ? "Soul Siphon" :
                            (arm.id == "arcane_orb") ? "Arcane Orb" :
                            (arm.id == "seeking_gale") ? "Seeking Gale" :
                            (arm.id == "earth_tremor") ? "Earth Tremor" : arm.id;

        std::string aDesc = (arm.id == "ember_censer") ? "Searing flame aura burns nearby foes." :
                            (arm.id == "boomerang_axe") ? "Whirling axes that pierce & return." :
                            (arm.id == "throwing_daggers") ? "Rapid piercing blades at foes." :
                            (arm.id == "orbiting_runeblades") ? "Mystic blades orbit & shred foes." :
                            (arm.id == "chain_lightning_rod") ? "Bouncing arcs of chain lightning." :
                            (arm.id == "frost_bell") ? "Freezing waves chill & slow foes." :
                            (arm.id == "thorn_bramble") ? "Poisonous brambles on ground." :
                            (arm.id == "holy_lantern") ? "Holy beams pierce enemy hordes." :
                            (arm.id == "gravity_well") ? "Gravitational vortex crushes foes." :
                            (arm.id == "skeletal_horde") ? "Summons loyal skeletons to fight for you." :
                            (arm.id == "soul_siphon") ? "Pulses wards that damage & drain HP." :
                            (arm.id == "arcane_orb") ? "Piercing orb bursts into arcane bolts." :
                            (arm.id == "seeking_gale") ? "Homing wind arrows track down foes." :
                            (arm.id == "earth_tremor") ? "Earthquakes knock back enemy hordes." : "Weapon of the Hollow";

        DrawText(aName.c_str(), static_cast<int>(colX + 10), static_cast<int>(rowY + 7), 13, RAYWHITE);
        std::string lvlTxt = "Lv " + std::to_string(arm.level);
        DrawText(lvlTxt.c_str(), static_cast<int>(colX + armCard.width - 45), static_cast<int>(rowY + 7), 13, Color{255, 215, 80, 255});
        DrawText(aDesc.c_str(), static_cast<int>(colX + 10), static_cast<int>(rowY + 28), 11, Color{180, 180, 195, 255});
      }
    }

    // 2. Acquired Tomes & Perks
    const float tomesTopY = pauseBox.y + 285.0F;
    std::string tomesHeader = "ACQUIRED TOMES & PERKS (" + std::to_string(sim.ownedTomes.size()) + " / 6)";
    DrawText(tomesHeader.c_str(), static_cast<int>(leftX), static_cast<int>(tomesTopY), 15, Color{90, 180, 245, 255});

    if (sim.ownedTomes.empty()) {
      DrawText("No passives acquired yet. Level up to gain tomes.", static_cast<int>(leftX + 15), static_cast<int>(tomesTopY + 28), 13, Color{140, 140, 155, 255});
    } else {
      for (std::size_t i = 0; i < sim.ownedTomes.size(); ++i) {
        const auto& tome = sim.ownedTomes[i];
        const float colX = leftX + static_cast<float>(i % 2) * 285.0F;
        const float rowY = tomesTopY + 25.0F + static_cast<float>(i / 2) * 58.0F;
        const Rectangle tomeCard{colX, rowY, 275.0F, 52.0F};

        DrawRectangleRounded(tomeCard, 0.15F, 4, Color{28, 34, 46, 255});
        DrawRectangleRoundedLines(tomeCard, 0.15F, 4, Color{60, 120, 175, 200});

        std::string tName = (tome.id == "heft") ? "Tome of Heft" :
                            (tome.id == "quickening") ? "Tome of Quickening" :
                            (tome.id == "vitality") ? "Tome of Vitality" :
                            (tome.id == "swiftness") ? "Tome of Swiftness" :
                            (tome.id == "plating") ? "Tome of Plating" :
                            (tome.id == "lodestone") ? "Tome of Lodestone" :
                            (tome.id == "zeal") ? "Tome of Zeal" :
                            (tome.id == "growth") ? "Tome of Growth" :
                            (tome.id == "greed") ? "Tome of Greed" : tome.id;

        std::string tDesc = (tome.id == "heft") ? "+Might: Increases all damage dealt." :
                            (tome.id == "quickening") ? "+Attack Speed: Faster strikes & casts." :
                            (tome.id == "vitality") ? "+Max HP: Increases total survivability." :
                            (tome.id == "swiftness") ? "+Move Speed: Faster repositioning." :
                            (tome.id == "plating") ? "+Armor: Reduces incoming damage." :
                            (tome.id == "lodestone") ? "+Pickup Radius: Magnetizes gems further." :
                            (tome.id == "zeal") ? "+Cooldown Reduction: Faster abilities." :
                            (tome.id == "growth") ? "+XP Gain: Accelerates leveling." :
                            (tome.id == "greed") ? "+Embers: Increases currency dropped." : "Passive perk";

        DrawText(tName.c_str(), static_cast<int>(colX + 10), static_cast<int>(rowY + 7), 13, RAYWHITE);
        std::string rnkTxt = "Rank " + std::to_string(tome.level);
        DrawText(rnkTxt.c_str(), static_cast<int>(colX + tomeCard.width - 55), static_cast<int>(rowY + 7), 13, Color{110, 210, 255, 255});
        DrawText(tDesc.c_str(), static_cast<int>(colX + 10), static_cast<int>(rowY + 28), 11, Color{180, 185, 205, 255});
      }
    }

    // Right Section: Class Kit & Combat Stats
    const float rightX = pauseBox.x + 630.0F;

    DrawText("CLASS ABILITIES & RUN STATS", static_cast<int>(rightX), static_cast<int>(pauseBox.y + 70), 15, Color{220, 210, 140, 255});

    // Class Abilities Card
    const Rectangle classCard{rightX, pauseBox.y + 95.0F, 335.0F, 175.0F};
    DrawRectangleRounded(classCard, 0.08F, 4, Color{30, 28, 40, 255});
    DrawRectangleRoundedLines(classCard, 0.08F, 4, Color{80, 75, 105, 200});

    std::string className = (sim.selectedClass == ed::descent::ClassId::Warrior) ? "Ironclad Warrior" :
                            (sim.selectedClass == ed::descent::ClassId::Tank) ? "Bastion Crusader" :
                            (sim.selectedClass == ed::descent::ClassId::Archer) ? "Shadow Ranger" :
                            (sim.selectedClass == ed::descent::ClassId::Mage) ? "Arcane Elementalist" : "Grave Necromancer";
    DrawText(className.c_str(), static_cast<int>(rightX + 12), static_cast<int>(classCard.y + 10), 14, Color{255, 220, 110, 255});

    std::string primaryTxt = "Primary: " + std::string((sim.selectedClass == ed::descent::ClassId::Warrior) ? "Greatsword Cleave" :
                                                       (sim.selectedClass == ed::descent::ClassId::Tank) ? "Hammer Smash" :
                                                       (sim.selectedClass == ed::descent::ClassId::Archer) ? "Piercing Shot" :
                                                       (sim.selectedClass == ed::descent::ClassId::Mage) ? "Arcane Bolt" : "Soul Scythe");
    DrawText(primaryTxt.c_str(), static_cast<int>(rightX + 12), static_cast<int>(classCard.y + 36), 12, RAYWHITE);

    std::string skillTxt = "Skill (Q): " + std::string((sim.selectedClass == ed::descent::ClassId::Warrior) ? "Whirlwind" :
                                                       (sim.selectedClass == ed::descent::ClassId::Tank) ? "Bulwark Shield" :
                                                       (sim.selectedClass == ed::descent::ClassId::Archer) ? "Rain of Arrows" :
                                                       (sim.selectedClass == ed::descent::ClassId::Mage) ? "Frost Nova" : "Soul Harvest");
    DrawText(skillTxt.c_str(), static_cast<int>(rightX + 12), static_cast<int>(classCard.y + 60), 12, Color{130, 215, 255, 255});

    std::string dashTxt = "Dash (Space): " + std::string((sim.selectedClass == ed::descent::ClassId::Warrior) ? "Combat Roll" :
                                                         (sim.selectedClass == ed::descent::ClassId::Tank) ? "Shield Charge" :
                                                         (sim.selectedClass == ed::descent::ClassId::Archer) ? "Tumble" :
                                                         (sim.selectedClass == ed::descent::ClassId::Mage) ? "Blink" : "Shadow Walk");
    DrawText(dashTxt.c_str(), static_cast<int>(rightX + 12), static_cast<int>(classCard.y + 84), 12, Color{180, 240, 150, 255});

    std::string ultTxt = "Ultimate (R): " + std::string((sim.selectedClass == ed::descent::ClassId::Warrior) ? "Berserker Wrath" :
                                                         (sim.selectedClass == ed::descent::ClassId::Tank) ? "Iron Fortress" :
                                                         (sim.selectedClass == ed::descent::ClassId::Archer) ? "Storm of Arrows" :
                                                         (sim.selectedClass == ed::descent::ClassId::Mage) ? "Meteor Swarm" : "Army of the Dead");
    DrawText(ultTxt.c_str(), static_cast<int>(rightX + 12), static_cast<int>(classCard.y + 108), 12, Color{255, 140, 90, 255});

    // Combat Stats Card
    const Rectangle statsCard{rightX, pauseBox.y + 285.0F, 335.0F, 240.0F};
    DrawRectangleRounded(statsCard, 0.08F, 4, Color{30, 28, 40, 255});
    DrawRectangleRoundedLines(statsCard, 0.08F, 4, Color{80, 75, 105, 200});

    DrawText("CURRENT ATTRIBUTES", static_cast<int>(rightX + 12), static_cast<int>(statsCard.y + 10), 13, Color{200, 200, 220, 255});

    std::string hpS = "Health: " + std::to_string(static_cast<int>(sim.player.currentHp)) + " / " + std::to_string(static_cast<int>(sim.player.maxHp));
    DrawText(hpS.c_str(), static_cast<int>(rightX + 12), static_cast<int>(statsCard.y + 36), 12, Color{240, 90, 90, 255});

    std::string mgtS = "Might: +" + std::to_string(static_cast<int>(std::round((sim.player.might - 1.0F) * 100.0F))) + "%";
    DrawText(mgtS.c_str(), static_cast<int>(rightX + 12), static_cast<int>(statsCard.y + 60), 12, Color{245, 175, 75, 255});

    std::string spdS = "Attack Speed: +" + std::to_string(static_cast<int>(std::round((sim.player.attackSpeed - 1.0F) * 100.0F))) + "%";
    DrawText(spdS.c_str(), static_cast<int>(rightX + 12), static_cast<int>(statsCard.y + 84), 12, Color{110, 220, 255, 255});

    std::string armS = "Armor: " + std::to_string(static_cast<int>(sim.player.armor));
    DrawText(armS.c_str(), static_cast<int>(rightX + 12), static_cast<int>(statsCard.y + 108), 12, Color{170, 180, 200, 255});

    std::string movS = "Move Speed: " + std::to_string(static_cast<int>(sim.player.moveSpeed));
    DrawText(movS.c_str(), static_cast<int>(rightX + 12), static_cast<int>(statsCard.y + 132), 12, Color{150, 230, 140, 255});

    std::string cdrS = "Cooldown Red.: " + std::to_string(static_cast<int>(std::round(sim.player.cooldownReduction * 100.0F))) + "%";
    DrawText(cdrS.c_str(), static_cast<int>(rightX + 12), static_cast<int>(statsCard.y + 156), 12, Color{210, 150, 255, 255});

    std::string pickS = "Magnet Radius: " + std::to_string(static_cast<int>(sim.player.pickupRadius)) + " px";
    DrawText(pickS.c_str(), static_cast<int>(rightX + 12), static_cast<int>(statsCard.y + 180), 12, Color{240, 220, 140, 255});

    std::string xpGrw = "Growth: +" + std::to_string(static_cast<int>(std::round(sim.player.growth * 100.0F))) + "%  |  Greed: +" + std::to_string(static_cast<int>(std::round(sim.player.greed * 100.0F))) + "%";
    DrawText(xpGrw.c_str(), static_cast<int>(rightX + 12), static_cast<int>(statsCard.y + 204), 12, Color{200, 200, 210, 255});

    // Bottom Action Buttons
    const Vector2 mouse = GetMousePosition();
    const Rectangle resumeBtn{360, 605, 260, 46};
    const bool hResume = CheckCollisionPointRec(mouse, resumeBtn);
    DrawRectangleRounded(resumeBtn, 0.2F, 4, hResume ? Color{55, 120, 75, 255} : Color{40, 90, 55, 255});
    DrawRectangleRoundedLines(resumeBtn, 0.2F, 4, hResume ? Color{160, 245, 170, 255} : Color{110, 200, 130, 255});
    DrawText("RESUME (ESC / SPACE)", static_cast<int>(resumeBtn.x + 38), static_cast<int>(resumeBtn.y + 15), 14, WHITE);

    const Rectangle abandonBtn{660, 605, 260, 46};
    const bool hAbandon = CheckCollisionPointRec(mouse, abandonBtn);
    Color abBg = sim.isEndless ? (hAbandon ? Color{45, 95, 120, 255} : Color{35, 70, 95, 255})
                               : (hAbandon ? Color{130, 45, 45, 255} : Color{95, 35, 35, 255});
    Color abBorder = sim.isEndless ? Color{120, 210, 255, 255} : Color{240, 120, 120, 255};
    DrawRectangleRounded(abandonBtn, 0.2F, 4, abBg);
    DrawRectangleRoundedLines(abandonBtn, 0.2F, 4, abBorder);
    const char* abTxt = sim.isEndless ? "RETIRE WITH SPOILS" : "ABANDON RUN";
    DrawText(abTxt, static_cast<int>(abandonBtn.x + (260 - MeasureText(abTxt, 14)) / 2), static_cast<int>(abandonBtn.y + 15), 14, WHITE);
  }
}


} // namespace ed::app
