# Emberdeep: The Hollow Crown — Game Design & Technical Spec

> **Working title.** A medieval-magical clicker game about mining, crafting, and conquering a 10-boss campaign.
> **Audience for this document:** a coding agent building the game in TypeScript + React.
> **All numbers are starting values.** Put them in data files so they can be tuned without touching logic.

---

# Emberdeep: The Hollow Crown — Game Design & Technical Spec (v2)

> **Working title.** A medieval-magical clicker about mining, crafting and conquering a 10-boss campaign.
> **What changed in v2:** native **C++20 client** (replaces TypeScript/React); a **self-hosted C++ backend + PostgreSQL** for cloud saves; **guilds, an async PvP arena and leaderboards**; **Docker** for the server stack; and an **optional free-asset acquisition pipeline**. Sections 1 and 3–13 (game design) are unchanged except where they touch platform, controls or online features.
> **Audience:** a coding agent. All numbers are starting values and live in data files.

---

## 0. Instructions for the Coding Agent

1. Build in the **phases in Section 24**. Finish and verify one phase before starting the next.
2. **Single source of truth for game data:** JSON files in `/data` (materials, gear, gems, recipes, enemies, bosses, perks, layers, arena, guild). Both the client and the server load and validate the same files. Game rules live in the shared **`core`** library (no rendering, no networking, no file I/O).
3. **Assets are optional.** Everything must render and make sound with **zero external assets** using procedural shapes and synthesized audio (the permanent fallback). Downloaded assets (Section 21) are an enhancement layered on top and must follow the licensing rules there.
4. **C++20, CMake, vcpkg (manifest mode, pinned baseline).** Compile with warnings as errors (`-Wall -Wextra -Wpedantic` / `/W4 /WX`), run `clang-format` and `clang-tidy`, and use ASan/UBSan in debug CI. Tests use **Catch2 v3**.
5. **Determinism:** gameplay randomness uses the shared **PCG32** RNG in `core`. **Do not** use `std::uniform_*_distribution` (its output differs between standard libraries). Combat uses **integer / basis-point math only**, so the client and server always agree.
6. **Never block the render thread.** Disk and network I/O run on worker threads and hand results back through a thread-safe queue. Never share mutable game state across threads; pass immutable snapshots.
7. **Desktop-first:** Windows, Linux, macOS. Logical resolution 1280×720 (16:9), scaled to the window with letterboxing. Mouse + keyboard; gamepad is optional and comes late.
8. **The game is offline-first.** Solo play never requires the server. Online features degrade gracefully (clear "Offline" state, no crashes, no lost progress).
9. When something is ambiguous, pick the simplest option, record it in `docs/DECISIONS.md`, and continue.

---
---
## 1. Game Pillars

- **Satisfying clicks.** Every hit needs punch: shake, particles, floating numbers, cracks.
- **Mine → Craft → Conquer.** Mining feeds crafting; crafting feeds power; power (plus skill) beats bosses.
- **Skill checks, not just stats.** Campaign bosses are short mini-games (Undertale/Punch-Out/osu!mania flavor). Gear helps but never replaces skill.
- **Long tail.** After beating all 10 bosses, Echo Ranks, Deep Strata, an endless Abyss dungeon and Ascension keep progression going.

---

## 2. Tech Stack & Architecture

### 2.1 Technology Choices

**Client**

| Concern | Choice |
|---|---|
| Language / build | C++20 (GCC 13+, Clang 17+, MSVC 2022), CMake ≥ 3.25 with `CMakePresets.json`, vcpkg manifest mode |
| Window, 2D graphics, input, audio | **raylib** (latest stable, pin the exact version). Covers OpenGL-backed 2D shapes/polygons/gradients, textures, fonts, shaders, render textures, gamepad, and built-in audio (streamed OGG music, `Sound` SFX, procedural `Wave`/`AudioStream` synthesis) |
| GUI | A **custom retained-mode UI layer** in `client/src/ui/`: `Panel, Button, Label, ProgressBar, Tabs, ScrollList, ItemGrid, Tooltip, Modal, TextInput, Toast`, driven by a `theme.json`. `raygui` is allowed for dev/debug tools only. Reason: this game needs a themed stone/parchment look, and stock immediate-mode widgets look like tools |
| HTTP / WebSocket | `cpr` (libcurl) for REST, `IXWebSocket` for WSS |
| JSON, compression | `nlohmann/json`, `zstd` |
| Crypto | `libsodium` (hashing, random, HMAC); TLS via libcurl/OpenSSL |
| Logging | `spdlog` |
| Tests | Catch2 v3 |

**Server**

| Concern | Choice |
|---|---|
| Framework | **Drogon** (async C++ HTTP + WebSocket, built-in PostgreSQL client) |
| Database | **PostgreSQL 16**, self-hosted, raw SQL migrations |
| Cache / rate limits | Optional **Redis 7** (Compose profile `scale`); in-memory fallback for a single instance |
| Shared code | Links the same `core` library and `/data` files as the client, so the server can recompute stats and re-run combat itself |
| Packaging | Docker multi-stage image (Section 20) |

**Why raylib:** one dependency covers window, drawing primitives, audio and input; its shape-drawing API (`DrawPoly`, `DrawRectangleRounded`, `DrawCircleGradient`, `DrawSplineBezier`, `DrawRing`) matches this game's "art made from shapes" approach; and it builds cleanly with CMake/vcpkg. **Alternatives** if raylib blocks the project: SDL3 + SDL_ttf + SDL_mixer, or SFML 3 + TGUI. To keep that swap possible, screens and minigames must draw through a thin `gfx::` wrapper and play sound through `audio::`, never through raw raylib calls scattered around. Record any switch in `DECISIONS.md`.

**Why a C++ server:** the combat simulator, stat calculator and validation rules are shared code, so PvP results and plausibility checks are computed by exactly the same logic the client uses. That also removes a whole class of "client and server disagree" bugs.

### 2.2 Repository Layout
```
emberdeep/
  CMakeLists.txt  CMakePresets.json  vcpkg.json
  core/                    # shared static lib (client + server + tools + tests)
    include/ed/  src/      #   rng, fixed math, formulas, stats, mining, crafting, combat,
                           #   progression, gemevent, echo, ascension, save schema, validation, api DTOs
  data/                    # JSON game data (validated against schemas in data/schema/)
  client/src/
    app/ screens/ ui/ gfx/ audio/ minigames/ net/ save/ assets/
  server/
    src/  api/ (controllers)  services/  db/  ws/  jobs/
    migrations/*.sql
  tools/                   # asset fetcher (Python), balance simulator, chart tools, admin CLI
  assets/                  # optional downloaded assets + manifest.json (Section 21)
  docker/                  # Dockerfile, compose files, Caddyfile, backup scripts
  tests/                   # core unit tests, server integration tests
  docs/DECISIONS.md
```

### 2.3 Client Architecture
- **Main loop:** fixed **60 Hz update**, rendering every frame with interpolation where useful. Everything is drawn into a 1280×720 render texture, then scaled to the window.
- **Screens:** a stack (`Main, Inventory, Crafting, Profile, Campaign, Minigame, Dungeon, GemHollow, Guild, Arena, Leaderboards, Settings`). Modals sit above the stack.
- **Services** (owned by `App`, passed by reference to screens): `SaveService` (local), `SyncService` (cloud, worker thread), `NetClient`, `AudioService`, `AssetManager` (falls back to procedural drawing), `TimeService`, `Settings`.
- **State:** plain structs in `core` (`GameState`), mutated only on the main thread by `core` system functions. UI reads state; UI never contains rules.
- **Events:** a small typed event bus (`BlockBroken`, `LevelUp`, `BossCleared`, `ItemCrafted`, …) drives toasts, achievements, sound, and "mark save dirty".

### 2.4 Core Rules
- **Number formatting:** `1.2K, 3.4M, 5.6B, …` via `core::format`. Currencies are `double`; combat values are `int64_t`.
- **Time:** `TimeService` returns server-corrected UTC (offset learned from `GET /v1/time`), falling back to the local clock offline. Daily and weekly resets use it.
- **Local save:** JSON → zstd → `.sav` in the OS user-data directory (`%APPDATA%`, `~/.local/share`, `~/Library/Application Support`). Writes are **atomic** (temp file + rename), with **3 rolling backups**. Includes `schema_version` and migrations. Export/Import via file and a base64 string.
- **Pause:** minigames pause on window focus loss.





## 3. Main Screen

### Layout (portrait; landscape stretches horizontally)
```
┌────────────────────────────────────────┐
│ ☆ Lv 12  [XP ▓▓▓▓░░]        🪙 1,240   │  top bar: level, xp bar, gold
├────────────────────────────────────────┤
│         Layer: Iron Hollows ▾          │  layer selector
│                                        │
│        ▓▓▓▓▓▓▓▓▓░░░░  620/900         │  block health bar (above block)
│        ╭── Rare Iron Ore ──╮           │  name + rarity tag
│        │      ⬢ BLOCK ⬢     │          │  big clickable SVG block
│        ╰────────────────────╯          │
│   HP 900 · Hardness 10 · XP 29         │  block stat chips
│   Rarity: Rare (×2.5 HP, ×3 drops)     │
│                                        │
│ [ 💎 Gem Hollow – READY ]              │  daily event button (glows when available)
├────────────────────────────────────────┤
│ [Inventory] [Crafting] [Dungeon]       │
│ [Campaign ] [Profile ]                 │  bottom navigation
└────────────────────────────────────────┘
```

