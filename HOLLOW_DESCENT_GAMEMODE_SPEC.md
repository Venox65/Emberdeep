# The Hollow Descent — Roguelike Survivor Game Mode Spec

> **Add-on to:** *Emberdeep: The Hollow Crown* (main spec v2, C++20 + raylib, Drogon/PostgreSQL backend).
> **Genre:** top-down auto-battler horde survival (Vampire Survivors) inside procedurally generated room-based floors (Binding of Isaac), with Hades-style boons, door-reward previews and a heat/pact system.
> **Audience:** a coding agent. All numbers are **starting values** and live in `data/descent/*.json`.
> **Hard rule:** the player's main-game *equipment* (gear, upgrades, sockets, enchants, legendaries) **never** applies inside a run. Main-game progress reaches the mode only through the explicit, capped channels in Sections 11–12.

---

## 0. Instructions for the Coding Agent

1. Read the main spec's Section 0, 2 and 21 first. This mode reuses the `core` / `client` / `server` split, raylib, the `gfx::`/`audio::` wrappers, the asset pipeline and the procedural-art fallback.
2. **Game rules live in `core/descent/`** (no raylib, no I/O). The client draws; the server only validates summaries (Section 14).
3. **Data-driven everything:** classes, arms, tomes, boons, relics, enemies, biomes, room chunks, pacts and meta nodes are JSON, validated against schemas in `data/schema/descent/`. Boons and relics are built from the **trigger → condition → action** effect system in Section 17.3, never hard-coded one by one.
4. Build in the phases in **Section 19**. Finish and verify each phase before the next.
5. Everything must work with **zero external assets** (shape/pixel art fallback). Pixel sprites from Section 21 of the main spec are an optional enhancement.
6. Performance is a feature: **300 live enemies + 800 projectiles at 60 FPS** on an integrated GPU (Section 17).
7. Record unclear decisions in `docs/DECISIONS.md` and keep going.

### Glossary (maps the design brief to terms used here)
| Brief term | In this spec |
|---|---|
| **Perks** | **Tomes** (leveled passives picked on level-up) and the permanent **Ember Anvil** nodes |
| **Blessings** | **Patron Boons** (Hades-style, from six Patrons) |
| **Buffs** | Temporary effects from shrines, fountains and consumables (**Brews**) |
| **Curses** | Optional downsides taken from shrines, relics and **Pacts** (heat) |
| **Items** | **Relics** (Isaac-style passive finds) |

---

## 1. Pitch & Pillars

**The Hollow Descent:** you are a Delver who has fallen through a crack in the deepest Aether Vault into the *Hollow*, the dream-realm of **Vaelor, the Crowned Hunger**. Fight down through three cursed biomes, collect the favor of the old Patrons, and kill the Hunger on its throne. Every death sends you back to the Ember Anvil, a little stronger.

**Pillars**
- **Power fantasy through build-making.** Within ~10 minutes you should feel the screen fill with your own damage. Builds come from stacking Arms, Tomes, Boons and Relics with real synergies.
- **Two layers of decisions.** Moment-to-moment: movement, dashing, positioning. Between moments: room-door choices, boon picks, shop buys, risk/reward shrines.
- **Fair and readable.** Every enemy attack is telegraphed; the player can always see what killed them.
- **Short, replayable runs.** ~25–35 min per full run; 8–12 min for a single biome. Runs can be suspended at room boundaries.
- **Meta that respects the main game.** Mining, crafting and campaign progress make runs easier through bounded, clearly labeled channels, but a skilled player can still win with no preparation.
- **Replay after the final boss.** Heat, class Aspects, Crown Boons, daily/weekly seeds and the endless Loop keep the mode alive after the first win.

---

## 2. Mode Flow & Unlocking

### 2.1 Unlock
The mode appears as a **Descent** button in the main navigation bar (Section 3 of the main spec) once **Campaign Boss 3 (Morwenna) is cleared** and **player level ≥ 20**. A one-time cutscene (a crack opening below the Gem Hollow) introduces it.

### 2.2 Hub (`DescentHub` screen)
```
┌──────────────────────────────────────────────────────────────────┐
│  THE HOLLOW DESCENT         Soul Shards 340   Crown Shards 12    │
├───────────────┬──────────────────────────────┬───────────────────┤
│ [Begin Run]   │  Selected class preview      │ Last run summary  │
│ [Ember Anvil] │  (animated pixel hero)       │ Best Loop / Heat  │
│ [Preparations]│  Class mastery bar           │ Daily seed timer  │
│ [Codex]       │  Aspect / Crown Boon slots   │ Weekly challenge  │
│ [Leaderboard] │                              │                   │
└───────────────┴──────────────────────────────┴───────────────────┘
```
- **Begin Run** opens the Preparations screen (Section 11.1), then starts the run.
- Modes: **Standard Run** (random seed), **Daily Descent** (shared seed, 1 scored attempt per day, unlimited practice), **Weekly Trial** (fixed seed + fixed modifiers), **Endless Loop** (after the first win).
- **Resume Run** shows instead of Begin Run if a suspended run exists.

### 2.3 Run Loop (Overview)
```
Preparations → Biome 1 floor (≈9 rooms) → Biome 1 Boss → rest/stairs
            → Biome 2 floor (≈10 rooms) → Biome 2 Boss → rest/stairs
            → Biome 3 floor (≈11 rooms) → Biome 3 Boss → rest/stairs
            → The Crowned Hollow (2 gauntlet rooms) → Vaelor (final boss)
            → Victory summary → rewards → Hub
```
Death at any time ends the run (unless a revive is owned) and goes to the Run Summary. Quitting mid-run via the pause menu **abandons** the run (counts as a death; Soul Shards are paid at 50%). **Suspend** (save & exit) is allowed only between rooms.

---

## 3. Core Gameplay

### 3.1 Presentation & Scale
- Top-down 2D view, camera follows the player with slight look-ahead (smoothing 0.15 s). Logical resolution 1280×720; **tile = 32 px**; pixel art authored at 16 px and drawn ×2.
- Rooms are larger than the screen (Section 6.3), so the camera scrolls and clamps to room bounds.
- Sprite sizes: player 32×32 px on screen, small enemies 24–32 px, brutes 48–64 px, mini-bosses 96–128 px, bosses 192–320 px.

### 3.2 Controls
| Action | Keyboard / Mouse | Gamepad (later) |
|---|---|---|
| Move | `WASD` / arrows | Left stick |
| Aim | Mouse cursor | Right stick |
| Primary attack | **Automatic** (see 3.4) | Automatic |
| Dash | `Space` | `A` |
| Skill | `Q` or Right mouse | `X` |
| Ultimate (**Wrath**) | `R` | `Y` |
| Interact (doors, shops, veins, cracked walls) | `E` (hold for mining) | `B` |
| Pause / Codex | `Esc` / `Tab` | Start |
All input passes through the shared input layer; keys are rebindable.

### 3.3 Player Stats
Base values come from the class; everything else is modified by Tomes, Boons, Relics, Meta and Pacts.

| Stat | Meaning | Base / cap |
|---|---|---|
| **Max HP** | Health | class (80–200) |
| **Armor** | Flat damage subtracted from each hit taken | class (0–8) |
| **Damage Reduction** | Percentage reduction, applied before Armor | 0%, cap 60% |
| **Move Speed** | px/s | 165–205 |
| **Might** | + % damage dealt | 0% |
| **Attack Speed** | + % attack rate | 0% |
| **Area** | + % size of attacks and effects | 0% |
| **Projectile Speed** | + % | 0% |
| **Duration** | + % effect duration | 0% |
| **Amount** | + N projectiles/minions (integer) | 0 |
| **Cooldown Reduction** | Skills, dash recharge | 0%, cap 50% |
| **Crit Chance / Crit Damage** | | 5% / +50% |
| **Luck** | Better offers and drops (Section 5.6) | 0 |
| **Pickup Radius** | px | 72 |
| **Growth** | + % XP gained | 0% |
| **Greed** | + % Embers gained | 0% |
| **Regen** | HP per second | 0 |
| **Revives** | Extra lives | 0 |
| **Rerolls / Banishes / Skips** | Choice manipulation counters | 1 / 1 / 1 |

### 3.4 Combat Rules
- **Damage dealt:** `hit = base × (1 + Might) × (crit ? 1 + critDamage : 1) × situational`; vs. enemy armor: `final = hit × 100 / (100 + enemyArmor)` (same convention as the main game). Minimum 1.
- **Damage taken:** `taken = max(ceil(incoming × 0.2), incoming × (1 − DR) − Armor)`.
- **Invulnerability:** 0.5 s after any hit taken (contact or projectile); 0.2 s during a dash.
- **Primary attack** fires automatically. *Melee* classes (Warrior, Tank) swing at the nearest enemy in range; *ranged* classes (Archer, Mage) fire toward the cursor (manual aim) with an optional **Auto-aim** setting; the Summoner commands minions and lashes the nearest enemy.
- **Dash:** distance 200 px in 0.18 s, **2 charges**, each recharging in 3 s (reduced by Cooldown Reduction). Passes through enemies and projectiles (i-frames), not walls or hazards.
- **Skill (`Q`):** class active with a 7–9 s base cooldown.
- **Wrath meter / Ultimate (`R`):** fills with **+1 per kill, +10 per elite, +25 per mini-boss, +2 per 10 damage taken**. At 100 the ultimate can be cast; it resets to 0.
- **Knockback & stagger:** small enemies are knocked back by most hits (0 for bosses). Heavy enemies resist. **Hit-stop** of 30 ms on crits and 60 ms on big kills (toggleable).
- **Status effects:** Burn (damage over time, stacks to 3), Chill (−30% move speed), Freeze (stopped 1.5 s, +50% damage taken, then 3 s immunity), Hex (+12% damage taken), Poison (damage over time, ignores armor), Stun (0.5 s), Shock (chain on hit), Bleed (damage on moving enemies). Bosses cap Freeze and Stun durations at 25%.

### 3.5 Pickups & In-Run Economy
| Pickup | Source | Use |
|---|---|---|
| **XP Gem** (blue; green, purple for bigger) | Enemies | Fills the XP bar → level-up. Magnetized by Pickup Radius |
| **Embers** (gold coins) | Enemies (low), chests, veins, elites | In-run currency for **shops**, shrines and the Anvil |
| **Roast** (food) | Rare drops (3% base, +Luck) | Heal 20% Max HP |
| **Ember Stone** | Door rewards, mini-bosses | Upgrade a Boon by one level (Section 5.4) |
| **Chest** | Elites, treasure rooms | Relic or Arm evolution |
| **Soul Fragment** (violet) | Elites and bosses only | Banked into **Soul Shards** at run end (meta currency) |
| **Brew** (potion) | Shops, shrines, random | Consumable (Section 5.7) |
Gems auto-magnetize when you clear a room (all pickups fly to the player on room clear).

### 3.6 Level-Ups
- XP needed for run level `L`: `xpNeed(L) = round(8 + 5L + 0.35L²)`. Run level cap **30**; beyond that, XP converts to Embers.
- On level-up the game **pauses** and offers **3 cards** (a 4th if a Luck roll succeeds). Pick one, **Reroll** (spends a reroll), **Banish** a card from this run's pool, or **Skip** (heal 10% HP).
- Offer pool: upgrade an owned Arm (weight 3 each), new Arm (weight 2, only if a slot is free), upgrade an owned Tome (weight 3), new Tome (weight 2, if a slot is free). If nothing is available the card is **Heal 30%** or **+60 Embers**.
- **Arm slots: 4** (+1 per relic *Cracked Crown Shard*, max 6). **Tome slots: 4** (max 6).

---

## 4. Classes

Five classes. Each has: a **primary attack**, a **Skill (Q)**, a **Dash variant**, a **Passive**, an **Ultimate (R, Wrath)**, a starting **Arm** and a stat spread. Class affinity tags (`[Melee] [Projectile] [Area] [Summon] [Crit] [Defense]`) bias which boons and relics are offered.