### Required UI Elements
- **Block health bar** above the block. Numeric HP text, smooth drain, flashes on crit.
- **Block info:** material, rarity tag, max HP, hardness ("strength"), XP given, and an expected-drops tooltip.
- **Layer selector:** choose which layer's blocks spawn (unlocked layers only).
- **Equipped pickaxe** shown as a shape-built pickaxe that swings on each click.
- **Bottom nav:** Inventory, Profile, Crafting, Campaign, plus Dungeon. Icons are SVG primitives.
- **Gem Hollow button:** greyed with a countdown ("Resets in 06:12:40") when used today; golden glow when ready.
- **Toasts** for level-ups, rare drops, unlocks.

### Feedback / Juice (all shape-based)
- Block squash on hit, screen shake scaled to crit.
- Crack overlay: SVG paths revealed at 75%, 50%, 25% HP.
- Particles: small polygons in the material's color, gravity and fade.
- Floating damage numbers (white normal, gold crit, larger on crit).
- Break: block shatters into ~12 shard polygons; drops fly to a "loot" indicator.
- **Weak Spot (optional but recommended):** a glowing rune circle appears on the block every 4–8 s. Clicking it is a guaranteed crit for +50% damage. It disappears after 1.5 s.

---

## 4. Mining System

### 4.1 Damage Formula
```
rawDamage        = miningPower × (1 + powerBonus%) × (isCrit ? critDamage : 1)
effectiveHardness = max(0, block.hardness − pierce)
finalDamage      = rawDamage × 100 / (100 + effectiveHardness)   // min 1
```
- `critChance` base 5%, `critDamage` base ×1.5.
- Holding a click/touch does **not** auto-repeat by default (true clicker). Optional setting "Hold to mine" at 5 hits/s.
- Rapid-fire limit: max 15 registered hits/s to prevent autoclicker abuse from breaking balance.

### 4.2 Block Generation
When a block breaks, the next spawns:
1. Pick a **material tier** from the selected layer: **75%** layer tier `n`, **20%** tier `n−1` (if n>1), **5%** tier `n+1` (bonus tough block, if it exists).
2. Roll a **rarity** (Luck stat shifts weight toward higher rarities).
3. Final HP, XP and drops = material base × rarity multiplier.

### 4.3 Block Rarity
| Rarity | Base Weight | HP × | XP × | Drop Qty × | Extras |
|---|---|---|---|---|---|
| Common | 70% | 1.0 | 1.0 | 1.0 | — |
| Uncommon | 22% | 1.5 | 1.5 | 2.0 | Faint sparkle |
| Rare | 6.5% | 2.5 | 3.0 | 3.0 | +Arcane Dust chance ×2, glowing outline |
| Epic | 1.5% | 5.0 | 5.0 | 6.0 | Guaranteed Arcane Dust, animated aura |

Luck: each point of Luck moves 0.1% weight from Common into the higher rarities proportionally (cap +15%).

### 4.4 Drops
- **Ore:** 1–2 of the block's material × rarity qty multiplier.
- **Arcane Dust:** 8% base chance per block (any material), 1–3 dust. Used in upgrades and enchanting.
- **Gold:** none directly from blocks. Gold comes from selling and the dungeon.
- **Double drop:** Luck gives `Luck × 0.2%` chance to double ore.

---

## 5. Materials & Layers (Progression Ladder)

Ten tiers, one per layer. Layer *n* is unlocked when **player level ≥ layer.minLevel** and you own a **pickaxe of tier ≥ n−1**. Layer 1 is open from the start.

| # | Material | Layer | Color (hex) | Block Look (shapes) | HP | Hardness | XP | Sell (ore) | Min Lv |
|---|---|---|---|---|---|---|---|---|---|
| 1 | **Ashen Stone** | Ashen Warrens | `#8a8a94` | Grey rounded boulder, pale facets | 20 | 0 | 5 | 1 | 1 |
| 2 | **Copper** | Copper Crypt | `#c9773f` | Orange-brown hex with green verdigris specks | 52 | 4 | 12 | 3 | 5 |
| 3 | **Iron** | Iron Hollows | `#7b8794` | Blue-grey block, riveted corners | 135 | 10 | 29 | 8 | 10 |
| 4 | **Silvervein** | Silvermire | `#d7e2ec` | Pale block with bright zig-zag veins | 350 | 20 | 69 | 20 | 15 |
| 5 | **Aurelite** | Aurelite Keep | `#f2c14e` | Gold-yellow crystals, warm glow | 915 | 36 | 166 | 50 | 20 |
| 6 | **Runic Slate** | Runic Archives | `#5a4fcf` | Violet slab with pulsing glowing rune | 2,380 | 60 | 398 | 125 | 27 |
| 7 | **Starmetal** | Stargazer Spire | `#6fd3ff` | Dark navy meteor chunk with twinkling star points | 6,190 | 95 | 955 | 310 | 34 |
| 8 | **Wyrmite** | Wyrmbone Caverns | `#d94f30` | Bone-white and ember-red striped shard | 16,100 | 145 | 2,290 | 780 | 42 |
| 9 | **Voidglass** | Voidglass Sanctum | `#2b0f3a` | Near-black glassy prism with magenta edge light | 41,800 | 210 | 5,500 | 1,950 | 50 |
| 10 | **Aetherium** | The Aether Vault | `#7cf7e0` | Iridescent cyan/pink shifting crystal | 108,700 | 300 | 13,200 | 4,900 | 60 |

Formulas behind the table (use them to generate later tiers): `HP = 20 × 2.6^(n−1)`, `XP = 5 × 2.4^(n−1)`, `sell = 1 × 2.5^(n−1)`.

### Deep Strata (post-game, endless)
After beating the final boss, layer 10 gains a **Depth** counter. Depth `d ≥ 1` generates blocks with `HP × 2.6^d`, `XP × 2.4^d`, ore drops of Aetherium ×(1 + 0.5d), and a growing chance of **Aether Shards** (used in Mythic upgrades, Section 8.4).

### Secondary Materials
| Item | Source | Use |
|---|---|---|
| **Arcane Dust** | Any block (8%) | Upgrades, gem cutting |
| **Ingots** (one per ore) | Smelting | Gear crafting |
| **Trophies** (one per dungeon zone) | Dungeon enemies | Gear crafting (Section 9) |
| **Boss Essence** | Boss first clears and star rewards | Legendary Awakening |
| **Echo Sigils** | Echo bosses (post-game) | Mythic upgrades |
| **Aether Shards** | Deep Strata | Mythic upgrades |
| **Ember Marks** | Ascension | Permanent perks |

---

## 6. Gemstones & Daily Event

Gems are **not** found in normal blocks. They come from the **Gem Hollow** daily event, boss star rewards and rare dungeon chests.

### 6.1 Gem Table
| Rarity | Weight | Gems | Effect when socketed (Rough / Cut / Flawless = ×1 / ×2 / ×4) |
|---|---|---|---|
| Common | 55% | **Amber** | +1 Luck |
| | | **Garnet** | +2% Attack |
| Uncommon | 28% | **Jade** | +2% Max HP |
| | | **Moonstone** | +0.5% Crit Chance |
| Rare | 12% | **Sapphire** | +1% Focus |
| | | **Bloodstone** | 1% lifesteal (dungeon) |
| Epic | 4.2% | **Star Ruby** | +3% Mining Power |
| | | **Arcane Opal** | +3% XP gain |
| Legendary | 0.8% | **Dragon's Eye** | +2 Pierce, +5% Crit Damage |
| | | **Aether Diamond** | +1.5% to all stats |

Gems drop as **Rough**. Combining **3 of the same gem** at the same cut into the **next cut** costs Arcane Dust + gold (Rough→Cut: 10 dust; Cut→Flawless: 30 dust). Gems are socketed into gear sockets (see Section 8) and can be swapped for a small gold fee.

### 6.2 The Gem Hollow (Once-a-Day Event)
- **Availability:** once per local calendar day. The main-screen button shows READY or a countdown to local midnight.
- **Scene:** a dark crystal cavern with **12 dull rock veins** in a grid; a lantern circle follows the cursor and lights nearby veins.
- **Swings:** the player has **25 swings** (+1 per consecutive-day streak day, max +10). Each tap = 1 swing.
- **Veins:** each vein has 1–3 durability. On break, a roll: **45% gem**, **35% Arcane Dust (2–6)**, **20% gold pouch**.
- **Weak-point ring:** on some veins a ring shrinks; tapping while it is inside the target zone breaks the vein instantly and adds a **+1 gem** bonus.
- **Luck** shifts the gem rarity weights upward by up to +25% of the weight of the tiers above Common.
- **Expected result:** ~5–8 gems, occasionally a Legendary. Show a results summary with rarities highlighted.
- **Rules:** progress is saved mid-event (if the user leaves, they can resume the same day); once finished or expired at midnight, it locks until the next day.

---

## 7. Player Progression

### 7.1 Level & XP
- XP from breaking blocks, dungeon wins and boss clears.
- `xpToNext(L) = round(40 × L^1.8)`. Level cap **60**, increased by Ascension.
- Each level gives: **+1.5% Mining Power**, **+3 Max HP**, **+1 Attack**, and **1 Vow Point**.

### 7.2 Miner's Vows (Perk Tree)
Spend Vow Points in three branches. Each node has ranks (max shown).

| Branch | Node | Effect per rank | Max |
|---|---|---|---|
| **Delver** (mining) | Strong Arms | +3% Mining Power | 10 |
| | Keen Eye | +1% Crit Chance | 10 |
| | Prospector | +1 Luck | 10 |
| | Deep Breath | Weak Spot appears 5% more often | 5 |
| | Shatterer (capstone, Lv 30) | Every 25th hit is a guaranteed crit | 1 |
| **Warrior** (combat) | Stout Heart | +4% Max HP | 10 |
| | Whetstone | +3% Attack | 10 |
| | Ironskin | +2% Defense | 10 |
| | Swift Boots | +1 Speed | 5 |
| | Second Wind (capstone, Lv 30) | Once per dungeon run, revive at 30% HP | 1 |
| **Scholar** (economy) | Quick Study | +3% XP | 10 |
| | Haggler | +3% gold from sales | 10 |
| | Steady Hand | +1% Focus (campaign leniency) | 10 |
| | Dust Collector | +2% Arcane Dust chance | 5 |
| | Gem Whisperer (capstone, Lv 30) | +3 Gem Hollow swings | 1 |

Provide a free **respec** button that costs gold (`level × 100`).

### 7.3 Stat Glossary
| Stat | Category | Meaning |
|---|---|---|
| Mining Power | Mining | Base damage per click |
| Crit Chance / Crit Damage | Mining & Combat | Chance and multiplier |
| Pierce | Mining | Flat reduction of block hardness |
| Luck | Mining | Rarity weight shift, double-drop chance, gem rarity |
| Scholar (XP%) | Economy | Bonus XP |
| Max HP | Combat & Campaign | Dungeon health |
| Attack | Combat | Dungeon damage |
| Defense | Combat | Damage reduction: `100/(100+Def)` |
| Speed | Combat | Turn order |
| **Focus** | Campaign | Widens timing windows and lowers thresholds by up to **+25%** |
| **Ward** | Campaign | Extra hearts in boss fights, max **+2** |

---

## 8. Gear

### 8.1 Slots & Primary Stats
Six slots. Each gear piece has a **tier (1–10)**, a **base stat set**, an **upgrade level** and **sockets**.

| Slot | Primary Stats | Secondary Stats |
|---|---|---|
| **Pickaxe** | Mining Power, Attack | Pierce, Crit Chance, Crit Damage |
| **Helm** | Max HP | Luck, Ward |
| **Chestplate** | Defense, Max HP | Ward |
| **Gauntlets** | Attack | Crit Chance, Focus |
| **Boots** | Speed | Max HP, Focus |
| **Amulet** | Luck | XP%, Crit Damage |

### 8.2 Stat Scaling by Tier
`stat(tier) = baseStat × 2.3^(tier − 1)` (Pickaxe Power uses `3 × 2.6^(tier−1)` to match block HP).

| Slot | Base (tier 1) |
|---|---|
| Pickaxe | Power 3, Attack 2, Pierce 0 (+1 per 2 tiers), Crit 5% |
| Helm | Max HP 30, Luck 1 |
| Chestplate | Defense 4, Max HP 50 |
| Gauntlets | Attack 4, Crit 2% |
| Boots | Speed 5 (+1.5 per tier, not exponential), Max HP 15 |
| Amulet | Luck 2, XP% 3 |

Ward comes only from Helm/Chestplate at tiers 4, 7, 10 (+1 each, hard-capped at +2 total). Focus comes from Gauntlets/Boots tiers 3, 6, 9 (+5% each, capped at 25% total incl. perks).

### 8.3 Gear Names by Tier (Fantasy Flavor)
| Tier | Set Name | Pickaxe | Helm | Chestplate | Gauntlets | Boots | Amulet |
|---|---|---|---|---|---|---|---|
| 1 | Ashen | Stone Pick | Ash Cap | Ash Jerkin | Ash Wraps | Ash Sandals | Pebble Charm |
| 2 | Copper | Copper Pick | Copper Coif | Copper Cuirass | Copper Grips | Copper Greaves | Copper Locket |
| 3 | Iron | Iron Pick | Iron Helm | Iron Hauberk | Iron Gauntlets | Iron Sabatons | Iron Sigil |
| 4 | Silver | Silver Pick | Silver Circlet | Silver Mail | Silver Gloves | Silver Treads | Silver Pendant |
| 5 | Aurelite | Gilded Pick | Gilded Helm | Gilded Plate | Gilded Fists | Gilded Boots | Gilded Amulet |
| 6 | Runic | Runepick | Rune Hood | Rune Robe | Rune Mitts | Rune Slippers | Rune Talisman |
| 7 | Starmetal | Starbreaker | Star Helm | Star Plate | Star Gauntlets | Star Greaves | Star Choker |
| 8 | Wyrm | Wyrmfang Pick | Wyrm Helm | Wyrm Hide | Wyrm Claws | Wyrm Boots | Wyrm Talon |
| 9 | Void | Voidpick | Void Cowl | Void Shroud | Void Grasp | Void Steps | Void Eye |
| 10 | Aether | Aetherpick | Aether Crown | Aether Plate | Aether Hands | Aether Stride | Aether Heart |

### 8.4 Upgrading
- Upgrade levels **+0 → +10**. Each level adds **+8% of the piece's base stats** (so +10 = ×1.8).
- Cost for level `k` (1–10) of a tier `t` piece: **gold** `20 × 3^(t−1) × k^1.5`, **ingots** of that tier `ceil(k/2) + 1`, **Arcane Dust** `k`.
- Upgrades never fail (no frustration). Downgrade/refund is not allowed.
- **Mythic levels (+11 to +20):** unlocked after beating the final boss. Each adds +10% base stats. Cost also needs **Aether Shards** (Deep Strata) and **Echo Sigils** (Echo bosses).

### 8.5 Sockets
- Sockets by tier: 1–3: 0, 4–6: 1, 7–9: 2, 10: 3. Legendaries always have +1 extra.
- Gems are socketed in the Inventory/Gear detail view; swapping costs `100 × tier` gold.

### 8.6 Legendary Set Bonus
The number of **equipped legendary items** grants:
- **2:** +10% Mining Power and +10% Luck
- **4:** +25% Attack, Max HP and Defense; +1 Ward (counts toward the cap)
- **6 (all slots):** "Hollow Crowned" — +50% XP, unique golden aura around the equipped-gear paperdoll and the mining pickaxe

---

## 9. Crafting

### 9.1 Screens & Flow
Tabs: **Smelter**, **Forge**, **Gem Cutter**, **Enchant (later)**.

**Smelter:** convert **4 ore → 1 ingot** of that tier, instant (with an animated furnace made of shapes). Bulk buttons: ×1, ×10, Max.

**Forge:** list recipes for every unlocked tier; show cost, resulting stats, and a compare arrow versus the currently equipped piece. Crafting takes 1 s (a hammer strike animation) with a small delay for feel.

### 9.2 Recipes (each of the 10 tiers, 6 slots)
| Slot | Ingots (tier) | Trophies (tier) | Extras | Gold |
|---|---|---|---|---|
| Pickaxe | 8 | 2 | — | `60 × 3^(t−1)` |
| Helm | 6 | 3 | — | `40 × 3^(t−1)` |
| Chestplate | 10 | 4 | — | `80 × 3^(t−1)` |
| Gauntlets | 5 | 2 | — | `40 × 3^(t−1)` |
| Boots | 5 | 2 | — | `40 × 3^(t−1)` |
| Amulet | 3 | 2 | 1 gem (any) | `50 × 3^(t−1)` |

Tier 1 exception: no trophies required (tutorial-friendly). Trophies come from the dungeon (Section 11), which links dungeon runs to crafting.

### 9.3 Enchanting (Phase 8+)
Spend Arcane Dust + gold to add **one random bonus stat line** to a piece (e.g. +3% Crit, +2 Luck). Reroll costs increase. Locked behind Level 20.

---

## 10. Campaign (10 Bosses)

Each boss is a **short skill mini-game** (30–90 s) on a canvas. Think Undertale attack patterns and Punch-Out reads.

### 10.1 Global Rules
- **Unlock:** boss *n* requires boss *n−1* cleared and a minimum player level (`minLevel = layer's Min Lv`, recommended gear tier = *n*).
- **Hearts:** you start every fight with **3 + Ward** hearts. Reaching 0 = defeat. Unlimited retries, no cost.
- **Focus (assist):** all timing windows are multiplied by `(1 + focus)` and score thresholds divided by `(1 + focus)`. Focus is capped at 25% so skill stays relevant.
- **Stars:** each boss has 1★ (clear), 2★ (good), 3★ (flawless). Each star gives a **one-time reward** (gold, gems, Boss Essence).
- **Rewards:** first clear = big gold + XP + **guaranteed Legendary**. Repeat clears = smaller gold + trophies; 3★ repeats have a 5% chance of a Boss Essence.
- **Presentation:** before each fight, a title card (shape-built boss silhouette), a one-line rule explanation, a "Ready?" button, then a 3-2-1 countdown. Every fight has a pause button and a "Give Up".
- **Controls:** each mini-game supports **touch/mouse** and **keyboard** (listed below).

### 10.2 The Ten Bosses