### 4.1 Unlock Conditions (rewards main-game progress)
| Class | Unlocked by |
|---|---|
| **Warrior** (Ashblade) | Mode unlock |
| **Archer** (Moonfeather Ranger) | Campaign Boss 4 cleared |
| **Tank** (Bastion Knight) | Campaign Boss 5 cleared |
| **Mage** (Runeweaver) | Campaign Boss 6 cleared |
| **Summoner** (Soulshepherd) | Campaign Boss 9 cleared **or** first Descent victory with any class |

### 4.2 Base Stats
| | Max HP | Armor | Move | Crit | Special |
|---|---|---|---|---|---|
| Warrior | 120 | 3 | 190 | 5% | +10% Area |
| Tank | 200 | 8 | 165 | 0% | +10% Damage Reduction |
| Archer | 90 | 0 | 205 | 10% | +20% Projectile Speed |
| Mage | 80 | 0 | 190 | 5% | +15% Duration |
| Summoner | 100 | 1 | 180 | 5% | +1 Amount (minions) |

### 4.3 Warrior — *Ashblade* `[Melee] [Area] [Crit]`
*A brawler who gets faster as the carnage builds.*
- **Primary — Sweep:** auto-swings a 140° arc (range 70 px) at the nearest enemy, 1.1 attacks/s, damage 14, hits everything in the arc.
- **Skill — Whirling Blades (CD 8 s):** spins for 1.2 s, 360° damage ticks (6 hits × 60% damage), +30% move speed during the spin.
- **Dash — Charge Strike:** damages enemies passed through (80% of a Sweep).
- **Passive — Bloodrush:** every kill grants +2% Attack Speed for 4 s (stacks to 10, refreshed per kill).
- **Ultimate — War Cry (6 s):** +40% Might, +25% Attack Speed, heal 20% Max HP over the duration, and a shockwave on cast (staggers nearby enemies).
- **Starting Arm:** *Boomerang Axe*.

### 4.4 Tank — *Bastion Knight* `[Defense] [Melee] [Area]`
*Wades into the horde, turns damage into damage.*
- **Primary — Shield Slam:** short cone (90°, range 60 px), 0.8 attacks/s, damage 10 **+ 25% of current Armor** as bonus damage, strong knockback.
- **Skill — Bulwark (CD 9 s):** raises the shield for 3 s, blocks all frontal projectiles and 70% of frontal contact damage, reflects 50% of blocked projectile damage.
- **Dash — Shield Rush:** 0.5 s stun and heavy knockback to enemies hit.
- **Passive — Aegis Barrier:** a barrier worth 20% Max HP that regenerates after 6 s without taking damage; **Thorns:** reflects 15% of damage taken as area damage (50 px) around you.
- **Ultimate — Fortress (6 s):** taunts all enemies within 400 px, −50% damage taken, pulses damage once per second.
- **Starting Arm:** *Ember Censer* (aura).

### 4.5 Archer — *Moonfeather Ranger* `[Projectile] [Crit]`
*Glass-cannon kiter, rewarded for precise aim and movement.*
- **Primary — Arrow:** fires toward the cursor, 2.2 shots/s, damage 9, pierces 1 enemy, speed 520 px/s.
- **Skill — Arrow Volley (CD 7 s):** 5 arrows in a fan (45°), each 70% damage.
- **Dash — Backroll:** your next 3 arrows within 2 s are guaranteed crits.
- **Passive — Hunter's Mark:** the 3rd consecutive hit on the same enemy always crits.
- **Ultimate — Rain of Arrows (4 s):** a 160 px radius zone at the cursor that fires 12 arrows/s (60% damage each) and slows enemies 40%.
- **Starting Arm:** *Throwing Daggers*.

### 4.6 Mage — *Runeweaver* `[Projectile] [Area]`
*Stand and cast for big damage, or blink and re-position.*
- **Primary — Arcane Bolt:** homing bolt (turn rate 3 rad/s) toward the cursor, 1.3 shots/s, damage 12, explodes in a 40 px radius.
- **Skill — Meteor (CD 9 s):** marks the cursor position; a meteor lands after 1.0 s for 300% damage in a 90 px radius, burning ground for 3 s.
- **Dash — Blink:** teleports 220 px (instant), leaves a rune that detonates for 120% damage after 1 s.
- **Passive — Arcane Focus:** while standing still (not moving for 0.4 s) gain +3% Might per second, up to +30%; every 5th bolt chains lightning to 2 enemies.
- **Ultimate — Arcane Storm (5 s):** a rotating ring of 6 orbs 120 px from you, each dealing 100% damage/s and homing bolts at nearby enemies.
- **Starting Arm:** *Chain Lightning Rod*.