#### 1. Grubnok, the Burrow-King — *Ashen Warrens*
- **Look:** big brown circle head, two triangle ears, jagged gold crown, tiny circle eyes.
- **Challenge: Frenzy Dig (CPS test).** Tap the throne rock as fast as possible for **10 s**. A Rage bar fills as CPS rises.
- **Clear:** average ≥ 5 CPS. 2★: ≥ 7. 3★: ≥ 9. (Thresholds divided by 1 + Focus.)
- **Twist:** a golden nugget spawns randomly; tapping it gives +5 instant taps.
- **Controls:** tap/click, or Space.

#### 2. Sir Rattlebone — *Copper Crypt*
- **Look:** stacked grey circles and rectangles forming a skeleton knight, visor as a dark rectangle, rounded shield.
- **Challenge: Bone Duel (timing).** A needle sweeps across a bar; stop it inside the green zone. **8 rounds**; the zone shrinks from 30% to 8% width and needle speed rises.
- **Rules:** a miss costs a heart. 3★: no misses and ≥ 4 center ("perfect") hits.
- **Controls:** tap/click or Space.

#### 3. Morwenna the Ashen Witch — *Iron Hollows*
- **Look:** dark triangle robe, pointed hat, floating ember circles, a hooked staff.
- **Challenge: Cinder Dance (4-lane rhythm, osu!mania style).** Notes fall down 4 lanes. Hit them on the line. **75 s, ~110 notes**, 110 BPM, defined in a chart file (`chart_morwenna.ts`: array of `{time, lane}`).
- **Judgments:** Perfect ±45 ms, Good ±100 ms, otherwise Miss.
- **Groove gauge:** starts at 50, +2 per hit, −6 per miss. Reaching 0 fails the fight (hearts are not used here).
- **Clear:** accuracy ≥ 70% (`(perfect + 0.6×good)/total`). 2★ ≥ 85%. 3★ ≥ 95%.
- **Controls:** keys `D F J K` or tap the 4 lanes. Beat cues come from a WebAudio metronome (synth clicks) so no audio files are needed.

#### 4. Thessaly, the Bog Hydra — *Silvermire*
- **Look:** three green necks (thick curved paths) with circle heads and yellow slit eyes, rising from a murky ellipse.
- **Challenge: Head Hunt (whack-a-mole with decoys).** 3×3 holes, **35 s**. Real heads (green) pop up for 1.4 s down to 0.8 s. **Decoys** (blue, bubbly) pop up 30% of the time.
- **Rules:** clear with ≥ 30 real hits. Tapping a decoy costs a heart. Missed real heads have no penalty. 3★: ≥ 45 hits and no heart lost.
- **Controls:** tap/click; keyboard `1–9` on a numpad layout.

#### 5. Aldric, the Gilded Knight — *Aurelite Keep*
- **Look:** tall gold rectangle armor with a plume triangle, a huge circle shield and a long thin sword.
- **Challenge: Parry Duel (reaction).** **24 attacks** telegraphed from **Left, Right or Up** (arrow flashes and the sword pose changes). Press the matching direction inside the window (600 ms shrinking to 350 ms).
- **Feints:** from attack 12 on, some telegraphs are fakes (grey flash). Do **nothing** on those, or lose a heart.
- **Rules:** wrong/late = −1 heart. Every 6th successful parry opens a **Counter window** (press Space/tap Attack) that damages Aldric (visual only).
- **Clear:** survive all 24. 3★: no hearts lost.
- **Controls:** arrow keys or swipe / tap zones on screen; Space/tap center for counter.

#### 6. Isolde, Runewarden — *Runic Archives*
- **Look:** hooded triangle robe, an open book (two quads), six glowing rune shapes orbiting her.
- **Challenge: Rune Recall (Simon says).** Six runes, each a distinct shape and color (triangle, square, hexagon, circle, diamond, star). Watch a sequence, then repeat it. **10 rounds**, sequence length 3 → 12, show speed 600 ms → 350 ms per rune.
- **Rules:** a wrong rune = −1 heart, and the round replays. 3★: no mistakes.
- **Controls:** tap runes; keys `1–6`.

#### 7. Seraphel, the Fallen Star — *Stargazer Spire*
- **Look:** a large glowing 8-point star polygon with a soft radial-gradient halo and a cracked "face" made of two small dark shapes.
- **Challenge: Star Trace (ordered pointing under pressure).** **40 s**. Constellations of 4–6 numbered stars appear; tap them **in order** before they fade (2 s each). Meteors (streaks) cross the screen; tapping a meteor costs a heart.
- **Clear:** finish ≥ 8 constellations. 3★: ≥ 12 and no hearts lost.
- **Controls:** tap/click only (mouse-friendly), optional keyboard number keys not required.

#### 8. Ignathia, the Ember Wyrm — *Wyrmbone Caverns*
- **Look:** a long serpentine path body in red, bone-white spike triangles, a wedge head with glowing orange eye.
- **Challenge: Wyrmfire Run (lane dodge + burst).** **45 s**, 3 lanes, auto-scroll. Dodge fire waves by moving left/right; pick up embers for a score bonus. Every 15 s, a **Scale Break** phase: mash **25 taps in 3 s** to shatter a shield, or take a hit.
- **Rules:** waves hit = −1 heart. Clear = reach the end. 3★: ≤ 1 heart lost and ≥ 20 embers.
- **Controls:** ←/→ keys (or A/D), swipe, or tap left/right half.

#### 9. Xar'Mal, the Lich Emperor — *Voidglass Sanctum*
- **Look:** tall dark polygon robe, a skull made from circles and triangles, floating crown shards, violet fire.
- **Challenge: Soul Dodge (bullet hell, Undertale-style).** **60 s**. The player controls a small heart (hitbox radius 5) inside a square arena. Patterns come in waves: **spirals, falling rain, sweeping lasers (0.8 s telegraph), orbiting rings**, defined in a scripted timeline (`pattern_xarmal.ts`).
- **Rules:** on hit −1 heart, then 1 s of invincibility. Clear = survive 60 s. 3★: ≤ 1 hit taken.
- **Controls:** mouse/touch drag (heart follows finger with offset), or arrow keys / WASD with `Shift` for slow-move.

#### 10. The Hollow King — *The Aether Vault / Hollow Throne*
- **Look:** giant empty armor (rectangles, no face), a floating crown ring of teal light, a hollow chest showing swirling stars.
- **Challenge: The Five Trials (gauntlet).** Hearts are shared across all phases (3 + Ward; **1 heart is restored between phases 2 and 3**).
  1. **Phase 1 — Burst:** 6 s CPS burst, need ≥ 8 CPS.
  2. **Phase 2 — Rhythm:** 30 s, 4 lanes, groove gauge rules from Boss 3.
  3. **Phase 3 — Parry:** 12 attacks with feints (Boss 5 rules).
  4. **Phase 4 — Soul Dodge:** 40 s pattern remix (Boss 9 rules).
  5. **Phase 5 — Crown Strike:** 3 timing hits with a 5% zone and fast needle.
- **Clear:** finish all phases. 2★: ≤ 2 hearts lost. 3★: 0 hearts lost.

### 10.3 Legendary Drops (Guaranteed on First Clear)
Base stats: **tier-*n* stats × 1.5**, +1 extra socket, upgradable like normal gear. Each has a unique effect.

| # | Item | Slot | Unique Effect |
|---|---|---|---|
| 1 | **Burrowking's Crown** | Helm | **Tremor:** every 10th click deals ×3 damage. |
| 2 | **Bonecleaver Gauntlets** | Gauntlets | **Rattling Rhythm:** consecutive crits stack +5% Crit Damage (max 10 stacks, reset on non-crit). |
| 3 | **Cinderheart Amulet** | Amulet | **Cinderlight:** 8% per click to ignite the block for 3 s, dealing 40% of Power per second. |
| 4 | **Hydrascale Cuirass** | Chestplate | **Regrowth:** heal 3% Max HP at the start of each dungeon round. |
| 5 | **Sunforged Pickaxe** | Pickaxe | **Midas Strike:** 10% chance on block break to grant bonus gold equal to 5× that ore's sell value. |
| 6 | **Runewarden's Diadem** | Helm | **Scholar's Insight:** +30% XP and Arcane Dust drops ×1.5. |
| 7 | **Starfall Treads** | Boots | **Stargazer's Stride:** +15% Speed and always act first in round 1 of dungeon fights. |
| 8 | **Wyrmfang Breaker** | Pickaxe | **Earthsplitter:** crits deal +100% damage to blocks above 50% HP. |
| 9 | **Voidglass Phylactery** | Amulet | **Soul Siphon:** heal 5% of damage dealt in the dungeon; +20% Pierce effect on blocks. |
| 10 | **Aetherbrand, the Hollow Pick** | Pickaxe | **Hollow Echo:** every click has a 15% chance to strike again for 60% damage (can chain once). Visually leaves a teal afterimage. |

### 10.4 Awakening
Spend **Boss Essence** to awaken a legendary (I, II, III). Each level increases the unique effect's numbers by +25% and adds +10% base stats. Costs: 5 / 15 / 40 essence.

---

## 11. Dungeon (Auto-Battle for Gold)

### 11.1 Overview
The player picks a **dungeon zone** (same 10 zones as the layers). Zone *n* unlocks when boss *n−1* is cleared (zone 1 always open). Press **Enter Dungeon** and a turn-based fight plays out **automatically** — no inputs beyond the speed toggle (×1, ×2, ×4) and Skip.

### 11.2 Run Structure
- A run has **5 floors**. Floors 1–4 spawn 1–3 enemies; floor 5 is a **mini-boss**.
- The full run is **simulated deterministically first** (seeded), then replayed visually. This makes Skip instant and tests trivial.
- If the player dies, they keep half the gold and loot found so far; if they clear all 5 floors they keep all and get a **chest** (extra gold + chance for a gem or Arcane Dust).
- Optional config flag `DUNGEON_TORCHES_ENABLED` (default `false`): a run costs 1 Torch, max 10, regen 1 per 5 min.

### 11.3 Combat Rules
- **Round:** all combatants act in descending **Speed** order (ties: player first).
- **Basic attack:** `damage = attack × rand(0.9, 1.1) × 100/(100 + targetDefense)`; crit uses the crit chance/damage stats.
- **Enemy abilities** (simple, cooldown-based): *Heal* (restore 15% HP), *Poison* (5% of max HP over 3 rounds), *Guard* (halve next damage), *Frenzy* (+30% attack for 2 rounds).
- **Player:** attacks automatically; legendary passives and gem effects trigger (Regrowth, Soul Siphon, Bloodstone lifesteal, etc.).
- **Battle log:** compact text lines under the animation (e.g. "You hit Cave Rat for 34!").

### 11.4 Enemy Scaling (Zone n, Floor f 1–5)
```
enemyHP      = 40  × 2.4^(n−1) × (1 + 0.15(f−1))
enemyAttack  = 6   × 2.2^(n−1) × (1 + 0.10(f−1))
enemyDefense = 4   × 2.1^(n−1)
enemyGold    = 10  × 2.5^(n−1) × (1 + 0.10(f−1))
enemySpeed   = 8 + 2n
Mini-boss    = ×3 HP, ×1.5 Attack, ×3 Gold, and one extra ability
```
Player stats come from gear and level; tune with a balance simulation script (`npm run balance`) that reports win rate and gold/min per zone for a "typical" gear set.

### 11.5 Zones, Enemies & Trophies
| # | Zone | Regular Enemies (shapes) | Mini-Boss | Trophy Drop |
|---|---|---|---|---|
| 1 | Ashen Warrens | Cave Rat (small oval + tail), Goblin Grub, Dust Imp | Gnome Sergeant | Gnawed Pelt |
| 2 | Copper Crypt | Skeleton Thrall, Crypt Bat, Green Slime | Bone Captain | Grave Bone |
| 3 | Iron Hollows | Cinder Cultist, Fire Wisp, Iron Golem-lite | Ashen Priestess | Cinder Wick |
| 4 | Silvermire | Bog Toad, Mire Wraith, Leech Swarm | Mire Matron | Bog Scale |
| 5 | Aurelite Keep | Gilded Guard, Cursed Hound, Coin Mimic | Steward of Gold | Gilded Thread |
| 6 | Runic Archives | Ink Wisp, Tome Mimic, Rune Sentinel | Archivist Shade | Ink Wisp |
| 7 | Stargazer Spire | Comet Sprite, Moonlit Gargoyle, Star Weaver | Astral Watcher | Star Shard |
| 8 | Wyrmbone Caverns | Drake Whelp, Ember Salamander, Bone Wyrmling | Wyrm Warden | Wyrm Scale |
| 9 | Voidglass Sanctum | Shadow Acolyte, Void Leech, Hollow Knight | Void Reaver | Void Ichor |
| 10 | The Aether Vault | Aether Phantom, Crown Shard, Vault Warden | Echo of the King | Aether Mote |

Each enemy is drawn from 3–6 primitives, using zone color palettes.

### 11.6 Rewards
- **Gold** per enemy (formula above), scaled by `1 + Haggler%`.
- **Trophies:** 1–2 per floor cleared.
- **XP:** `enemyGold × 0.5`.
- **Chest (run clear):** gold ×3, 15% Arcane Dust, 5% gem (Rough).

---

## 12. Post-Campaign Scaling (After All 10 Bosses)

### 12.1 Echo Campaign
After beating the Hollow King, the campaign unlocks **Echo Ranks**. Bosses return in a fight against their "Echo" (purple-tinted variants).
- **Rank R:** all bosses get modifiers `mod = 1 + 0.08 × (R − 1)`, capped at 2.0: faster needles/patterns, narrower windows, higher CPS thresholds.
- **Rewards:** `1 + 0.25 × (R − 1)` × base gold; each Echo boss clear grants **Echo Sigils** (used for Mythic upgrades and Ascension boosts).
- **Rank up:** clear all 10 Echo bosses at rank R to unlock R + 1. Echo Focus is capped lower (15%) so it does not trivialize things.
- Hearts stay the same (3 + Ward), so mastery matters.

### 12.2 Deep Strata & Abyss
- **Deep Strata:** endless mining depth in the Aether Vault (see Section 5).
- **The Abyss:** endless dungeon; each floor is +8% stronger than the last, a mini-boss every 10 floors, weekly-seeded. Rewards Aether Motes, Echo Sigils and a leaderboard-style personal best (local only).

### 12.3 Mythic Gear
Levels +11 to +20 (Section 8.4) and a fourth socket on Tier 10 gear. Mythic gear gets a subtle animated glow.

### 12.4 Ascension (Prestige)
- **Unlocks** after clearing the Hollow King once (recommended at Echo Rank 2).
- **Resets:** player level, gold, materials, non-legendary gear, Vow Points.
- **Keeps:** legendaries (and their Awakening), gems, campaign/Echo progress, achievements, settings.
- **Reward:** `Ember Marks = floor(sqrt(goldEarnedThisRun / 50,000)) + 2 × EchoRank`.
- **Ember Perks (permanent):** +5% Mining Power / gold / XP per rank (max 20), +5 level cap per 3 ranks (up to 100), start each run with a tier-3 gear set, and faster smelting.

---

## 13. Other Screens

### 13.1 Inventory
- Tabs: **Ore**, **Ingots**, **Gems**, **Gear**, **Misc/Key items**.
- Item cards show icon (shapes), name, count, sell value. Tap for details; **Sell**, **Sell All Duplicates**, **Equip**, **Socket**.
- Gear detail: stats, upgrade level, sockets, comparison against equipped. Lock icon prevents accidental selling.
- Sorting/filtering by tier and rarity.

### 13.2 Profile
- Character paperdoll: a simple figure made from shapes with the six equipped items drawn on it.
- Level, XP bar, all computed stats (with source breakdown on tap).
- **Vows** tree tab.
- **Lifetime stats:** blocks broken, clicks, gold earned, gems found, bosses cleared (with stars), dungeon runs, playtime.
- **Achievements** (~30): e.g. "Break 1,000 blocks", "First Legendary", "3★ all bosses", "Reach Echo Rank 5".
- **Settings:** volume, hold-to-mine, screen shake, number format, export/import save, hard reset.

### 13.3 Campaign Screen
- A vertical path map (SVG nodes connected by lines) showing 10 boss nodes: locked (grey), available (pulsing), cleared (gold with stars). Tap a node for a boss card: lore line, challenge description, your best stars, recommended level/gear tier, rewards and **Fight**.

---

## 14. Backend & Online Services

### 14.1 Principles
- **Offline-first.** Solo play is fully local. Online features need an account and a connection.
- **Server-authoritative where competition is involved:** PvP Arena, Abyss leaderboard, Guild Colossus, daily event claims, seasons, guild state.
- **Client-authoritative for solo progress,** protected by plausibility checks (Section 19). Cheating alone is possible but cannot reach competitive boards.
- **Self-hosted first:** one Docker Compose stack on a single machine. Design so it *could* scale later (stateless API, Redis optional).

### 14.2 Architecture
```
 Client (C++/raylib)
   ├─ HTTPS REST ─────────────▶ Caddy (TLS) ─▶ API (Drogon, C++) ─▶ PostgreSQL 16
   └─ WSS (chat, notifications) ▶                    │
                                                     └─▶ Redis (optional: rate limits, cache)
 Jobs inside the API process (scheduler): season rollover, weekly resets, leaderboard cache refresh,
 save retention, inactive-leader transfer. Nightly backup container: pg_dump.
```

### 14.3 API Conventions
- Base path `/v1`, JSON bodies, `Authorization: Bearer <access token>`.
- Errors: `{ "error": { "code": "string_code", "message": "human text" } }` with proper HTTP status.
- Every request sends `X-Client-Version`; the server can answer `426 Upgrade Required`.
- Requests that grant anything (event claims, arena attacks, inbox claims) require an **`Idempotency-Key`** header and are safe to retry.
- Lists use cursor pagination (`?limit=50&cursor=…`). Max body: 512 KB.
- `GET /healthz` (liveness, DB ping) and optional `GET /metrics` (Prometheus text).

### 14.4 Endpoint Overview
| Area | Endpoints |
|---|---|
| Auth | `POST /auth/guest`, `POST /auth/register` (link/upgrade), `POST /auth/login`, `POST /auth/refresh`, `POST /auth/logout`, `POST /auth/password`, `DELETE /account` |
| Time | `GET /time` (server UTC, used for clock offset) |
| Saves | `GET /saves/latest`, `PUT /saves` (conditional), `GET /saves/history`, `POST /saves/{revision}/restore` |
| Profile | `GET /profile/me`, `GET /profile/{publicId}`, `PUT /profile/settings` (timezone offset, display name) |
| Daily event | `POST /events/gem-hollow/start`, `POST /events/gem-hollow/claim` |
| Guilds | see Section 16.7 |
| Arena | see Section 17.9 |
| Leaderboards | see Section 18.4 |
| Inbox | `GET /inbox`, `POST /inbox/{id}/claim` (season rewards, Colossus rewards, moderator messages) |
| Reports | `POST /reports` (player, chat message, guild) |
| WebSocket | `GET /ws` (auth by first message) — channels: `guild:{id}`, `user:{id}` |