### 4.7 Summoner — *Soulshepherd* `[Summon] [Area]`
*Fights through an army: your minions deal most of the damage.*
- **Primary — Soul Lash:** weak auto beam (range 120 px) at the nearest enemy, 0.9/s, damage 6, applies Hex for 3 s.
- **Starting kit — 2 Spirit Wisps:** minions that chase and strike enemies (damage 8, 1.5 attacks/s, 20 HP each, respawn after 6 s). Minion **cap 12**.
- **Skill — Spectral Knight (CD 9 s):** summons a knight for 12 s (80 HP, taunts, melee 18 damage). Max 3.
- **Dash — Phase Step:** swap positions with the nearest minion (or dash normally if none within 300 px).
- **Passive — Bond of Souls:** minions inherit 30% of your Might, Area and Attack Speed; a minion kill has a 10% chance to spawn a free Wisp for 8 s.
- **Ultimate — Legion (10 s):** summons 8 spectral soldiers (don't count toward the cap), all minions gain +50% damage.
- **Starting Arm:** *Spirit Hounds* (summon).

### 4.8 Aspects (Alternate Kits)
Each class has **2 Aspects** that change one defining mechanic and are unlocked through **class mastery** (Section 10.3) and clears.

| Class | Aspect | Unlock | Change |
|---|---|---|---|
| Warrior | **Aspect of the Berserker** | Mastery 5 | Sweep becomes a fast 3-hit combo; lose 1% Max HP/s; +60% Might below 40% HP |
| Warrior | **Aspect of the Duelist** | First clear | Replaces Sweep with a thrust (long reach, single target, +50% crit damage); parry window on dash |
| Tank | **Aspect of the Colossus** | Mastery 5 | Slower, bigger slam (x2 Area, x1.6 damage); barrier doubled |
| Tank | **Aspect of the Warden** | First clear | Bulwark becomes a placeable shield wall (5 s) |
| Archer | **Aspect of the Sniper** | Mastery 5 | 0.9 shots/s, +200% damage, pierces everything |
| Archer | **Aspect of the Trickster** | First clear | Arrows ricochet 2 times; dash leaves a decoy |
| Mage | **Aspect of the Pyromancer** | Mastery 5 | Bolts become fireballs (burn, no homing, +30% Area) |
| Mage | **Aspect of the Chronomancer** | First clear | Skill becomes a time bubble (enemies inside move at 40% speed for 4 s) |
| Summoner | **Aspect of the Necromancer** | Mastery 5 | Minions are resurrected skeletons that explode on death |
| Summoner | **Aspect of the Beastmaster** | First clear | Wisps become 2 Spirit Hounds with bleed attacks |

---

## 5. In-Run Progression Layers

Five layers stack in a run. Everything resets on death except the meta layers in Section 10.

### 5.1 Arms (Secondary Auto-Weapons)
Gained from level-ups, shops and treasure rooms; **4 slots**; each Arm has **5 levels**. They fire automatically with no input.

| Arm | Behavior (level 1) | Level-ups add | Evolution |
|---|---|---|---|
| **Throwing Daggers** | Throws a dagger forward every 1.2 s, pierce 1, dmg 8 | +1 dagger, +dmg, +pierce | **Storm of Steel** (with *Quickening* ≥ 3): 8 daggers in a spiral |
| **Orbiting Runeblades** | 1 blade orbits at 90 px, dmg 6/hit | +blades, +speed, +radius | **Sunwheel** (with *Reach* ≥ 3): 6 blades, burn, huge radius |
| **Ember Censer** | Aura (60 px) dealing 5 dmg/0.5 s + Burn | +radius, +dmg | **Wyrmbreath Thurible** (with *Hearth* ≥ 3): periodic fire nova |
| **Chain Lightning Rod** | Arcs to the nearest enemy, chains 2, 10 dmg per 1.6 s | +chains, +dmg | **Tempest Spire** (with *Conduit* ≥ 3): storm cloud above you |
| **Frost Bell** | Pulse every 3 s, 14 dmg + Chill in 120 px | +radius, faster | **Winter's Toll** (with *Rime* ≥ 3): pulse freezes |
| **Thorn Bramble** | Drops thorn patches (4 s, dmg 4/0.5 s) every 2.5 s | +patches, +duration | **Briar Garden** (with *Verdance* ≥ 3): patches regrow and root |
| **Boomerang Axe** | Axe flies out 220 px and returns, dmg 12, pierces | +axes, +dmg | **Reaver's Return** (with *Heft* ≥ 3): axes leave bleed trails |
| **Holy Lantern** | Lights a rotating 60° cone (180 px), 8 dmg/0.4 s | +cones, +range | **Dawnbringer** (with *Grace* ≥ 3): full-circle sunbeam, heals on kill |
| **Gravity Well** | Drops a vortex (2 s) pulling enemies, dmg 6/0.3 s | +radius, +duration | **Voidmaw** (with *Void Touch* ≥ 3): collapses at end for huge damage |
| **Spirit Hounds** | 2 hounds chase enemies (summon, dmg 7) | +hounds, +speed | **Hellhound Pack** (with *Packbond* ≥ 3): flaming hounds, explode on death |
**Evolutions** happen at **Evolution Altars** (in treasure rooms and after mini-bosses): an Arm at level 5 plus its paired Tome at level ≥ 3 can be evolved for free.

### 5.2 Tomes (Passive Perks)
Each Tome has 5 levels. **4 slots.**

| Tome | Effect per level |
|---|---|
| **Heft** | +8% Might |
| **Quickening** | +6% Attack Speed |
| **Reach** | +8% Area |
| **Swiftness** | +5% Move Speed |
| **Vitality** | +10% Max HP (heals the added amount) |
| **Plating** | +1 Armor |
| **Lodestone** | +15% Pickup Radius |
| **Fortune** | +8% Luck-equivalent (each point = +1 Luck) |
| **Tempo** | +5% Cooldown Reduction |
| **Echo** | +8% Duration; level 3 and 5: +1 Amount |
| **Hearth** | +1 HP/s Regen at level 3 and 5; +10% Burn damage |
| **Conduit** | +10% shock/chain damage; +1 chain at level 3 and 5 |
| **Rime** | +10% Chill/Freeze effect; +1 s freeze at level 5 |
| **Verdance** | +10% damage over time effects |
| **Grace** | +5% healing received |
| **Void Touch** | +8% damage vs. Hexed or slowed |
| **Packbond** | +10% minion damage and health |

### 5.3 Pacing Targets (per Level-Up)
Level 1–10 should happen in the first biome; Level 20 by about the middle of Biome 3. An average run reaches ~Level 24 at the final boss. Tomes and Arms should be mostly maxed by the final boss only if the player prioritizes them.

### 5.4 Patron Boons (Blessings)
Six **Patrons** offer **Boons** as room rewards. The door of the next room shows the reward icon (Section 6.6) so the player chooses a route and a Patron.

**Boon slots:** Attack, Skill, Dash, Aura, Utility. **A character holds at most one Boon per slot.** Offers only include Boons for empty slots, or upgrades to a held Boon (Ember Stones); a **Reforge** option (replace a held Boon) appears only at an Anvil.
**Rarity** (base weights): Common 65% · Rare 25% · Epic 8% · Legendary 2% (only offered when prerequisites are held). Luck pushes weight toward higher rarities (+1% per Luck, shifted from Common). Multiplier on the Boon's numbers: Common ×1, Rare ×1.5, Epic ×2.
**Levels:** Each **Ember Stone** adds +1 Boon level (to 5): +40% of the Boon's base value per level.

| Patron | Domain | Unlocked by (main game) |
|---|---|---|
| **Terra** | Stone, ore, defense | Mode unlock |
| **Aurel** | Gold, fortune | Mode unlock |
| **Ignis** | Fire, wyrmkind | Campaign Boss 3 (mode unlock) |
| **Selene** | Silver, moon, frost, crits | Campaign Boss 4 |
| **Runa** | Runes, arcane, knowledge | Campaign Boss 6 |
| **Nyx** | Void, shadow, hunger | Campaign Boss 9 |

**Boons (base values at Common, level 1):**

| Patron | Slot | Boon | Effect |
|---|---|---|---|
| **Terra** | Attack | **Seismic Strike** | Every 6th attack triggers a shockwave (120 px) for 80% damage |
| | Skill | **Granite Skin** | Skills grant a 3 s +3 Armor buff |
| | Dash | **Boulder Charge** | Dash knocks back and stuns (0.4 s) enemies passed through |
| | Aura | **Bedrock Will** | Below 30% HP: −25% damage taken |
| | Utility | **Vein Sense** | Mining veins (Section 6.7) appear 50% more often; yield ×2 |
| **Aurel** | Attack | **Gilded Touch** | Kills have 8% chance to drop 3× Embers |
| | Skill | **Midas Mark** | Skills mark the target elite: +25% damage taken; drops a chest on death |
| | Dash | **Coin Scatter** | Dash drops coins that damage enemies (20 dmg each) and can be collected |
| | Aura | **Coin Shield** | +1 Armor per 50 held Embers (cap +6) |
| | Utility | **Tax Collector** | Shop prices −20%, +1 shop offer |
| **Ignis** | Attack | **Searing Edge** | Attacks apply Burn (30% damage/s for 3 s, stacks to 3) |
| | Skill | **Cinder Ground** | Skills leave burning ground (3 s) |
| | Dash | **Flame Trail** | Dash leaves a fire trail (2 s) |
| | Aura | **Warm Blood** | +8% Might per burning enemy within 200 px (cap +24%) |
| | Utility | **Kindling** | Kills of burning enemies explode (60 px, 50% damage) |
| **Selene** | Attack | **Frostbite** | 15% chance to Chill; chilled enemies take +15% crit chance |
| | Skill | **Moonlit Strike** | Skill hits guarantee a crit on the first target |
| | Dash | **Moon Dance** | +1 Dash charge |
| | Aura | **Winter's Hush** | Enemies within 150 px are slowed 12% |
| | Utility | **Tidal Pull** | +30% Pickup Radius; gems magnetize on spawn |
| **Runa** | Attack | **Arcane Echo** | 15% chance an attack repeats at 50% damage |
| | Skill | **Rune Cascade** | −20% skill cooldown; casting a skill grants +5% Wrath |
| | Dash | **Glyph Step** | Dash leaves a rune that explodes after 1 s (100% dmg) |
| | Aura | **Scholar's Insight** | +15% Growth (XP) |
| | Utility | **Sigil of Warding** | The first hit in each room is negated |
| **Nyx** | Attack | **Hex Strike** | Attacks apply Hex (+12% damage taken); spreads on death |
| | Skill | **Eclipse** | Skills pull enemies toward the target point |
| | Dash | **Shadow Step** | +0.3 s i-frames after dash; next hit +40% damage |
| | Aura | **Hungering Dark** | +20% Might, −15% Max HP |
| | Utility | **Void Siphon** | Lifesteal 1.5% of damage dealt (max 2 HP/s) |

**Duo Boons** (take no slot; offered when you hold **both** required Boons):
| Duo | Requires | Effect |
|---|---|---|
| **Steam Burst** | Ignis + Selene | Chilled burning enemies explode in steam (area 100 px, +200% DoT damage) |
| **Arcane Fire** | Ignis + Runa | Echoed attacks always apply Burn and ignite the ground |
| **Gold Vein** | Aurel + Terra | Veins yield Soul Fragments; +30% Embers from mining |
| **Gravebound** | Terra + Nyx | Stunned enemies are Hexed; hexed kills drop barrier shards (+5 barrier) |
| **Midas Glyph** | Aurel + Runa | Runes pay Embers when they detonate |
| **Eclipse Frost** | Selene + Nyx | Hexed enemies are always Chilled; frozen enemies take +40% crit damage |

**Legendary Boons** (take no slot; each requires one specific Boon at level ≥ 3):
| Legendary | Requires | Effect |
|---|---|---|
| **Phoenix Rebirth** (Ignis) | Searing Edge | Revive once at 50% HP with a screen-wide fire nova |
| **Absolute Zero** (Selene) | Frostbite | Ultimate freezes all enemies in the room for 3 s |
| **Golden Age** (Aurel) | Tax Collector | Embers also grant XP; +1% Might per 100 Embers held (cap +25%) |
| **Runic Overload** (Runa) | Rune Cascade | Every 10 s your next Skill casts twice |
| **Mountain's Heart** (Terra) | Granite Skin | Gain a barrier equal to 30% Max HP at the start of each room |
| **Event Horizon** (Nyx) | Hex Strike | Every 20 kills spawns a black hole (4 s) |

### 5.5 Relics (Items)
Found in **Treasure Rooms** (choose 1 of 3), chests, shops, boss rewards and secret rooms. No slot limit. Relics can have **tradeoffs**, in the spirit of The Binding of Isaac. Offered relics are weighted by class tags and **exclude** relics already owned.

| Relic | Rarity | Effect |
|---|---|---|
| **Cracked Crown Shard** | Rare | +1 Arm slot, −10% Max HP |
| **Miner's Lantern** | Common | +40% Pickup Radius; reveals secret rooms on the minimap |
| **Pickaxe of Echoes** | Rare | Attacks hit nearby veins and cracked walls from range |
| **Rusty Heart** | Common | +15% Max HP, −5% Move Speed |
| **Glass Cannon Charm** | Rare | +40% Might, damage taken ×1.5 |
| **Wyrm Scale** | Rare | +2 Armor; enemies that touch you take 10 fire damage |
| **Lucky Rabbit's Foot** | Common | +5 Luck |
| **Hourglass of Ash** | Epic | Cooldowns −25%; Embers earned −15% |
| **Vampiric Fang** | Rare | Heal 1 HP per 12 kills |
| **Bone Compass** | Common | Door previews also show enemy count and elite flag |
| **Scholar's Monocle** | Common | +20% Growth (XP) |
| **Thief's Gloves** | Common | Shop prices −15%; can steal one item per shop (bounty hunter spawns, see 6.5) |
| **Phoenix Feather (Relic)** | Epic | Revive once at 40% HP (consumed) |
| **Mirror Shield** | Rare | 10% chance to reflect any projectile |
| **Overcharged Core** | Epic | Every 8 s your next attack is +300% damage |
| **Heavy Boots** | Common | Cannot be knocked back; −10% Move Speed |
| **Eye of the Hunger** | Epic | Elites you kill drop an extra Soul Fragment; elites gain +25% HP |
| **Twin Souls** | Epic | +1 Amount; −15% Might |
| **Rune of Ruin** | Epic | Crits explode (50 px, 40% damage) |
| **Alchemist's Flask** | Common | Brews you drink are twice as strong |
| **Echo of the Burrow-King** | Legendary* | Every 10th hit deals ×3 damage (*unlocked when the main-game legendary **Burrowking's Crown** is owned) |
**Main-game legendary tie-in:** owning a boss legendary in the main game unlocks a themed relic in the mode (Section 11.4).

### 5.6 Luck
Luck improves: Boon rarity, a 4th level-up card chance (+1% per Luck, cap 30%), Roast and chest drop rates, relic quality and shop discounts (-0.5% per Luck, cap −20%).

### 5.7 Shrines, Fountains and Brews (Buffs & Curses)
**Shrines** appear in Shrine rooms and are *always optional*.

| Shrine | Effect |
|---|---|
| **Blood Altar** | Pay 25% of current HP → choose 1 of 3 relics |
| **Greed Shrine** | Accept a **Curse** (−10% Max HP *or* +15% enemy HP for the floor) → gain 150 Embers |
| **Whisper Shrine** | Pick 1 of 3 **Curses** for a permanent run-long +8% Might |
| **Wishing Well** | Spend 100 Embers → random Boon (any unlocked Patron, any rarity) |
| **Anvil** | Spend Embers to upgrade an Arm by 1 level, or **Reforge** a Boon |
| **Fountain** | Heal 40% (once per floor) |
| **Altar of the Patrons** | Choose the Patron of your next 2 boon offers |
**Brews** (consumables; max 3 carried; use with `1`/`2`/`3`):
| Brew | Effect |
|---|---|
| **Ember Draught** | +25% Might for 30 s |
| **Stoneblood** | +5 Armor for 30 s |
| **Quicksilver** | +30% Move Speed and Attack Speed for 20 s |
| **Elixir of Mending** | Heal 50% Max HP |
| **Phoenix Tonic** | Revive once at 30% HP (lasts the run) |
| **Scholar's Vial** | Instantly gain 1 level |
**Curses** (from shrines/relics/Pacts): Frailty (−10% Max HP), Rust (−10% Might), Sloth (−8% Move Speed), Famine (−20% Embers), Dull Wits (−15% Growth), Haunted (+15% enemy density for the floor).

---
## 6. Floors & Procedural Room Generation

### 6.1 Structure
A run is **3 floors** (one per biome) followed by the **Crowned Hollow** finale. A floor is a **grid of rooms** (Isaac-style) where each room is a self-contained arena larger than the screen. Doors connect adjacent grid cells. Doors are **locked during combat** and open when the room is cleared.

| Floor | Biome | Rooms (±1) | Target time |
|---|---|---|---|
| 1 | Gravebarrow Halls | 9 | 7–9 min |
| 2 | Mirewood Hollows | 10 | 8–10 min |
| 3 | Emberforge Depths | 11 | 9–11 min |
| Finale | The Crowned Hollow | 2 gauntlets + boss | 5–8 min |

### 6.2 Floor Layout Algorithm
Grid size **9 × 7**. All randomness uses the floor RNG stream (Section 6.11).

```
function generateFloor(seed, biome):
    rng = Rng(seed)
    targetRooms = biome.baseRooms + rng.range(-1, +1)
    required    = 5            // Boss, Miniboss, Treasure, Shop, Shrine (Fountain on floors 2-3 is extra)
    repeat until valid:                                   // max 50 attempts, then fallback layout
        grid.clear(); place Start at (4,3); frontier = [Start]
        while rooms < targetRooms and frontier not empty:
            cell = rng.pick(frontier)
            for each neighbour n of cell in random order:
                if n is free and n.filledNeighbours <= 1 and rooms < targetRooms and rng.chance(0.5):
                    add room n; frontier.push(n)
        deadEnds = rooms with exactly 1 door, excluding Start
        // ensure enough dead ends for special rooms; extend branches if short
        while len(deadEnds) < required: extend a random non-dead-end room into a free cell with 1 filled neighbour
        valid = (len(deadEnds) >= required)
    // assign special rooms
    Boss      = dead end with max BFS distance from Start
    Miniboss  = a different dead end, >= 3 steps from Start
    Treasure, Shop, Shrine = other dead ends (random order)
    Fountain  (floors 2-3) = random dead end or branch room at distance 2 from Start
    Elite     = 1-2 non-dead-end rooms at distance >= 3
    Secret    = free cell with >= 2 filled neighbours (hidden; opened by mining a cracked wall)
    VeinCavern= 0-1 dead end if Terra boons offered or 20% chance
    everything else = Combat (Gauntlet with 10% chance on floors 2 and 3)
```
**Gating:** the Boss door is locked until the **Miniboss** is defeated (it drops the *Warden's Key*). The Boss room is the only exit; beating it opens the descent stairs to the next floor. The minimap marks the Boss room with a skull and a lock.
**Backtracking** is allowed; cleared rooms stay cleared and never respawn enemies. A cleared room's pickups remain.

### 6.3 Room Sizes (Tiles)
| Size | Tiles (W × H) | Pixels | Used for |
|---|---|---|---|
| Small | 40 × 22 | 1280 × 704 (one screen) | Shop, Shrine, Treasure, Fountain, Secret, Start |
| Medium | 56 × 32 | 1792 × 1024 | Combat, Elite, Vein Cavern |
| Large | 72 × 40 | 2304 × 1280 | Gauntlet, Miniboss |
| Boss | 56 × 32 (circular or rounded) | 1792 × 1024 | Boss rooms (biome specific) |

### 6.4 Room Types
| Type | Contents | Reward |
|---|---|---|
| **Start** | Safe, no enemies; class tutorial hints on floor 1 | — |
| **Combat** | 3 waves of enemies (Section 7) | Door reward (see 6.6) |
| **Elite** | 3 waves including 1–2 **Elites** | Reward + guaranteed Chest |
| **Gauntlet** | 5 waves, ×1.8 budget; optional (marked with a flaming door) | Double reward + a Brew |
| **Miniboss** | A single mid-boss with minion waves (Section 8) | Warden's Key, Evolution Altar, Ember Stone ×2 |
| **Boss** | Floor boss | Boss relic, full heal, a Boon of choice, stairs |
| **Treasure** | Chest guarded by a trap (waves) or a sealed room; also holds an **Evolution Altar** | Choose 1 of 3 Relics |
| **Shop** | A merchant (Section 6.5) | Spend Embers |
| **Shrine** | One random shrine from Section 5.7 | Varies |
| **Fountain** | Heals 40% once; one free Brew refill | Heal |
| **Vein Cavern** | Rich mining veins plus a few enemies | Embers, gems |
| **Secret** | Hidden; extra loot or a rare shrine | 1 rare relic or large Ember cache |

### 6.5 Shops
The merchant **Old Marrow** sells 5 items: 2 Tomes/Arms (level-up cards for Embers), 1 Relic, 1 Brew, 1 Roast. A **Reroll** costs 20 Embers (price +10 each use). Prices start at 60–220 Embers and grow with floor (×1 / ×1.5 / ×2). A *Thief's Gloves* steal spawns a **Bounty Hunter** elite on the next room clear.

### 6.6 Doors & Reward Previews (Hades-Style)
Every door shows a **reward icon** for the room behind it, once the player has *seen* that room's type on the minimap. Reward icons:
| Icon | Meaning |
|---|---|
| Patron sigil (one of 6) | A Boon from that Patron |
| Ember Stone | +1 level to a Boon |
| Anvil | An Arm upgrade or Reforge |
| Heart | Heal 30% |
| Coin pile | 100–200 Embers |
| Crown outline | Relic |
| Skull + flame | Elite / Gauntlet marker |
| Question mark | Hidden until entered (appears with the *Fog* Pact) |
Reward is **rolled at floor generation** and locked, so it is deterministic by seed. Each room offers exactly one reward; Boon choices (3 cards) are generated on pickup, not on door preview.

### 6.7 Mining Veins & Cracked Walls (Main-Game Tie-In)
- **Veins** are ore nodes embedded in walls and floors of Combat and Vein Cavern rooms. **Hold `E` for 1.5 s** nearby to mine (interrupted by damage). Results: Embers (60%), Soul Fragment (10%), a Brew (10%), Roast (10%), a **gemstone token** (10%; 1 token = 1 Rough gem credited to the main game at run end, see Section 12).
- **Cracked walls** hide Secret rooms. Mine them the same way (2.5 s).
- Veins skin by biome (copper in Gravebarrow, mossy iron in Mirewood, glowing wyrmite in Emberforge).

### 6.8 Room Interior Generation
Each room interior is assembled from **chunks** over a tile grid.
1. **Pick an archetype** (weighted by biome and room type):
   | Archetype | Description |
   |---|---|
   | **Open Hall** | Mostly empty with scattered cover |
   | **Pillared** | Regular columns forming lanes |
   | **Cross** | Four wings from a central plaza |
   | **Ring** | A central obstacle you can kite around |
   | **Moat** | A hazard channel with 2–3 bridges (bog water, lava) |
   | **Maze-lite** | Low walls creating 3–4 loops |
   | **Cathedral** | Aisles with altars and braziers |
   | **Shattered** | Random rubble clusters |
2. **Place a perimeter** of walls with doors at fixed midpoints on each connected side.
3. **Place chunks:** pick `N` chunks (6×6, 8×8 or 12×6 tiles) from `data/descent/chunks/<biome>/*.json` matching the archetype tags and difficulty. Chunks can be rotated and mirrored. They contain walls, pits, hazards, decor and **spawn anchors** (positions that prefer ranged or elite spawns).
4. **Validate:** flood fill from every door; all doors must connect; **walkable area ≥ 60%**; no isolated pocket larger than 12 tiles; no hazard directly adjacent to a door (2-tile buffer). If validation fails, retry up to 20 times, then fall back to an empty arena with only perimeter walls.
5. **Decorate:** scatter props (bones, mushrooms, anvils…), torches and ambient particles using the biome palette. Decor never blocks movement.
6. **Place content:** veins (0–3), chests, shop/shrine props as required by the room type.

**Chunk file example**
```json
{ "id": "grave_pillars_01", "size": [8, 8], "tags": ["pillared", "grave"], "difficulty": 1,
  "tiles": [
    "........",
    ".##..##.",
    ".#....#.",
    "........",
    "........",
    ".#....#.",
    ".##..##.",
    "........" ],
  "legend": { ".": "floor", "#": "wall", "^": "spike_trap", "~": "pit", "S": "spawn_anchor_ranged", "E": "spawn_anchor_elite" },
  "rotatable": true, "mirrorable": true }
```

### 6.9 Hazards (Player and Enemies)
| Hazard | Behavior |
|---|---|
| **Spike Trap** | Retracts and extrudes every 2 s; 15 damage (telegraphed by a 0.5 s tint) |
| **Crumbling Floor** | Collapses 1 s after being stepped on; falling deals 20 damage and respawns you at the nearest safe tile |
| **Poison Pool** | 8 damage/s and −25% move speed |
| **Spore Vent** | Periodic cloud (3 s, 4 damage/s) every 6 s, 1.5 s telegraph |
| **Lava River** | 30 damage and knockback to the bank; enemies avoid it |
| **Steam Vent** | Periodic jet 12 damage |
| **Falling Stalactite** | A shadow marks the landing zone for 1 s, 25 damage |
| **Coffin** | Opens and releases a Grunt when the player is within 180 px |
Hazards also affect enemies (e.g., knocked-back enemies can fall into pits or lava, an **instant kill** that grants no XP). Bosses are immune.

### 6.10 Minimap & Fog of War
A corner minimap shows discovered rooms as squares: visited (filled), seen-but-unvisited (outline), special icons for Boss, Treasure, Shop, Shrine, Fountain, Secret (after reveal). The player discovers adjacent rooms' types only after **clearing the room they are standing in** (not before). *Bone Compass* and *Miner's Lantern* relics reveal more.

### 6.11 Seeds & Determinism
- A run has one **run seed** (64-bit). Streams derived by hashing: `floorSeed(i) = H(runSeed, "floor", i)`, `roomSeed(i, x, y) = H(floorSeed(i), x, y)`, plus independent streams for `loot`, `spawns`, `combat`, `cosmetic` (cosmetic never affects gameplay).
- **Daily Descent** seeds come from the server (`GET /descent/daily`). Weekly Trial also fixes the modifier set.
- The run seed is shown in the summary and can be re-entered for **practice** (unscored).

---

## 7. Enemies & Spawning

### 7.1 Archetypes (Shared AI)
All enemies are built from archetypes; biomes skin and slightly modify them. Values below are **Biome 1 baselines** (HP/damage scale per Section 7.4).

| Archetype | HP | Contact / Attack | Speed (px/s) | Cost | Behavior |
|---|---|---|---|---|---|
| **Swarmer** | 8 | 4 contact | 90 | 1 | Seeks the player, spawns in groups of 6–12 |
| **Grunt** | 30 | 8 contact | 70 | 3 | Seeks, slight flanking offset |
| **Shooter** | 22 | 7 projectile every 2.0 s (0.4 s telegraph) | 60 | 5 | Keeps 220 px away, strafes |
| **Charger** | 45 | 14 contact | 60 → 260 charge | 8 | 0.8 s telegraph, then dashes 0.6 s in a straight line; stunned if it hits a wall |
| **Brute** | 160 (Armor 20) | 18 slam (AoE 80 px, 1.0 s telegraph) | 50 | 14 | Heavy knockback resist |
| **Splitter** | 35 | 6 contact | 75 | 6 | Splits into 2 Swarmers on death |
| **Exploder** | 18 | 20 AoE (70 px) after 0.6 s fuse | 110 | 4 | Rushes the player and detonates; also damages enemies |
| **Support** | 40 | none | 55 | 9 | Heals allies 10% HP/s or shields in a 160 px aura. Max 2 alive |
| **Spawner** | 70 | none | 40 | 12 | Spawns 1 Swarmer every 4 s (cap 6 alive); attackable; stationary |

### 7.2 Elites & Affixes
An **Elite** has ×4 HP, ×1.3 damage, a visible aura and **1 affix** (2 at Heat ≥ 6). Cost ×4. It drops a **Chest**, 1 **Soul Fragment**, and 3× Embers.

| Affix | Effect |
|---|---|
| **Hasty** | +40% move speed |
| **Armored** | +60 Armor |
| **Vampiric** | Heals 30% of damage it deals |
| **Volatile** | Explodes on death (100 px, 40 damage) |
| **Shielded** | Barrier equal to 50% HP; breaks to stagger it |
| **Splitting** | Spawns 4 Grunts at 50% HP |
| **Blinker** | Teleports behind the player every 5 s |
| **Cursed Aura** | 120 px aura: −20% your Attack Speed |

### 7.3 Spawning Director
The director controls what appears and when, using a **threat budget**.

**Room budget:** `T = 60 × (1 + 0.18 × roomsClearedInRun) × typeMult × (1 + 0.12 × Heat) × (1 + 0.15 × Haunted)`.
`typeMult`: Combat 1.0 · Elite 1.3 · Gauntlet 1.8 · Miniboss rooms use a fixed script (Section 8).

**Waves:**
- Combat/Elite rooms have **3 waves** taking 40% / 35% / 25% of the budget (Gauntlet: 5 waves with 25% / 20% / 20% / 20% / 15%).
- A wave **spawns all at once in groups** after a 1 s telegraph. The next wave starts when **≤ 20% of the current wave remains** or **14 s** have elapsed, whichever is first.
- **Trickle spawner:** while waves are active, if live enemies < 12% of the cap, spawn 1 Swarmer every 1.2 s (costs no budget; yields XP) so the player is never idle.
- **Stragglers:** 45 s after the final wave begins, remaining enemies glow and rush the player, so no one is left hiding behind walls.

**Composition:** the director picks archetypes with biome-weighted **roles**:
| Phase of the room | Preferred archetypes (weights) |
|---|---|
| Wave 1 | Swarmers 45 · Grunts 35 · Shooters 10 · Exploders 10 |
| Wave 2 | Grunts 30 · Shooters 20 · Chargers 15 · Splitters 15 · Swarmers 10 · Support 10 |
| Wave 3 | Brutes 25 · Chargers 20 · Shooters 20 · Spawners 10 · Support 10 · Grunts 15 |
Per-biome unlocks: Brutes and Spawners only appear from room 3 of floor 1; Support from floor 1 room 5.

**Groups / formations:** `Ring` (surround at radius), `Line` (a column), `Pincer` (two flanks), `Clump` (a tight cluster), `Doorway` (from a door). 20% of enemies come through doors; the rest spawn on rings.

**Spawn positions:** pick a point on a ring of radius 450–620 px around the player **inside room bounds, on walkable tiles, ≥ 360 px from the player**, not on hazards. A **rune circle** telegraph (0.6–1.0 s) marks each spawn; enemies are invulnerable and inactive during it. `spawn_anchor_*` tiles attract ranged/elite spawns.

**Caps & performance:** **max 250 live enemies** (hard cap 300 including spawns); if the cap is reached the director delays waves. Enemies far behind the camera (> 900 px) are *simulated at 15 Hz*. A wave's `budget` is spent greedily on the rolled archetypes.

### 7.4 Scaling
```
HP(enemy)  = baseHP × biomeHPMult × (1 + 0.04 × roomsClearedInRun) × (1 + 0.10 × Heat)
Damage     = baseDamage × biomeDmgMult × (1 + 0.02 × roomsClearedInRun)
Speed      = baseSpeed × (1 + 0.01 × Heat)  (cap +30%)
```
| Floor | `biomeHPMult` | `biomeDmgMult` |
|---|---|---|
| 1 | 1.0 | 1.0 |
| 2 | 2.4 | 1.6 |
| 3 | 5.0 | 2.4 |
| Finale | 9.0 | 3.4 |

### 7.5 Enemy Data Example
```json
{ "id": "bone_archer", "archetype": "shooter", "biome": "gravebarrow",
  "hp": 22, "armor": 0, "speed": 60, "cost": 5, "xp": 5,
  "attack": { "type": "projectile", "damage": 7, "cooldown": 2.0, "telegraph": 0.4,
              "projectileSpeed": 220, "range": 420 },
  "keepDistance": 220,
  "sprite": { "atlas": "grave_enemies", "frames": ["archer_0", "archer_1"], "fallback": "bone_archer_shape" },
  "drops": { "xpGem": 1.0, "ember": 0.2, "roast": 0.01 } }
```

---

## 8. Biomes & Biome Bosses

### 8.1 Gravebarrow Halls (Floor 1)
- **Theme:** ashen crypt under the old mines; palette grey stone, bone, copper green, torch orange. Music: slow chiptune dungeon loop.
- **Hazards:** Spike Traps, Crumbling Floor, Coffins.
- **Roster:** Swarmer **Bone Rats** · Grunt **Skeleton Thrall** · Shooter **Bone Archer** · Charger **Ghoul** · Brute **Crypt Guard** (frontal shield: hits from the front deal 25%) · Support **Grave Priest** · Splitter **Bone Pile** · Exploder **Powder Skull** · Spawner **Necro Urn**.
- **Mini-boss — Bone Captain Rattlewick (HP 1,400, Armor 10):** *Sword Combo* (3 hits, 0.5 s telegraph each), *Shield Dash* (charge, stun on wall), *Rally* (summons 6 Thralls every 25% HP lost).
- **Boss — Grave Warden Osric (HP 6,000, arena: round crypt):**
  - *Chain Sweep:* rotating chain (1.6 s sweep, 360°, damage 14); dash through it.
  - *Bone Volley:* 7-bullet fan every 3 s (damage 8, speed 180).
  - *Gravefall:* 3 circular markers (1.0 s) → falling tombstones (damage 22, 60 px radius).
  - **Phase 2 (< 50% HP):** coffins open (spawn Thralls), Chain Sweep reverses and speeds up, adds *Ghost Wave* (spectral skulls swooping in straight lines, 2 s telegraph).
  - **Rewards:** boss relic choice (1 of 3, including one class-tagged), full heal, Boon of choice, stairs.

### 8.2 Mirewood Hollows (Floor 2)
- **Theme:** bioluminescent swamp inside a forgotten drowned mine; palette teal, mud brown, spore violet. Music: slower, ambient chiptune with bubbling bass.
- **Hazards:** Poison Pools, Spore Vents, Thick Reeds (block projectiles and hide enemies, destructible).
- **Roster:** Swarmer **Leech Swarm** · Grunt **Bog Toad** (leaps 160 px, 0.5 s telegraph) · Shooter **Mire Wisp** (floats over hazards) · Charger **Mudhound** · Brute **Bog Brute** · Support **Marsh Shaman** · Splitter **Spore Blob** · Exploder **Puffcap** (poison cloud on death) · Spawner **Brood Mother**.
- **Mini-boss — Mire Matron (HP 3,400):** *Poison Spit* (arc, leaves a pool), *Leap Slam*, *Brood Call* (leeches every 20 s).
- **Boss — Ysolde, the Bloom Queen (HP 14,000, arena: flooded garden):**
  - *Thorn Lines:* vines erupt in 4 straight lines (1.0 s telegraph, damage 20).
  - *Spore Burst:* expanding ring cloud (damage 6/s) with a safe gap.
  - *Petal Blades:* 8 petals orbit then home in (shootable, HP 20).
  - **Phase 2 (< 50% HP):** the arena floods (poison edge pools grow 10%/15 s), she splits into **3 bloom clones** (only the real one takes damage; its petals are a different color).
  - **Rewards:** as Osric, plus 1 Ember Stone.

### 8.3 Emberforge Depths (Floor 3)
- **Theme:** the old wyrm forge; palette black stone, magma orange, steel. Music: faster, driving loop.
- **Hazards:** Lava Rivers, Steam Vents, Falling Stalactites.
- **Roster:** Swarmer **Cinder Imp** · Grunt **Slag Soldier** · Shooter **Ember Cultist** · Charger **Wyrmling** · Brute **Forge Golem** · Support **Flame Priest** · Splitter **Magma Slime** · Exploder **Bomb Imp** · Spawner **Molten Egg**.
- **Mini-boss — Wyrm Warden Kharn (HP 8,000):** *Fire Breath* (cone sweep 1.2 s), *Wing Slam* (shockwave), *Lava Pillars* (marked circles → eruptions), summons Wyrmlings at 66% and 33%.
- **Boss — Forgemaster Kaldur (HP 30,000, arena: forge floor with a central anvil):**
  - *Hammer Slam:* huge cone (telegraph 1.2 s, damage 45) leaving shockwave rings (jump with dash).
  - *Molten Waves:* horizontal lava waves with a 4-tile gap.
  - *Anvil Drop:* falling anvils at your position (shadow for 0.8 s, damage 35).
  - **Phase 2 (< 50% HP):** *Overheat:* Kaldur glows; lava slowly rises from the arena edges; adds *Slag Golems* that must be pulled from the anvils; attacks speed up 25%.
  - **Rewards:** boss relic choice, Ember Stone ×2, full heal, access to the Crowned Hollow.

### 8.4 The Crowned Hollow (Finale)
A void-dream version of the Aether Vault: black glass, cyan and magenta light, floating crown fragments. Two **gauntlet rooms** (5 waves each, Hollow Knights, Shade Wisps, Void Leeches; HP mult 9.0) lead to the **Throne** (Section 9). Between the two gauntlets is a single **Patron's Respite** room: a free full heal, a Fountain, and a choice of **one Legendary-eligible Boon upgrade** (Ember Stone ×3).

---

## 9. Final Boss — Vaelor, the Crowned Hunger

### 9.1 Overview
| | |
|---|---|
| **Arena** | Circular throne room, radius 22 tiles (704 px); a ring of void edge |
| **HP** | 90,000 (phase thresholds 100→60 → 25 → 0%) |
| **Expected duration** | 3–4 min |
| **Look (pixel)** | A towering robed skeleton-like figure of void and bone, 320 px tall, a cracked golden crown floating above a gaping hollow face; trailing ribbons; crown shards orbit him in phase 3 |
| **Music** | Unique boss track (menacing, 2 layers; the second layer fades in at phase 2) |
| **Fairness rules** | Every attack is telegraphed ≥ 0.6 s; the arena always has a safe path; no instant kills; lethal damage is blockable by dash i-frames |

### 9.2 Phase 1 — The Crowned Hunger (100% → 60%)
- **Crown Spiral:** 3 rotating arms fire bullets (speed 160, damage 12) for 6 s; arms reverse direction every 2 s.
- **Maw Marks:** 5 circles (radius 70 px) mark the player's position in sequence, 1.0 s telegraph each, damage 30.
- **Hollow Procession:** spawns 14 **Hollow Knights** (Brutes, budget 45) from the arena edge. Each knight that reaches Vaelor **heals him 1%** — kill them before they arrive.
- **Throne Slam:** Vaelor leaps (shadow telegraph 1.0 s), creating 2 expanding shockwave rings (damage 35); **dash through the rings** to avoid them.

### 9.3 Phase 2 — The Devouring (60% → 25%)
- **Void Tide:** the arena edge is consumed; the safe radius shrinks 12% every 15 s to a minimum of 12 tiles. Standing in the void deals 15 damage/s.
- **Crown Lances:** 6 beams rotate from Vaelor, 0.8 s telegraph then a 2-second sweep (damage 25).
- **Homing Voids:** 4 slow orbs (HP 40) that explode in a 90 px radius (damage 28); shootable.
- **Hunger Tendrils:** ground lines erupt (damage 22) targeted around the player.
- **Devour:** pulls the player toward the center for 2 s (a 1 s warning; escape with a dash or a Boulder Charge); contact in the maw deals 50 damage.

### 9.4 Phase 3 — The Starved King (25% → 0%)
- The crown **shatters into 6 Crown Shards** (HP 1,500 each) orbiting Vaelor; while any shard lives, Vaelor takes **−50% damage**. Each shard fires a different pattern: spiral, aimed volley, homing orb, line laser, ring burst, tentacle.
- **Bullet Curtains:** horizontal and vertical walls of bullets with a 3-tile safe gap.
- **Final Hunger (< 10% HP):** a **20-second Starvation timer** begins; the void closes to a 10-tile circle and the arena pulses (damage 40 every 2 s) when the timer expires. Kill him before it runs out.
- **Heat 10+ only — Phase 4, The Hollow Crown:** Vaelor re-forms (35% HP) with all prior attacks at +25% speed and a final desperation pattern.

### 9.5 Rewards and the Permanent "Crown" Reward
**Every victory:**
- **Soul Shards** bonus (+150, scaled by Heat), a boss relic, an unlock of the next Heat tier.
- **Crown Shards** (meta currency): `5 + floor(Heat / 2)`.
- **Spoils** credited to the main game (Section 12).

**First victory with any class — "The Hunger's Crown" (permanent, mode-wide):**
1. **Crown Slot:** a permanent additional start-of-run slot. Before each run you choose **one Crown Boon** (powerful starting effect, Section 10.5).
2. **Heat system expansion:** unlocks the full Pact list and Heat ≥ 4.
3. **Endless Loop mode:** after beating Vaelor you may continue into a Loop (Section 13.4).
4. **Cosmetics & title:** the *Crownbreaker* title, a golden aura on your hero in the Hub and a pickaxe trail (Section 12).

**First victory with each additional class:** unlocks that class's **Duelist/Warden/Trickster/Chronomancer/Beastmaster Aspect** (Section 4.8) and **+10 Crown Shards**.
**Repeat victories at higher Heat:** bonus Crown Shards and cosmetic **Crown Marks** (one per Heat tier per class) displayed on the class portrait.

---
## 10. Meta Progression (Between Runs)

### 10.1 Currencies
| Currency | Earned by | Spent on |
|---|---|---|
| **Soul Shards** | Every run (win or lose), Section 13.3 | **Ember Anvil** nodes |
| **Crown Shards** | Victories only (Section 9.5) | **Crown Altar** (Crown Boons), **Descent Merchant** (Section 12.3) |
| **Embers** | In-run only | Shops, shrines, Anvil (resets at run end) |
| **Main-game resources** | The main game | **Tribute** on Anvil nodes, **Provisions**, Sigil swaps (Section 11) |

### 10.2 The Ember Anvil (Permanent Upgrade Tree)
A skill-tree screen in the Hub with four branches. Each rank costs **Soul Shards** *and* a **Tribute** of main-game ingots/dust, so mining and crafting directly power the mode.

**Cost formulas** (per rank `r`, starting at 1):
```
soulCost(r) = round(baseSoul × 1.45^(r−1))
tribute(r)  = ceil(baseTribute × r) × ingots of tier t,  t = min(10, nodeTier + floor((r−1) × tierStep))
```
Nodes can also require a campaign boss (see "Gate").

| Branch | Node | Effect per rank | Max rank | baseSoul | baseTribute | nodeTier / tierStep | Gate |
|---|---|---|---|---|---|---|---|
| **Vitality** | Tough Hide | +4 Max HP | 10 | 20 | 2 | 1 / 0.6 | — |
| | Second Wind | +0.1 HP/s Regen | 5 | 60 | 3 | 3 / 0.8 | Boss 3 |
| | Iron Will | +1 Armor | 4 | 80 | 3 | 3 / 1.0 | Boss 5 |
| | Phoenix Spark | +1 Revive (25% HP) | 1 | 800 | 12 + 3 Boss Essence | 7 / — | Boss 8 |
| **Might** | Keen Edge | +2% Might | 10 | 25 | 2 | 1 / 0.6 | — |
| | Swift Hands | +2% Attack Speed | 8 | 40 | 2 | 2 / 0.7 | Boss 2 |
| | Wide Arc | +2% Area | 8 | 40 | 2 | 2 / 0.7 | Boss 2 |
| | Sharpened Instincts | +1% Crit Chance | 10 | 35 | 2 | 2 / 0.6 | Boss 4 |
| **Fortune** | Lucky Charm | +1 Luck | 5 | 50 | 3 | 2 / 0.8 | — |
| | Scavenger | +3% Greed (Embers) | 10 | 20 | 2 | 1 / 0.6 | — |
| | Quick Study | +3% Growth (XP) | 10 | 25 | 2 | 1 / 0.6 | — |
| | Second Thoughts | +1 Reroll (rank 1–3) | 3 | 120 | 4 | 3 / 1.0 | Boss 4 |
| | Banisher | +1 Banish (rank 1–3) | 3 | 120 | 4 | 3 / 1.0 | Boss 4 |
| **Arcana** | Patron's Favor | +3% rarer Boon odds | 5 | 70 | 3 | 3 / 0.8 | Boss 6 |
| | Ember Savant | Start with +50 Embers | 5 | 30 | 2 | 1 / 0.6 | — |
| | Open Mind | +10% chance of a 4th level-up card | 5 | 60 | 3 | 3 / 0.8 | Boss 6 |
| | Relic Whisper | +1 relic choice in Treasure rooms | 1 | 400 | 8 | 5 / — | Boss 7 |
**Respec:** free at any time (refunds Soul Shards; tribute is **not** refunded) so experimentation never punishes the player. A completed tree grants about +40 Max HP, +20% Might, +4 Armor and strong QoL, roughly a 30–40% power increase.

### 10.3 Class Mastery
Each class has **Mastery levels 1–10**, gained from **Mastery XP** per run: `rooms × 10 + bosses × 50 + (victory ? 200 : 0)`. Level `L` requires `100 × L^1.5` XP.

| Level | Reward |
|---|---|
| 1 | Alternate palette (cosmetic) |
| 3 | Start each run with 1 free **Tome** at level 1 |
| 5 | Unlocks the **Mastery Aspect** (Section 4.8) |
| 7 | Start with 1 free **Arm** of your class tags at level 2 |
| 9 | +1 Reroll for that class |
| 10 | Title *Master of the &lt;class&gt;* and a Brew at start |

### 10.4 Heat & Pacts (Difficulty Dial)
Before a run, the player can accept **Pacts** (voluntary difficulty modifiers). Their combined **Heat** multiplies rewards.

| Pact | Effect per rank | Max rank | Heat / rank |
|---|---|---|---|
| **Hardened Foes** | +10% enemy HP | 5 | 1 |
| **Swift Foes** | +5% enemy speed | 4 | 1 |
| **Dense Horde** | +10% wave budget | 5 | 1 |
| **Elite Surge** | +10% of spawns become Elites | 3 | 1 |
| **Fragile** | −10% Max HP | 3 | 1 |
| **Scarcity** | −10% Embers; shop prices +10% | 3 | 1 |
| **Merciless** | Bosses deal +10% damage | 3 | 1 |
| **Hurry** | Stragglers rush at 150 s / 120 s / 90 s | 3 | 1 |
| **Fog of War** | Door previews are hidden | 1 | 2 |
| **No Mercy** | No Roast; Fountains heal 25% | 1 | 2 |
| **Haunted Relics** | Each relic found carries a minor Curse | 1 | 2 |
| **Vaelor's Gaze** | Final boss +25% HP and an extra pattern | 1 | 2 |
- **Heat** = sum of Heat across selected ranks. **Soul Shard bonus: +8% per Heat.**
- **Heat 1–3** are available from the first run. **Heat ≥ 4** unlocks after the first victory (The Hunger's Crown). **Phase 4 of Vaelor** appears at Heat ≥ 10.
- Each class tracks its highest victorious Heat; **Crown Marks** are awarded per class at Heat 1, 5, 10, 15, 20.

### 10.5 The Crown Altar & Crown Boons
After the first victory the **Crown Slot** unlocks. Crown Boons are bought with **Crown Shards** and **one is chosen before each run**.

| Crown Boon | Effect | Cost |
|---|---|---|
| **Crown of Embers** | Start with 2 Ember Stones (apply to boons) and +100 Embers | 10 |
| **Crown of Thorns** | Reflect 20% of damage taken; +1 Armor | 10 |
| **Crown of the Delver** | Veins yield ×2; +20% Pickup Radius; start with a random Brew | 10 |
| **Crown of Patronage** | Begin with a Rare Boon from a random unlocked Patron | 12 |
| **Crown of Echoes** | Start with a free Arm (of your choice) at level 2 | 15 |
| **Crown of Fortune** | +10 Luck, +1 Reroll | 15 |
| **Crown of Hunger** | Kills heal 0.5 HP; −10% Max HP | 20 |
| **Crown of Ruin** | The first attack of each room deals ×5 damage | 25 *(requires a Heat 5 victory)* |
| **Crown of Eternity** | +1 Revive (40% HP) | 40 *(requires a Heat 10 victory)* |

### 10.6 Codex
Records every enemy, Boon, relic, Arm and Tome discovered. Completing categories awards **cosmetics** and Soul Shards (not power). The Codex shows unlock conditions for undiscovered entries as hints.

---

## 11. Preparations & Main-Game Links (Pre-Run Upgrades)

### 11.1 Preparations Screen
Opened by **Begin Run**. Left to right:
1. **Class & Aspect** selection.
2. **Crown Boon** (after first victory).
3. **Soul Sigil** (gems, 11.2).
4. **Provisions** (consumable per-run boosts, 11.3).
5. **Pacts** (heat, 10.4).
6. **Summary** of the stat modifiers from every source with a clear total (e.g. "Max HP 120 → 171 · Might +14% · Luck +3") and the expected **Soul Shard multiplier**.
**Start Run** deducts Provisions' cost. A run can be started with no preparation at all.

### 11.2 Soul Sigil (Gems from the Main Game)
A **Soul Sigil** plate has **3 sockets** (+1 at main level 40). Gems from the main game's **Gem Hollow** (Section 6 of the main spec) are placed here. **Gems are not consumed**; they are borrowed from the main game's inventory (a gem socketed in a main-game item cannot be used simultaneously). Cut multiplies the effect: Rough ×1, Cut ×2, Flawless ×4.

| Gem | Effect per socket (Rough) |
|---|---|
| Amber | +1 Luck |
| Garnet | +2% Might |
| Jade | +3% Max HP |
| Moonstone | +1% Crit Chance |
| Sapphire | +2% Cooldown Reduction |
| Bloodstone | +0.3% lifesteal |
| Star Ruby | +2% Attack Speed |
| Arcane Opal | +3% Growth |
| Dragon's Eye | +1 projectile pierce, +3% Crit Damage |
| Aether Diamond | +1% to all of the above stats |
**Cap:** the Sigil contributes at most **+25%** to any single stat.

### 11.3 Provisions (Consumable Main-Game Spend)
**Provision slots:** 2 (3 at main level 30, 4 at main level 50). Costs use the player's **highest unlocked material tier `T`** so they stay relevant: `gold = base × 3^(T−1)` and ingots of tier `max(1, T−1)`.

| Provision | Effect | Gold (base) | Other cost |
|---|---|---|---|
| **Hearty Ration** | +25 Max HP for the run | 40 | 3 ingots |
| **Whetstone** | +10% Might | 60 | 2 ingots |
| **Lucky Coin** | +1 Reroll and +1 Banish | 50 | 5 Arcane Dust |
| **Scout's Map** | Reveals floor 1's room types | 30 | — |
| **Merchant's Purse** | +150 Embers | 80 | — |
| **Rune of Choice** | The first Boon offer shows 5 cards | 100 | 15 Arcane Dust |
| **Patron's Token** | The first Boon offer is from the Patron of your choice | 120 | 5 dungeon Trophies (tier `T−1`) |
| **Delver's Pick** | Veins yield +50% | 60 | 4 ore (tier `T−1`) |
| **Brewer's Kit** | Start with 2 random Brews | 50 | 8 Arcane Dust |
| **Phoenix Feather** | +1 Revive (30% HP) | 150 | 10 Arcane Dust + 1 Boss Essence |
Provisions are **consumed on Start Run**. Cancelling the run before the first room refunds them.

### 11.4 Main-Game Progress That Directly Helps
| Main-game progress | Effect in the Descent |
|---|---|
| **Player level** | **Veteran's Bonus:** for each 5 levels above 20: +1% Max HP and +1% Might (cap +8% each at level 60) |
| **Campaign bosses cleared** | Unlocks Patrons (Section 5.4), classes (Section 4.1) and some Anvil nodes (Section 10.2) |
| **Campaign stars** (max 30) | +1% Growth per 3 stars (cap +10%) |
| **Owned boss legendaries** (own, not necessarily equipped) | Unlocks the themed **Echo relic** in the mode's relic pool: see below |
| **Materials / ingots / dust / trophies / Boss Essence** | Paid as Tribute and Provisions |
| **Gem Hollow gems** | Soul Sigil |
| **Ascension count** | +2% Soul Shards per Ascension (cap +20%) |

**Echo relics** (Legendary rarity, appear in Treasure rooms and boss rewards):
| Main-game legendary | Echo relic in the Descent |
|---|---|
| Burrowking's Crown | Every 10th hit deals ×3 damage |
| Bonecleaver Gauntlets | Consecutive crits stack +5% Crit Damage (max 10) |
| Cinderheart Amulet | 8% chance per hit to ignite the target |
| Hydrascale Cuirass | Heal 3% Max HP at the start of each room |
| Sunforged Pickaxe | 10% chance for kills to drop ×5 Embers |
| Runewarden's Diadem | +30% Growth; +10% Rare+ Boon odds |
| Starfall Treads | +15% Move Speed; dash charges refill when a room starts |
| Wyrmfang Breaker | Crits deal +100% damage to enemies above 50% HP |
| Voidglass Phylactery | Heal 5% of damage dealt (max 3 HP/s) |
| Aetherbrand | 15% chance for hits to strike again at 60% damage |

### 11.5 Balance Guard-Rails
- **Gear never enters the mode.** Only the channels above do.
- Total pre-run power from main-game links is **capped at roughly +45%** vs. a no-prep run (Provisions + Sigil + Veteran + Anvil) so that skill and Pacts matter.
- A new player with *no* preparation must still be able to win with a strong build at Heat 0 (target win rate 3–6%, Section 18).

---

## 12. Rewards Back to the Main Game

The mode returns **bounded** rewards so the two modes feed each other without making mining obsolete.

### 12.1 Spoils of the Descent
At the end of every run (win or lose) the player receives **Spoils**, credited to the main game's inventory:
```
ore    = floor(rooms × 4) of material tier T−1 (the layer below the highest unlocked)
dust   = floor(rooms × 1.5) Arcane Dust
gems   = veinTokens (max 3) → Rough gems (random, using Gem Hollow weights)
essence= 1 Boss Essence on the first victory of each week
```
**Daily cap:** spoils are paid for the first **4 runs per day**; further runs convert the spoils to Soul Shards at 1:1 ore and 3:1 dust. A victory counts as 1.5 runs toward the cap. (Prevents farming that would bypass mining.)

### 12.2 Descent Rank & Cosmetics
`Descent Rank = (class × heat-tier victory marks) + floor(lifetimeRoomsCleared / 250)` (cap 40).
Rank rewards: titles, hero frames, **pickaxe trail effects** for the main game's mining screen (cosmetic), and small **capped** boons:
- every **5 ranks:** +1 Gem Hollow swing (cap +3 total),
- every **10 ranks:** +1% Mining Power (cap +3%).
The *Crownbreaker* title and the golden mining aura are awarded for the first victory.

### 12.3 Descent Merchant (Crown Shards Shop)
A limited, weekly-refreshing shop:
| Item | Cost | Weekly limit |
|---|---|---|
| Arcane Dust bundle (50) | 8 | 5 |
| Rough Gem chest (3 gems) | 15 | 2 |
| Boss Essence ×1 | 30 | 1 |
| Cosmetic pickaxe trails, hero frames, hub decor | 20–60 | — |

---

## 13. Run Lifecycle & Scoring

### 13.1 Start
`Start Run` → apply Preparations → generate the floor from the run seed → spawn in the Start room with the class's starting Arm. The first Boon is a **free Patron choice** after clearing room 1 (offers one card each from three different unlocked Patrons).

### 13.2 End States
| State | Result |
|---|---|
| **Death** | Run Summary, 100% Soul Shards, Spoils, Mastery XP |
| **Victory** (Vaelor defeated) | Run Summary, bonuses (Section 9.5), option to enter the Endless Loop |
| **Abandon** | 50% Soul Shards, no Spoils, no Mastery XP |
| **Suspend** | Saved at the **start of the current room** (rooms are never saved mid-fight). A suspended run lasts until resumed or a new run is started (then it is abandoned) |

### 13.3 Soul Shards & Score
```
SoulShards = floor( (rooms×3 + minibosses×15 + bosses×30 + soulFragments×5 + runLevel×2 + (victory ? 150 : 0))
                    × (1 + 0.08×Heat) × (1 + 0.02×min(10, ascensions)) )
Score      = floor( (rooms×100 + kills×2 + bossKills×1000 + (victory ? 10000 : 0)
                     + (victory ? max(0, 2400 − seconds) × 3 : 0)) × (1 + 0.10×Heat) )
```

### 13.4 Endless Loop
After defeating Vaelor, the player may **continue** (only after the first victory):
- The floors repeat in order with the **Loop multiplier** `L`: enemy HP ×`(1 + 0.6L)`, damage ×`(1 + 0.25L)`, +1 elite affix every 2 loops, Boon rarity weights shift +3% per loop.
- Each loop is 3 floors + a boss (rotating among the three biome bosses with new attacks, then Vaelor every 3rd loop at +50% HP).
- The player keeps their build and fully heals at each loop start.
- Reward: Soul Shards ×(1 + 0.25L) per loop and **Crown Shards** (+2 per loop). **Best Loop** is saved and ranked.

### 13.5 Daily & Weekly
- **Daily Descent:** server-issued seed and a fixed **Daily Pact set** (Heat 3–6); any class; **one scored attempt** per day (the first run started that day); unlimited practice runs on the same seed.
- **Weekly Trial:** a seed plus fixed class, fixed Pacts and a special **Weekly Relic** (new each week). Rewards: Soul Shards, cosmetics, Crown Shards for completion (+5).

### 13.6 Run Summary Screen
Shows: result, time, rooms, kills, damage dealt (top 3 sources), damage taken (top 3), final build (Arms, Tomes, Boons, Relics), shard breakdown, spoils, mastery progress, unlocks, and a **Share** button (copies the run seed and a text card).

---

## 14. Online Integration (Uses the Main Spec's Backend)

### 14.1 Leaderboards (Main spec Section 18)
| Board | Metric | Window |
|---|---|---|
| `descent_daily` | Score on the daily seed | Daily |
| `descent_weekly` | Score on the weekly trial | Weekly |
| `descent_loop` | Best Endless Loop reached (and rooms) | All-time |
| `descent_heat` | Highest Heat victory (per class selectable) | All-time |
| `descent_speed` | Fastest victory time (Heat ≥ 5) | Seasonal |
Boards support Top 100, Around Me, and Guild scope as in the main spec. Entries show class, Heat and a **build snapshot** (Arms, Boons, Relics) viewable by other players.

### 14.2 Run Submission & Validation
`POST /descent/runs` (idempotent) with: `{runSeed, mode, class, aspect, heat, pacts, result, score, durationMs, rooms:[{id, type, ms, kills, hpEnd}], build:{...}, clientVersion, summaryHash}`.
The server **recomputes everything derivable from the seed**: floor layouts, room types, budgets and the maximum possible kills/XP per room, then checks the summary:
- the seed is the issued daily/weekly seed and the account's first attempt,
- per-room time ≥ a minimum (e.g., ≥ 6 s for the first wave to spawn and telegraph),
- kills ≤ the room's maximum spawnable enemy count, damage/kill ratios plausible, level and build consistent with picks offered by the seed,
- Soul Shards and Spoils are recomputed server-side (the client's numbers are advisory).
Anomaly flags follow the main spec's anti-cheat flow (soft-hide from public boards). Moderators can re-run the stored `rooms[]` checkpoints.

**Replays (stretch):** record inputs per tick (`move vector, aim angle, button bits`, ~1–2 KB/min compressed). A headless server build of the same `core` simulation can verify the top-N and a random audit sample. **Caveat:** the sim uses floats; cross-compiler/OS determinism is *not* guaranteed unless the build uses strict flags (`-ffp-contract=off`, no `-ffast-math`, own `sin/cos` tables). Treat replay verification as best-effort and use it only on the same server-built binary lineage.

### 14.3 Endpoints
`GET /descent/daily` · `GET /descent/weekly` · `POST /descent/runs` · `GET /descent/runs/{id}` (public build snapshot) · `POST /descent/replays` *(stretch)* · leaderboard routes reuse `GET /leaderboards/{board}`. All additive to the main spec's API and use the same auth, rate limits and idempotency rules.

### 14.4 Database (Added)
```sql
CREATE TABLE descent_seeds (day DATE PRIMARY KEY, daily_seed BIGINT NOT NULL, weekly_seed BIGINT, weekly_modifiers JSONB);
CREATE TABLE descent_runs (
  id BIGSERIAL PRIMARY KEY, account_id BIGINT REFERENCES accounts ON DELETE CASCADE,
  mode TEXT NOT NULL, seed BIGINT NOT NULL, class TEXT NOT NULL, aspect TEXT, heat INT NOT NULL DEFAULT 0,
  result TEXT NOT NULL, score BIGINT NOT NULL, rooms INT NOT NULL, duration_ms INT NOT NULL,
  summary JSONB NOT NULL, replay BYTEA, flagged BOOLEAN NOT NULL DEFAULT FALSE, created_at TIMESTAMPTZ NOT NULL DEFAULT now());
CREATE INDEX ON descent_runs (account_id, created_at DESC);
CREATE INDEX ON descent_runs (mode, seed, score DESC);
```
Leaderboard rows use the main spec's `leaderboard_scores` table with the board ids from 14.1.

### 14.5 Guild Tie-In
- **Guild Descent Goal (weekly):** the guild collectively clears `N` rooms and defeats biome bosses `M` times (members' submitted runs count). Rewards: guild XP, Treasury Arcane Dust, and a guild banner variant. No effect on the mode's combat balance.
- Guild chat shows **run share cards**. Guild leaderboard scope works for all `descent_*` boards.

---

## 15. UI

### 15.1 In-Run HUD
```
┌────────────────────────────────────────────────────────────────────────┐
│ XP ▓▓▓▓▓▓▓▓░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░  Lv 12          │
│ ♥ HP ▓▓▓▓▓▓░░ 84/120  ⛨ Barrier ▓▓                [Wave 2/3]  ⌖ Map  │
│                                                                        │
│                          (game view)                                   │
│                                                                        │
│  Arms: [⚔][✦][❄][ ]   Tomes: [H][Q][R][ ]      ◉ Dash ◉◉   Q ⌛ R ▓▓▓  │
│  Boons: A S D U I    Relics: ⚙⚙⚙              Embers 142               │
│                                   (boss HP bar here during boss fights)│
└────────────────────────────────────────────────────────────────────────┘
```
- XP bar spans the top; HP, barrier and level at top-left; **minimap** and Embers at top-right.
- Arm/Tome slots show level pips; Boon slots (Attack/Skill/Dash/Aura/Utility) show Patron colors and rarity borders.
- Dash charges, Skill (Q) cooldown radial and Ultimate (R) Wrath meter are bottom-center.
- Damage numbers are aggregated per entity per 0.2 s to avoid clutter; crits are larger.
- **Telegraphs** are always red-orange filled shapes with a dashed outline that **shrinks/fills** as the timer ends, drawn **under** sprites and above the floor.
- Boss HP bar with phase notches at the bottom center, with the boss name.

### 15.2 Screens
| Screen | Notes |
|---|---|
| **Hub** | Section 2.2 |
| **Preparations** | Section 11.1 |
| **Ember Anvil** | Tree view with cost and tribute tooltips; respec button |
| **Level-Up Modal** | 3–4 cards, reroll/banish/skip buttons, shows owned stack levels |
| **Boon Choice** | 3 large Patron cards with portrait, rarity border, slot icon, level preview |
| **Shop / Shrine** | In-room interactions with `E`, same card UI |
| **Pause** | Resume, Settings, Controls, Codex, Suspend (between rooms only), Abandon |
| **Run Summary** | Section 13.6 |
| **Codex** | Tabs: Enemies, Boons, Relics, Arms, Tomes, Bosses |
| **Leaderboards** | Reuses the main spec's board screen with the Descent boards |

### 15.3 Accessibility
Auto-aim for all classes (Settings), aim-assist strength, reduced screen shake/flash/hit-stop toggles, **telegraph patterns** (hatching in addition to color) for color-blind users, enemy projectile outlines with a distinct shape per faction, scalable damage numbers, **Assist Mode** (adjustable damage taken, extra Max HP, slower enemies; flags the run as *Assisted*: no leaderboard score, no Daily attempt). Fully rebindable controls; gamepad support after Phase D10.

---

## 16. Visual & Audio Style

- **Art:** top-down pixel art (16 px native, drawn ×2). Class silhouettes are distinct: Warrior red tabard, Tank steel plate, Archer green hood, Mage violet robe, Summoner teal cloak with floating wisps. Enemies are readable at 24–32 px with 1-px dark outlines; elites have a pulsing aura ring.
- **Shape fallback:** every sprite has a procedural counterpart in `gfx::descent::*` (circles, polygons, rectangles with outlines) so the mode is fully playable with zero assets.
- **Colors:** player projectiles use class colors with a white core; **enemy projectiles are always red/orange with a white core**; pickups: XP blue/green/purple, Embers gold, Roast brown, Ember Stone orange.
- **Juice:** white hit flash (2 frames), knockback squash, damage numbers, kill burst, XP gem sparkles, level-up glow, screen shake and hit-stop scaled to hit size (toggleable), floor-specific ambient particles (dust, spores, embers).
- **Audio:** per-biome loops (Gravebarrow slow crypt, Mirewood ambient swamp, Emberforge driving), a boss theme per boss, Vaelor's two-layer track. SFX: attack variants per class, hit, crit, kill, pickup (pitch rises with chain), level-up, boon pick (Patron-specific chime), shop, door lock/unlock, telegraph tick, dash, ultimate. Use the main spec's procedural SFX generators as the fallback.
- **Free assets (optional, see main spec Section 21):** Kenney *Tiny Dungeon* and *Tiny Creatures* (16×16, CC0) for tiles and enemy placeholders, a CC0 dungeon tileset such as 0x72's *DungeonTileset II* (verify the license on the page), game-icons.net for Boon/relic icons (CC-BY 3.0, attribution required), Kenney RPG/Impact audio (CC0), and the CC0 retro music collection on OpenGameArt. Every file must pass the manifest/license check before shipping.

---

## 17. Technical Design (C++)

### 17.1 Module Layout
```
core/descent/        # pure rules: run state, stats, effects, director, floorgen, rooms, loot, meta, scoring
  include/ed/descent/  src/
client/src/descent/  # screens, HUD, rendering, input, audio hooks, particles
server/src/descent/  # validation, seeds, leaderboards, submissions
data/descent/        # classes, arms, tomes, boons, relics, enemies, biomes, chunks, pacts, anvil, provisions
tools/descent_sim/   # headless bot runner for balance
```

### 17.2 Simulation Architecture
- **Fixed 60 Hz** simulation (`dt = 1/60`), rendering interpolated. Single-threaded sim; loading, saving and network on worker threads.
- **ECS:** use **EnTT** (via vcpkg) *or* a hand-rolled SoA pool; components: `Transform`, `Velocity`, `CircleCollider`, `Health`, `Faction`, `EnemyAI`, `Projectile`, `StatusSet`, `Pickup`, `Minion`, `Sprite`, `Lifetime`.
- **System order per tick:** Input → PlayerControl → Director → Flow-field update → EnemyAI → Steering/Movement → Spatial hash rebuild → Collision → Arms/Skills firing → Projectiles → Trigger/Effect processing → Damage resolution (event queue) → Status ticks → Pickups → Room state machine → Cleanup → UI events.
- **Spatial hash:** 64 px cells; circle-circle tests; enemy separation uses ≤ 8 neighbours.
- **Pathfinding:** a **flow field** (Dijkstra on the tile grid from the player) rebuilt every 0.25 s and when the player changes tile; flying enemies ignore it; ranged enemies use it plus a kite offset.
- **Memory:** **fixed pools** (enemies 320, projectiles 1,200, pickups 600, particles 3,000, damage numbers 120). No allocations in the tick.
- **Rendering:** atlas-batched sprites (raylib `rlgl`), y-sorting, camera culling; particle and text pooling. Tile maps drawn to a render texture per room and redrawn only when tiles change.
- **Budget:** sim ≤ 4 ms and render ≤ 8 ms per frame at the performance targets (300 enemies, 800 projectiles).

### 17.3 Effect System (Boons, Relics, Pacts, Tomes)
Boons and relics are **data**, built from `trigger → conditions → actions`.
- **Triggers:** `OnAttack, OnHit, OnCrit, OnKill, OnDash, OnSkill, OnUltimate, OnDamageTaken, OnRoomStart, OnRoomClear, OnTick(interval), OnPickup, OnLevelUp`.
- **Conditions:** `chance`, `everyN`, `internalCooldown`, `hasStatus`, `hpBelow/Above`, `targetTag`, `statAtLeast`.
- **Actions:** `ModStat(add|mul, duration?)`, `ApplyStatus`, `SpawnProjectile`, `SpawnAoE`, `Heal`, `GrantBarrier`, `SpawnMinion`, `DropPickup`, `RepeatAttack`, `Knockback`, `SetFlag`.
- **Scaling:** `value = baseValue × (1 + 0.4 × (level − 1)) × rarityMult`.
```json
{ "id": "ignis_searing_edge", "patron": "ignis", "slot": "attack", "tags": ["fire", "dot"],
  "rarity": { "common": 1.0, "rare": 1.5, "epic": 2.0 },
  "effects": [ { "trigger": "OnHit",
      "actions": [ { "type": "ApplyStatus", "status": "burn", "dps": 0.30, "duration": 3.0, "maxStacks": 3 } ] } ] }

{ "id": "relic_overcharged_core", "rarity": "epic",
  "effects": [ { "trigger": "OnTick", "interval": 8.0,
      "actions": [ { "type": "SetFlag", "flag": "next_attack_x4" } ] } ] }
```
Every effect has a **unit test** in a headless harness (spawn dummy enemy, fire trigger, assert result). Duo/Legendary Boons list required Boon ids and levels in `requires`.

### 17.4 Determinism
- Streams: `floor`, `rooms`, `spawns`, `loot`, `combat` and `cosmetic`, each seeded from the run seed (6.11). Cosmetic never feeds back into the sim.
- No gameplay dependence on container iteration order; entity ids are stable and monotonic. Use a stable sort for ties.
- Floats are allowed in the sim but the build must set strict floating-point flags (no fast-math, no FMA contraction) and use shared `sin/cos` tables to reduce drift (needed only if replays are verified).

### 17.5 Data & Validation
All JSON in `data/descent/` is validated at startup (schema + cross-references: every boon prerequisite exists, every enemy sprite or fallback shape exists, every chunk is rectangular and has door-clear borders). A `data_lint` tool runs in CI.

### 17.6 Save Additions
```cpp
struct DescentSave {
  int schema{1};
  double soulShards{}, crownShards{};
  std::map<std::string,int> anvilRanks;                 // node id -> rank
  struct ClassProg { bool unlocked{}; int masteryXp{}, victories{}; int bestHeatWin{-1}; std::set<std::string> aspects; std::set<int> crownMarks; };
  std::map<std::string, ClassProg> classes;
  bool crownSlot{}, firstVictory{};
  std::set<std::string> crownBoons;                      // owned
  std::string selectedCrownBoon, selectedClass, selectedAspect;
  std::map<std::string,int> pactSelection;               // pact id -> rank chosen
  std::vector<std::optional<GemInstance>> sigilGems;     // references to main-game gems
  std::set<std::string> codex;                           // discovered ids
  int descentRank{}, bestLoop{}; int64_t lifetimeRooms{}, lifetimeKills{};
  std::string spoilsDay; int spoilsRunsToday{};          // daily cap tracking (UTC day key)
  std::optional<RunSnapshot> suspended;                  // room-boundary snapshot
};
```
`SaveState` gains `DescentSave descent;` and bumps `schemaVersion` with a migration that creates defaults. The cloud sync treats Soul/Crown Shards and ranks as **non-mergeable** (conflict dialog as before). Server-issued rewards (daily/weekly) arrive through the **Inbox**.

### 17.7 Tooling & Tests
- **In-game dev console:** `seed`, `spawn <enemy>`, `give boon/relic/arm`, `room <type>`, `god`, `speed`, `clear`.
- **`descent_sim` (headless):** runs N seeded runs with bot players (moves away from nearest threats, picks cards by a heuristic, uses dash/skill on cooldown). Outputs per-floor clear time, death rooms, win rate by class and Heat, DPS curve and the distribution of Boons picked. Used for balance and as a regression test.
- **Unit tests (Catch2):** floor generator validity over 10,000 seeds (all special rooms present, reachable, boss farthest); chunk validation; director budget never exceeds `T` or caps; damage formulas; every effect action; Soul Shard/score formulas; server validator rejects impossible summaries.

---

## 18. Balance Targets (Starting Points)

| Metric | Target |
|---|---|
| Full-run time (win) | 25–35 min |
| Boss fight length (biome bosses) | 50–90 s; Vaelor 3–4 min |
| Player DPS at end of floor 1 / 2 / 3 / final | ~100 / ~250 / ~600 / ~1,000 (reference build) |
| Rooms cleared per level-up | Level 10 by end of floor 1; ~24 at the final boss |
| Win rate, no prep, Heat 0, average player | 3–6% (first clear typically after 8–12 runs) |
| Win rate with a mostly filled Anvil, Heat 0 | 25–40% |
| Time to first Anvil rank | After the first run (even a death should buy ≥ 1 rank) |
| Soul Shards per run | 60–90 early, 200+ on a win |
| Frame time at 250 enemies | ≤ 12 ms |
Use `descent_sim` to tune **enemy HP multipliers, budget growth, Boon values and Anvil costs** until the table holds. The bosses' HP in Sections 8–9 assume the reference DPS numbers.

---

## 19. Development Phases & Acceptance Criteria

| Phase | Scope | Done when… |
|---|---|---|
| **D0. Shell** | Mode entry (Hub placeholder), fixed-timestep sim, player movement, camera, one placeholder room, procedural player shape | Run loop starts and ends; 60 FPS; sim has unit tests |
| **D1. Combat core** | Swarmer + Grunt, flow field, collisions, auto-attack (Warrior), damage, XP gems, level-up cards (a few Arms/Tomes), HUD | A single room can be cleared and levelled in; damage formulas tested |
| **D2. Classes (part 1)** | Full kits for **Warrior + Archer** (skill, dash, ultimate, passive) | Both classes playable with all abilities |
| **D3. Floors & rooms** | Floor generator, chunk placement and validation, doors/locking, minimap, room types (Combat/Start/Treasure/Shop), seeds | Generator passes 10,000-seed validity test; floors are traversable |
| **D4. Director & Biome 1** | Threat-budget director, formations, spawn telegraphs, all archetypes, Gravebarrow roster and hazards, elites | Rooms feel varied; caps and performance targets hold |
| **D5. Boons, relics, shops** | Effect system, Terra/Aurel/Ignis Boons, relics, shrines, brews, shop, door rewards | Boon unit tests green; build variety is visible in `descent_sim` |
| **D6. First full floor** | Mini-boss + Osric, run flow, death/summary, Soul Shards, local save, suspend/resume | A complete floor-1 run from Hub to Summary |
| **D7. Content expansion** | Tank/Mage/Summoner, remaining Patrons, Duo/Legendary Boons, all Arms and evolutions | All classes and Patrons playable and tested |
| **D8. Floors 2–3 and finale** | Mirewood, Emberforge, all mini-bosses/bosses, Crowned Hollow, **Vaelor** | The game can be won end to end |
| **D9. Meta & main-game links** | Ember Anvil, mastery, Heat/Pacts, Crown Altar, Preparations (Sigil, Provisions), Veteran bonus, Echo relics, Spoils, Descent Merchant, Endless Loop | Meta loop works; caps enforced (tested) |
| **D10. Online** | Daily/Weekly seeds, run submission and server validation, leaderboards, guild goal, share cards | Server rejects forged summaries (tested); boards update |
| **D11. Polish & balance** | Bot-driven balance pass, accessibility, audio, free-asset pass (license manifest), tutorial hints, gamepad | See Definition of Done |

### Definition of Done
- All five classes, six Patrons, 3 floors, the finale and Vaelor are playable; the permanent Crown reward unlocks on the first victory and persists through cloud sync.
- Main-game links work (Sigil, Provisions, Tribute, Veteran bonus, Echo relics) and Spoils respect the daily cap; **no main-game gear affects the mode**.
- 60 FPS with 250 live enemies on an integrated GPU; no gameplay allocations; the mode runs with **zero external assets**.
- Floor generation never produces an unreachable room (10,000-seed test); every effect has a unit test; `descent_sim` reports balance within Section 18 targets.
- Run submissions are validated server-side; leaderboards, Inbox rewards and guild goals work and integrate with the existing sync, auth and anti-cheat systems.

---

## 20. Assumptions, Risks & Open Questions

- **Controls are mouse + keyboard first;** gamepad is planned for D11. The Archer and Mage are the most mouse-dependent classes (auto-aim is the accessible fallback).
- **Float determinism risk** (Section 14.2/17.4): replay verification is best-effort only. Leaderboard integrity rests mainly on seed-based plausibility checks and on flagging.
- **Local/online co-op** is out of scope for this spec (a likely follow-up that would need server-authoritative or lockstep netcode).
- **Meta power creep:** Anvil, Sigil and Provisions are capped (~+45% total); if players trivialize Heat 0, raise Heat rewards rather than removing meta.
- **Spoils** could undercut mining; the daily cap (4 runs) and the lower material tier (`T−1`) are the safeguards. Tune using real play data.
- **Heavy content volume** (≈ 75 boons/relics/arms, 40 enemies, 4 bosses, ~50 chunks per biome): data-driven design and the shared effect system keep the code small; art volume is why shape fallbacks and free assets matter.
- **Open numbers:** all values are estimates; `descent_sim` is the source of truth.

### Required changes to the main spec (checklist)
1. **Section 3 (Main screen):** add a **Descent** button to the navigation bar (unlock conditions in 2.1).
2. **Section 13 (Screens):** add `DescentHub`, `Preparations`, `EmberAnvil`, `DescentRun`, `DescentSummary`.
3. **Section 22 (Data models):** add `DescentSave` to `SaveState`; share the `GemInstance` type with the Soul Sigil.
4. **Sections 14/18 (Backend, leaderboards):** add the `/descent/*` endpoints, `descent_runs`/`descent_seeds` tables, and the five Descent boards.
5. **Section 16 (Guilds):** add the weekly **Guild Descent Goal**.
6. **Section 24 (Phases):** insert phases D0–D11 after the main game's Phase 9 (cloud saves) at minimum; the online part (D10) after the main game's Phase 10 (leaderboards).
7. **Section 21 (Assets):** extend `wanted.yaml` with the top-down tilesets, enemy sprites, projectile VFX, Boon icons and the dungeon music listed in Section 16 above.