### 14.5 Database Schema (PostgreSQL, condensed)
Migrations are numbered SQL files (`0001_init.sql`, …) applied by a small runner in the server at startup, tracked in `schema_migrations`. All queries are parameterized.

```sql
CREATE EXTENSION IF NOT EXISTS citext;  CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE accounts (
  id BIGSERIAL PRIMARY KEY, public_id UUID NOT NULL DEFAULT gen_random_uuid() UNIQUE,
  username CITEXT UNIQUE, email CITEXT UNIQUE, password_hash TEXT,        -- null for unlinked guests
  guest_secret_hash TEXT, display_name TEXT NOT NULL,
  tz_offset_min SMALLINT NOT NULL DEFAULT 0, tz_changed_at TIMESTAMPTZ,
  status TEXT NOT NULL DEFAULT 'active',                                  -- active|muted|banned|deleted
  created_at TIMESTAMPTZ NOT NULL DEFAULT now(), last_seen_at TIMESTAMPTZ);

CREATE TABLE refresh_tokens (
  id BIGSERIAL PRIMARY KEY, account_id BIGINT REFERENCES accounts ON DELETE CASCADE,
  token_hash BYTEA UNIQUE NOT NULL, device_id UUID, expires_at TIMESTAMPTZ NOT NULL, revoked_at TIMESTAMPTZ);

CREATE TABLE saves (
  account_id BIGINT REFERENCES accounts ON DELETE CASCADE, revision BIGINT NOT NULL,
  schema_version INT NOT NULL, blob BYTEA NOT NULL, sha256 BYTEA NOT NULL,
  device_id UUID, device_name TEXT, progress_score BIGINT NOT NULL,
  client_saved_at TIMESTAMPTZ, created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  PRIMARY KEY (account_id, revision));                                    -- latest = max(revision)

CREATE TABLE profiles (                       -- public, server-derived snapshot (never trusted from client directly)
  account_id BIGINT PRIMARY KEY REFERENCES accounts ON DELETE CASCADE,
  level INT, echo_rank INT, max_depth INT, blocks_broken BIGINT, power_score BIGINT,
  arena_build JSONB, arena_build_updated_at TIMESTAMPTZ, guild_id BIGINT,
  hidden BOOLEAN NOT NULL DEFAULT FALSE, updated_at TIMESTAMPTZ NOT NULL DEFAULT now());

CREATE TABLE guilds (
  id BIGSERIAL PRIMARY KEY, name CITEXT UNIQUE NOT NULL, tag CITEXT UNIQUE NOT NULL,
  emblem JSONB NOT NULL, description TEXT, join_mode TEXT NOT NULL DEFAULT 'request',  -- open|request|invite
  min_level INT NOT NULL DEFAULT 1, level INT NOT NULL DEFAULT 1, xp BIGINT NOT NULL DEFAULT 0,
  treasury JSONB NOT NULL DEFAULT '{}', leader_id BIGINT REFERENCES accounts, created_at TIMESTAMPTZ NOT NULL DEFAULT now());
CREATE TABLE guild_members (
  guild_id BIGINT REFERENCES guilds ON DELETE CASCADE, account_id BIGINT PRIMARY KEY REFERENCES accounts ON DELETE CASCADE,
  role TEXT NOT NULL DEFAULT 'member', joined_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  contribution_week BIGINT NOT NULL DEFAULT 0, contribution_total BIGINT NOT NULL DEFAULT 0);
CREATE TABLE guild_applications (guild_id BIGINT, account_id BIGINT, message TEXT, created_at TIMESTAMPTZ DEFAULT now(), PRIMARY KEY (guild_id, account_id));
CREATE TABLE guild_messages (id BIGSERIAL PRIMARY KEY, guild_id BIGINT, account_id BIGINT, body TEXT NOT NULL, created_at TIMESTAMPTZ NOT NULL DEFAULT now());
CREATE TABLE guild_colossus (id BIGSERIAL PRIMARY KEY, guild_id BIGINT, week_key TEXT, level INT, max_hp NUMERIC, hp_remaining NUMERIC, defeated_at TIMESTAMPTZ, UNIQUE (guild_id, week_key));
CREATE TABLE guild_colossus_contrib (colossus_id BIGINT, account_id BIGINT, damage NUMERIC NOT NULL DEFAULT 0, sessions_today INT NOT NULL DEFAULT 0, last_session_day DATE, PRIMARY KEY (colossus_id, account_id));

CREATE TABLE seasons (id SERIAL PRIMARY KEY, starts_at TIMESTAMPTZ NOT NULL, ends_at TIMESTAMPTZ NOT NULL, status TEXT NOT NULL DEFAULT 'active');
CREATE TABLE arena_ratings (
  season_id INT REFERENCES seasons, account_id BIGINT REFERENCES accounts, rating INT NOT NULL DEFAULT 1000,
  wins INT NOT NULL DEFAULT 0, losses INT NOT NULL DEFAULT 0, tickets SMALLINT NOT NULL DEFAULT 8,
  tickets_updated_at TIMESTAMPTZ NOT NULL DEFAULT now(), defense_loss_today INT NOT NULL DEFAULT 0, defense_stance TEXT NOT NULL DEFAULT 'balanced',
  PRIMARY KEY (season_id, account_id));
CREATE INDEX ON arena_ratings (season_id, rating DESC);
CREATE TABLE arena_battles (
  id BIGSERIAL PRIMARY KEY, season_id INT, attacker_id BIGINT, defender_id BIGINT, seed BIGINT NOT NULL,
  attacker_stance TEXT, defender_stance TEXT, attacker_build JSONB, defender_build JSONB,
  winner TEXT NOT NULL, delta_attacker INT, delta_defender INT, log BYTEA, created_at TIMESTAMPTZ NOT NULL DEFAULT now());

CREATE TABLE leaderboard_scores (
  board TEXT NOT NULL, window_key TEXT NOT NULL, account_id BIGINT NOT NULL, score BIGINT NOT NULL, updated_at TIMESTAMPTZ NOT NULL DEFAULT now(),
  PRIMARY KEY (board, window_key, account_id));
CREATE INDEX ON leaderboard_scores (board, window_key, score DESC, updated_at ASC);

CREATE TABLE inbox (id BIGSERIAL PRIMARY KEY, account_id BIGINT, kind TEXT, payload JSONB NOT NULL, created_at TIMESTAMPTZ NOT NULL DEFAULT now(), claimed_at TIMESTAMPTZ);
CREATE TABLE anomaly_flags (id BIGSERIAL PRIMARY KEY, account_id BIGINT, kind TEXT, severity SMALLINT, details JSONB, created_at TIMESTAMPTZ NOT NULL DEFAULT now(), resolved_at TIMESTAMPTZ);
CREATE TABLE reports (id BIGSERIAL PRIMARY KEY, reporter_id BIGINT, target_type TEXT, target_id TEXT, reason TEXT, created_at TIMESTAMPTZ NOT NULL DEFAULT now(), handled_at TIMESTAMPTZ);
CREATE TABLE idempotency_keys (account_id BIGINT, key TEXT, response JSONB, created_at TIMESTAMPTZ NOT NULL DEFAULT now(), PRIMARY KEY (account_id, key));
```

---

## 15. Accounts, Save Sync & Autosave

### 15.1 Accounts
- **Guest-first:** on first launch the client silently creates a guest account (`POST /auth/guest`) and stores a device secret locally (OS credential store if available, otherwise a user-only-readable file). The player can play immediately.
- **Link account:** email + password (Argon2id via libsodium) and a unique username (3–16 chars, profanity-filtered). Email verification is a later feature. Linking is prompted at Level 10 (skippable) and always available in Settings.
- **Tokens:** access token (JWT, 15 min) + rotating refresh token (30 days, stored hashed, revocable per device).
- **Account deletion:** `DELETE /account` removes the account, saves, and personal data; guild memberships are cleared, leaderboard rows removed.

### 15.2 Local Autosave (always on)
Autosave every **10 s** when the state is dirty, on every screen change, on window close, and on minigame completion. Atomic writes, 3 rolling backups (Section 2.4).

### 15.3 Cloud Autosave & Sync
- **Push** when logged in and dirty: every **60 s**, immediately after major events (first boss clear, legendary drop, ascension, guild join/leave), on focus loss, and on exit (best effort, 3 s timeout).
- **Pull** on launch (after login) and when "Sync now" is pressed.
- **Payload:** `schema_version`, `device_id`, `device_name`, `base_revision`, `progress_score`, `client_saved_at`, `sha256`, and the zstd-compressed JSON save (typically 10–60 KB).
- **Failure handling:** exponential backoff with jitter (5 s → 5 min), only the *latest* state is ever queued, and play is never interrupted.
- **Server-side history:** keep the last 20 revisions plus one per day for 30 days. Players can restore any of them from Settings.

### 15.4 Conflict Resolution
The push is conditional on `base_revision`.
1. Server revision == `base_revision` → accepted, new revision returned.
2. Server revision > `base_revision` and the **local state is unchanged** since the last sync → silently fast-forward (download cloud save).
3. Both changed → `409 Conflict` with the cloud save's metadata. The client shows a **Choose Save** dialog with both sides' level, playtime, gold, bosses cleared, ascensions, device name and last-played time, and recommends the one with the higher `progress_score`. The loser is kept in history (server) and as a local backup, so the choice is reversible.
4. Stretch: automatic merge of *monotonic* fields only (lifetime stats, achievements, best boss stars). Never auto-merge currencies or inventory.

`progress_score = ascensions×10^9 + echoRank×10^7 + bossesCleared×10^6 + level×10^3 + floor(log2(lifetimeGold+1))`.

### 15.5 Sync UI
- Top-bar cloud icon: **Synced ✓ / Syncing… / Offline / Conflict! / Error**.
- Settings → Account: link account, log in, log out, Sync now, Download cloud save, Upload local save, Restore from history, Export/Import file, Delete account.
- Server-issued rewards (season prizes, Colossus rewards) arrive through the **Inbox** and are applied to the local save on claim (then synced normally).

---

## 16. Guilds

Unlocked at **player Level 10** and requires an account.

### 16.1 Basics
- **Identity:** unique name (3–20 chars), tag (2–4 chars), description, **emblem** built from shapes (base shape + colors + a symbol shape, stored as JSON and drawn procedurally), join mode (**Open / Request / Invite-only**), minimum level.
- **Size:** max **30 members**.
- **Creation:** Level 10 and 5,000 gold. Leaving a guild adds a 12-hour cooldown before joining another.

| Role | Permissions |
|---|---|
| **Leader** (1) | Everything, transfer leadership, disband, edit settings |
| **Officer** | Accept/reject applications, invite, kick members, start Guild Projects, pin chat messages |
| **Member** | Donate, fight the Colossus, chat, leave |

If the Leader is inactive for **14 days**, leadership auto-transfers to the most senior active Officer (or member).

### 16.2 Guild Progression
Guild XP comes from donations, Colossus damage and member Arena wins. Guild level (1–20) unlocks **economy and mining perks only**. Guild perks never apply to Arena stats, so PvP stays fair.

| Guild Lv | Perk (cumulative) |
|---|---|
| 2 | +2% XP |
| 4 | +2% gold from sales and the dungeon |
| 6 | +1 Gem Hollow swing |
| 8 | +3 Luck |
| 10 | +5% Mining Power |
| 12 | +1 Colossus session per day |
| 15 | +5% Arcane Dust chance; unlock emblem frames |
| 20 | +10% Mining Power total; golden guild banner |

### 16.3 Donations & Projects
- Members donate gold, ingots or Arcane Dust to the **Treasury** (daily caps by player level to stop pure pay-to-rank style hoarding). Donating grants personal **Guild Coins** and guild XP.
- Officers start a **Guild Project** to raise the guild level. Cost for level L: `gold = 10,000 × 1.7^(L−1)`, `ingots = 50 × L` of tier `ceil(L/2)`.
- **Guild Coins shop:** Arcane Dust bundles, cosmetic emblem frames, pickaxe trail effects (particle presets), name-plate borders.

### 16.4 The Sleeping Colossus (Weekly Co-op Boss)
- A giant block with a **shared HP pool**, reset every Monday 00:00 UTC. Colossus level rises by 1 each week it is defeated, and drops by 1 if it is not (min 1).
- **Sessions:** each member gets **3 sessions per day** (+ guild perks). A session is a **30-second mining frenzy** on the Colossus using the player's normal click-damage formula (hardness ignored), hit rate capped at 12 hits/s.
- **HP target:** computed at week start from members' recorded power: `maxHp = 0.7 × Σ(expected weekly damage per member)` so it is beatable if about 70% of the guild participates.
- **Server validation:** the client submits `{clicks, damage, durationMs}`; the server clamps damage to `profile power × critAvg × min(clicks, 12/s × duration)` and adds it to the pool.
- **Rewards:** on defeat, everyone with ≥ 1 session receives baseline rewards; the top 3 contributors get extras. Rewards arrive in the **Inbox** (Boss Essence, Arcane Dust, gems, Guild Coins). If not defeated, rewards scale down proportionally to HP dealt.

### 16.5 Guild Chat
- WebSocket channel `guild:{id}`, 200-char messages, **1 message/second** per user, last 200 messages stored. System lines for joins, leaves, promotions and Colossus events.
- Server-side profanity filter, per-user mute, and "Report" on any message. Muted accounts (moderator action) cannot post. Chat can be hidden in Settings.

### 16.6 Guild UI
Tabs: **Overview** (level, XP bar, perks, emblem), **Members** (roles, last active, weekly contribution), **Colossus** (HP bar, your sessions left, contributor board, weekly countdown), **Treasury** (donate, projects), **Chat**, **Manage** (officers+). Players without a guild see **Browse / Search / Create**.

### 16.7 Guild Endpoints
`GET /guilds?query=&minLevel=` · `POST /guilds` · `GET /guilds/{id}` · `POST /guilds/{id}/join` (join or apply) · `POST /guilds/{id}/applications/{accountId}/accept|reject` · `POST /guilds/{id}/members/{accountId}/promote|demote|kick` · `POST /guilds/leave` · `POST /guilds/{id}/transfer` · `PUT /guilds/{id}` · `DELETE /guilds/{id}` · `POST /guilds/{id}/donate` · `POST /guilds/{id}/projects` · `GET /guilds/{id}/colossus` · `POST /guilds/{id}/colossus/session/start` · `POST /guilds/{id}/colossus/session/finish` · `GET /guilds/{id}/messages?cursor=` · WS message `chat.send`.

---

## 17. PvP Arena (Asynchronous)

Unlocked at **player Level 15** (or after Boss 2) and requires an account.

### 17.1 Concept
You fight **snapshots** of other players' builds using the same **automatic turn-based combat** as the dungeon (Section 11). The **server runs the simulation** and returns a battle log; the client just replays it. No real-time netcode needed, and results cannot be forged.

### 17.2 Arena Build (Snapshot)
- Contains: level, equipped gear (template, upgrade level, gems, enchant, awakening), Vows, and legendary IDs. The server **recomputes all combat stats itself** with `core::stats` from the shared data files; it never trusts client-provided totals.
- **Arena stat rules:** level, gear, gems, Vows and legendary combat effects count. **Guild perks, Ascension perks and mining-only effects do not.** Arena power is compressed so gear tier matters but never makes a low-rated matchup a guaranteed loss: `arenaStat = base × (1 + 0.6 × tierNormalized)` (tune in data).
- Uploaded when gear/level changes (debounced 30 s) and on login: `PUT /arena/build`. The server checks that the build is consistent with the player's latest cloud save (equipped items must exist there) and with progression limits.

### 17.3 Battle Flow
1. Open **Arena**: rating, tier, tickets, season timer.
2. **Find opponents** → server returns **3 candidates** within ±150 rating (widening if the pool is small; **NPC ghost builds** fill gaps). Each shows name, guild tag, rating, power, equipped-gear icons, and win/loss rating preview. Guildmates are excluded from matchmaking.
3. Pick a **stance** (Section 17.4), then **Attack**.
4. `POST /arena/attack` → server fetches the defender's *current* snapshot, sets a fresh seed, simulates, applies rating changes, and returns `{log, result, rewards, newRating}`.
5. The client replays at ×1/×2/×4 or skips to the result. A full battle-log record is stored for the defender's "Defense Log".

### 17.4 Stances (Light Decision Layer)
Each side chooses a stance at battle start: **Aggressive** (+10% Attack, −10% Defense), **Balanced**, **Guarded** (+10% Defense, −10% Attack). A triangle bonus adds **+8% damage** to the winner of the matchup: *Aggressive beats Guarded, Guarded beats Balanced, Balanced beats Aggressive.* The defender's stance is set in advance (default Balanced) and hidden. The attacker sees a hint ("used Guarded in 60% of recent defenses") so reading opponents has value without being pure guessing.

### 17.5 Rating & Tiers
- **Elo variant:** `K = 32` (`K = 24` above 1800), minimum gain/loss 5. The defender receives **50%** of the attacker's delta.
- **Defender protection:** a defender can lose at most **30 rating per day** (tracked in `defense_loss_today`). The same pair can fight at most **3 times per 24 h**.
- **Tiers:** Squire (0–999), Knight (1000–1399), Baron (1400–1799), Count (1800–2199), Duke (2200–2599), **High King** (2600+ and top 100). Each has a shape-built emblem.

### 17.6 Tickets & Rewards
- **Tickets:** max 8, +1 per 2 hours. Each attack costs 1.
- **Win:** Arena Coins `30 + 5×tierIndex`, gold `50 × 3^(bestTier-1)`, guild XP. **Loss:** 10 Arena Coins.
- **Arena Shop (Arena Coins):** Arcane Dust bundles, limited weekly Boss Essence, Rough gem chests (low odds for high rarities), cosmetics (titles, name borders, pickaxe trails).

### 17.7 Seasons
14-day seasons (`seasons` table). A server job at season end: snapshots the final leaderboard, sends **tier-based rewards** to the Inbox (gold, Boss Essence, cosmetic title for the tier, exclusive border for the top 100), then starts the next season with a soft reset `newRating = 1000 + (old − 1000) × 0.5`.

### 17.8 Arena UI
Tabs: **Battle** (banner with rating/tier emblem, tickets, timer, 3 opponent cards), **Defense Log** (who attacked, results, replay button, "Revenge" shortcut), **Shop**, **Ranks** (leaderboard, Section 18). The replay reuses the dungeon battle scene.

### 17.9 Arena Endpoints
`GET /arena/status` · `PUT /arena/build` · `PUT /arena/defense-stance` · `POST /arena/opponents` · `POST /arena/attack` · `GET /arena/battles?cursor=` · `GET /arena/battles/{id}` · `GET /arena/shop` · `POST /arena/shop/buy`.

**Stretch (out of scope for v2):** live real-time duels over WebSocket.

---

## 18. Leaderboards

### 18.1 Boards
| Board | Metric | Window | Trust |
|---|---|---|---|
| **Arena Rating** | Season rating | Season | Server-authoritative |
| **Abyss** | Highest floor (weekly seed) | Weekly | Server-authoritative (the server re-simulates the submitted build; max 3 submissions/day) |
| **Guild Colossus** | Damage per player and guild | Weekly | Server-authoritative (clamped) |
| **Guilds** | Guild level + XP | All-time | Server-authoritative |
| **Echo Rank** | Highest Echo Rank | All-time | Save-derived, plausibility-checked |
| **Deep Strata** | Deepest depth | All-time | Save-derived, plausibility-checked |
| **Weekly Miner** | Blocks broken this week | Weekly | Save-derived, rate-capped |
| **Level / Ascensions** | Level, Ascension count | All-time | Save-derived |

Boss-score boards (best CPS, best rhythm accuracy) are **not global**: they show only among guildmates, because minigame scores are too easy to fake without replays.

### 18.2 Behavior
- Each board offers **Top 100**, **Around me** (±5), and **Guild** scope (members only).
- Ties are broken by whoever reached the score first (`updated_at ASC`).
- Entries show display name, guild tag, emblem, tier badge and value; tapping opens a public profile card (level, guild, equipped-gear icons, achievements count, Arena tier).
- Reset: weekly boards on **Monday 00:00 UTC**. Top 3 weekly finishers receive a small Inbox reward (cosmetic title + Arcane Dust).
- Accounts flagged as suspicious (`profiles.hidden = true`) are excluded from public boards but still see their own rank.

### 18.3 Data Flow
Save-derived values are extracted **by the server** from each accepted save (`profiles` update + `leaderboard_scores` upsert), after plausibility checks. The client never posts leaderboard scores directly for these boards. Results are cached for 30–60 s (in-memory, or Redis when the `scale` profile is on).

### 18.4 Leaderboard Endpoints
`GET /leaderboards` (list boards) · `GET /leaderboards/{board}?window=&scope=&limit=&cursor=` · `GET /leaderboards/{board}/me` · `POST /abyss/submit` (build + idempotency key).

---

## 19. Security & Anti-Cheat

**Threat model.** A clicker with client-held progress can be edited with a hex editor or memory tool. Goals: (1) keep competitive modes fair, (2) never punish honest players, (3) leave offline solo play unrestricted. "Cheat in your own save if you like, but you can't take it online."

**Transport & auth**
- TLS everywhere (Caddy in production). Passwords: Argon2id. Short-lived JWTs, rotating refresh tokens, per-device revocation.
- Rate limits (token bucket, per IP and per account) on all endpoints; stricter on auth, chat, and arena attacks.
- Strict JSON schema validation on every request, size limits, parameterized SQL only, idempotency keys on rewarding calls.
- Secrets only from environment / Docker secrets, never in the repo.

**Save plausibility engine** (`core::validateSaveTransition(prev, next, elapsed)`), run on every accepted push:
- Lifetime counters must never decrease; gold/hour, XP/hour and blocks/hour must stay under generous upper bounds derived from the fastest possible play (hit cap × best power).
- Level must be reachable from XP; gear tier ≤ what layers/recipes allow at that progression; legendaries only with the matching boss cleared; stars ≤ 3; inventory counts within bounds.
- Each violation adds to an **anomaly score** (`anomaly_flags`). **Soft flag** → hidden from public boards, still syncs normally. **Hard flag** → queued for manual review. The server never deletes saves automatically.

**Server-authoritative modes:** Arena, Abyss, Colossus, Gem Hollow claims (server seed and per-day lock), season rewards, guild state.

**Local save signing:** an HMAC over the local save file is a deterrent against casual editing only (the key lives in the client binary). It is not a security boundary, so do not rely on it.

**Moderation & abuse:** report endpoint, profanity filter (names, guild names, chat), mute/ban statuses, an admin CLI (`tools/admin`: ban, mute, rename guild, resolve flags, view audit log). Same-device / same-IP alt-account heuristics feed anomaly flags for Arena win-trading.

**Privacy:** minimal personal data (email optional), account deletion endpoint, log retention 30 days, no third-party analytics unless explicitly added.

---

## 20. Docker & Deployment

**Is Docker a good idea?** *Yes for the server side:* the C++ toolchain, vcpkg dependencies, PostgreSQL and the reverse proxy become reproducible with one command, the same images run on a home server or VPS, and backups/migrations are easy to script. *No for the game client:* it is a native GUI + audio app, so build it natively on CI runners for each OS (an optional Linux build container is fine for CI only).

### 20.1 Files
```
docker/
  server.Dockerfile              # multi-stage build
  docker-compose.yml             # production-like stack
  docker-compose.dev.yml         # dev overrides (exposed ports, Adminer, debug logs)
  caddy/Caddyfile
  scripts/backup.sh  restore.sh
.env.example
```

### 20.2 Server Image
- **Stage 1 (builder):** Ubuntu 24.04 + build-essential, CMake, Ninja, git, pkg-config, vcpkg (with BuildKit cache mounts for the vcpkg binary cache), builds `server` in Release.
- **Stage 2 (runtime):** minimal Ubuntu/Debian with only runtime libs, the server binary, `/data`, and `migrations/`; runs as a **non-root user**; `HEALTHCHECK` on `/healthz`.

### 20.3 Compose Stack (sketch)
```yaml
services:
  db:
    image: postgres:16-alpine
    restart: unless-stopped
    environment: { POSTGRES_DB: emberdeep, POSTGRES_USER: emberdeep, POSTGRES_PASSWORD_FILE: /run/secrets/db_password }
    volumes: [ "pgdata:/var/lib/postgresql/data" ]
    healthcheck: { test: ["CMD-SHELL","pg_isready -U emberdeep"], interval: 5s, retries: 10 }
    networks: [ internal ]
    secrets: [ db_password ]
  api:
    build: { context: .., dockerfile: docker/server.Dockerfile }
    restart: unless-stopped
    depends_on: { db: { condition: service_healthy } }
    environment:
      DATABASE_URL: postgres://emberdeep@db:5432/emberdeep
      JWT_SECRET_FILE: /run/secrets/jwt_secret
      REDIS_URL: ""            # set to redis://redis:6379 when the scale profile is on
      RUN_MIGRATIONS: "true"
    networks: [ internal, web ]
    secrets: [ db_password, jwt_secret ]
  caddy:                        # profile: prod  (TLS + reverse proxy on 80/443)
    image: caddy:2
    profiles: [ prod ]
    ports: [ "80:80", "443:443" ]
    volumes: [ "./caddy/Caddyfile:/etc/caddy/Caddyfile", "caddy_data:/data" ]
    networks: [ web ]
  redis:                        # profile: scale
    image: redis:7-alpine
    profiles: [ scale ]
    command: ["redis-server","--appendonly","yes"]
    networks: [ internal ]
  backup:                       # nightly pg_dump, 14-day retention
    image: postgres:16-alpine
    volumes: [ "backups:/backups", "./scripts:/scripts:ro" ]
    entrypoint: ["/scripts/backup.sh"]
    networks: [ internal ]
volumes: { pgdata: {}, backups: {}, caddy_data: {} }
networks: { internal: { internal: true }, web: {} }
secrets:
  db_password: { file: ./secrets/db_password.txt }
  jwt_secret:  { file: ./secrets/jwt_secret.txt }
```
The database is **never** published to the host in production. Dev override publishes `5432` and adds `adminer`.

### 20.4 Workflow
- **Local dev:** `docker compose up -d db` and run the API natively for debugging, or `docker compose -f docker-compose.yml -f docker-compose.dev.yml up --build` for the full stack.
- **Client config:** `config.json` holds `server_url` (default `http://localhost:8080`); if the server is unreachable the game runs in Offline mode.
- **Deploy/update:** `docker compose pull && docker compose up -d --build`; migrations run on API start.
- **Backups:** nightly `pg_dump` to the `backups` volume with 14-day retention; document how to copy off-machine and how to run `restore.sh`. **Test a restore in CI or manually before release.**
- **Self-hosting notes:** expose only 80/443 (Caddy), keep Postgres internal, set a firewall, use a dynamic-DNS name if hosting at home, and monitor with `/healthz` plus container logs (Prometheus optional).
- **CI (GitHub Actions or similar):** matrix build + test `core` and `client` on Windows/Linux/macOS; build the server image; run **integration tests** against `docker compose -f docker-compose.test.yml` (API + fresh DB): auth, save conflict flow, guild lifecycle, arena attack, leaderboard queries.

---