# Emberdeep Project Guide

This document is the technical and gameplay reference for the current Emberdeep
repository. It describes the architecture, source layout, runtime data flow,
game systems, boss campaign, persistence, backend, controls, tests, and the
work completed during the development session that produced this version.

The repository is a C++20/raylib mining, progression, crafting, equipment,
dungeon, and ten-boss campaign game. The deterministic gameplay rules live in a
shared `core` library. The graphical client, terminal playtest, and server
reuse that library rather than maintaining separate gameplay implementations.

## 1. Project identity and design principles

Emberdeep is an offline-first procedural mining/clicker game set in a
multi-layer underground world. The intended loop is:

1. Mine the current block.
2. Collect ore, gems, gold, trophies, experience, and progression currencies.
3. Smelt ore and craft equipment.
4. Upgrade and equip gear.
5. Enter dungeons and campaign boss challenges.
6. Unlock deeper layers and stronger materials.
7. Complete the ten-boss campaign and continue through post-campaign systems.

Important design principles:

- **Shared rules:** gameplay calculations are in `core`, not duplicated in the
  clients.
- **Determinism:** seeded PCG32 randomness is used for minigames and dungeon
  simulation so tests and playtest scenarios can be reproduced.
- **Procedural visuals:** the graphical client uses raylib primitives and text;
  boss visuals do not depend on a sprite-heavy boss asset pipeline.
- **Offline-first saves:** local saves remain usable without the backend.
- **Progression gates:** layers and bosses require appropriate level,
  pickaxe, and previous-campaign progress.
- **Atomic persistence:** save writes use temporary files and replacement
  rather than exposing partially-written save files.
- **Data-driven definitions:** materials, layers, gems, bosses, perks, and
  dungeon zones also have JSON/data representations, while the current C++
  core has compiled default definitions used by the clients and tests.

The repository intentionally ignores TypeScript-related implementation hints
from earlier specification drafts. The implemented project is C++20.

## 2. Top-level repository layout

```text
.
├── CMakeLists.txt
├── CMakePresets.json
├── vcpkg.json
├── README.md
├── EMBERDEEP_PROJECT_GUIDE.md       # This document
├── EMBERDEEP GAME SPEC NEW.md       # Updated game specification
├── EMBERDEEP GAME SPEC NEW copy.md  # Copy of the updated specification
├── BOSS_8_IGNATHIA_SPEC.md          # Heartpierce source specification
├── bosses.md                        # Boss implementation plan/reference
├── core/
│   ├── include/ed/                  # Public shared-core headers
│   └── src/                        # Shared gameplay and persistence code
├── client/
│   ├── src/app/main.cpp             # Main raylib application
│   ├── src/playtest/main.cpp        # Terminal playtest executable
│   └── src/screens/                 # Lightweight client screen support
├── server/
│   ├── src/                         # API, save backend, PostgreSQL backend
│   ├── migrations/                  # PostgreSQL schema migrations
│   └── postgres-init/               # Database role/init/upgrade scripts
├── tests/
│   └── core/                        # Catch2 unit and regression tests
├── data/                            # JSON definitions and content data
├── deploy/                          # Caddy, Docker, and operations docs
├── sprites/                         # Mining/material sprite assets
└── docs/
    └── DECISIONS.md                 # Recorded architecture decisions
```

The checked-in `build/` directory is generated output and FetchContent state.
It is not source architecture and should not be edited manually. Local files
such as `.env`, `emberdeep.profile`, and `.sav` files contain environment or
player state and must not be committed.

## 3. Build graph and target relationships

The root `CMakeLists.txt` enables C++20, exports compile commands, adds `core`,
optionally adds `client`, always adds `server`, and adds `tests`.

### 3.1 `emberdeep_core`

Defined in `core/CMakeLists.txt` and built from:

```text
campaign.cpp
boss_minigames.cpp
crafting.cpp
dungeon.cpp
formulas.cpp
game_state.cpp
inventory.cpp
postgame.cpp
pcg.cpp
save.cpp
stats.cpp
cloud.cpp
```

This library contains the authoritative rules for:

- player/game state;
- mining and block generation;
- material and progression formulas;
- inventory, equipment, sockets, and gems;
- crafting and upgrades;
- campaign progression and boss outcomes;
- all boss minigame state machines;
- dungeons and post-game systems;
- local save serialization;
- cloud-save client communication.

### 3.2 `emberdeep_client`

Defined in `client/CMakeLists.txt`. It links to `emberdeep_core` and includes
screen support in `client/src/screens/main_screen.cpp`.

The same CMake target produces:

- `emberdeep_playtest`, the terminal-oriented playtest;
- `emberdeep_graphical`, the raylib graphical client.

The graphical target additionally requires raylib.

### 3.3 `emberdeep_server` and `emberdeep_api`

`emberdeep_server` links to `emberdeep_core` and contains the save backend.
`emberdeep_api` contains the HTTP API and links OpenSSL for hashing. PostgreSQL
support is optional and enabled with:

```text
-DEMBERDEEP_ENABLE_POSTGRES=ON
```

When enabled, the API also compiles `postgres_backend.cpp` and links the
PostgreSQL client library.

### 3.4 Tests

`tests/CMakeLists.txt` fetches Catch2 3.7.1 and builds
`emberdeep_core_tests`. Tests link only against the shared core and Catch2.
CTest discovers all Catch2 cases automatically.

## 4. Shared-core file reference

### `core/include/ed/types.hpp`

Defines reusable data-only definitions:

- `MaterialDef`;
- `LayerDef`;
- `GemDef`;
- `VowNodeDef`;
- `PlayerProgress`;
- `MiningResult`;
- list aliases such as `MaterialList`, `LayerList`, `GemList`, and
  `VowNodeList`.

### `core/include/ed/data_model.hpp`

Provides compiled default content:

- ten materials from Ashen Stone through Aetherium;
- ten layers from Ashen Warrens through The Aether Vault;
- ten gems;
- vow nodes for Delver, Warrior, and Scholar branches.

The default gem list is:

| ID | Name | Rarity | Effect |
|---|---|---|---|
| `amber` | Amber | common | +1 Luck |
| `garnet` | Garnet | common | +2% Attack |
| `jade` | Jade | uncommon | +2% Max HP |
| `moonstone` | Moonstone | uncommon | +0.5% Crit Chance |
| `sapphire` | Sapphire | rare | +1% Focus |
| `bloodstone` | Bloodstone | rare | 1% lifesteal |
| `star-ruby` | Star Ruby | epic | +3% Mining Power |
| `arcane-opal` | Arcane Opal | epic | +3% XP gain |
| `dragon-eye` | Dragon's Eye | legendary | +2 Pierce, +5% Crit Damage |
| `aether-diamond` | Aether Diamond | legendary | +1.5% to all stats |

`data/gems.json` contains the corresponding data-driven form. The JSON and
compiled defaults should be kept synchronized when content changes.

### `core/include/ed/game_state.hpp` and `core/src/game_state.cpp`

`PlayerState` stores level, XP, gold, Arcane Dust, boss/post-game currencies,
vow points, mining counters, boss clears, ascensions, echo rank, deepest depth,
pickaxe tier, and current layer.

`GameState` contains:

- `schemaVersion`;
- `PlayerState player`;
- a dirty flag;
- a save identifier.

The file also owns layer unlock checks, XP application, and game-state
serialization/deserialization.

### `core/include/ed/inventory.hpp` and `core/src/inventory.cpp`

The inventory model contains maps for:

- ore;
- ingots;
- gems;
- trophies.

It also contains a vector of `GearPiece` objects and a `GearSet` of equipped
item IDs.

`GearPiece` contains:

- slot and item ID;
- display name;
- tier;
- upgrade level;
- total socket count;
- legendary flag;
- ordered `socketedGems` IDs.

Important operations:

- `addItemToInventory`;
- `removeItemFromInventory`;
- `countItem`;
- `equipGear`;
- `gearSocketCount`;
- `insertGem`;
- `removeGem`.

Socket rules currently grant:

- tier 1-3: zero sockets;
- tier 4-6: one socket;
- tier 7-9: two sockets;
- tier 10: three sockets;
- legendary gear receives one additional socket.

Insertion validates that the gem exists in inventory and that the piece has an
empty socket before consuming one gem. Removal validates the socket index,
returns the gem to inventory, and removes it from the equipment piece.

The graphical Inventory screen currently uses:

- `G` to insert the first available gem into the selected piece;
- `R` to remove the selected piece's last socketed gem;
- `E` to equip;
- `U` to upgrade;
- `A`/`D` to select gear;
- `W`/`S` or mouse wheel to scroll.

The current UI deliberately chooses the first gem in the map rather than
opening a gem-selection dialog. This is functional but is a future UX
improvement opportunity.

### `core/include/ed/stats.hpp` and `core/src/stats.cpp`

`PlayerStats` is the derived-stat output used by mining, bosses, dungeons, and
the Profile screen. It includes:

- mining power;
- mining-power percentage bonus;
- attack;
- defense;
- max HP;
- speed;
- luck;
- pierce;
- crit chance basis points;
- crit damage basis points;
- XP bonus basis points;
- focus basis points;
- lifesteal basis points.

`calculatePlayerStats` starts with level-based base stats, applies each
equipped gear piece, applies upgrade and tier multipliers, then applies all
socketed gem effects from equipped gear.

Percentage gem effects are accumulated first and applied after all equipment
totals are assembled. This avoids order-dependent rounding:

- Garnet contributes attack percentage basis points;
- Jade contributes max-HP percentage basis points;
- Aether Diamond contributes percentage bonuses across the supported stats.

Basis points use 100 basis points per percentage point. For example, 500
crit-chance basis points means 5%.

### `core/include/ed/formulas.hpp` and `core/src/formulas.cpp`

Contains shared formulas for:

- material generation and layer scaling;
- mining damage;
- XP progression;
- progress score;
- reward calculations and related numeric rules.

The mining/client code calls these formulas rather than implementing local
copies.

### `core/include/ed/mining.hpp` and `core/src/mining.cpp`

Owns block generation and mining interactions. Layer/material definitions
determine hardness, HP, XP, sell value, and material identity. Mining applies
player stats, hardness, critical behavior, weak spots, block damage, resource
rewards, and progression counters.

The graphical client uses this layer for the main mining screen. Successful
layer changes spawn a new block; failed or boundary/locked navigation does not
reset the current block.

### `core/include/ed/crafting.hpp` and `core/src/crafting.cpp`

Defines craft recipes and operations for:

- recipe generation;
- smelting four matching ore into one ingot;
- crafting gear;
- upgrade cost calculation;
- gear upgrades.

Crafting consumes tier-specific ingots, trophies, gold, and—in recipes that
require it—a gem. Crafted equipment receives its tier-derived socket count.
Amulets are the current recipe family that requires a gem during crafting;
that consumed gem is separate from gems later inserted into sockets.

Upgrades consume gold and Arcane Dust and increase `upgradeLevel`, which feeds
into equipment stat multipliers.

### `core/include/ed/dungeon.hpp` and `core/src/dungeon.cpp`

Defines dungeon player stats, zones, floors, and deterministic dungeon run
simulation. A run returns gold/XP/rewards and can find a gem. Dungeon content
is represented in `data/dungeon_zones.json` and compiled/default structures.

The terminal playtest exercises deterministic dungeon behavior, while the
graphical client presents a simple zone-selection/run screen.

### `core/include/ed/campaign.hpp` and `core/src/campaign.cpp`

Defines:

- `BossChallenge`;
- `BossDef`;
- `BossAttempt`;
- `BossOutcome`;
- `CampaignState`.

The current challenge enum is:

```text
FrenzyDig
BoneDuel
CinderDance
HeadHunt
ParryDuel
RuneRecall
StarTrace
Heartpierce
SoulDodge
FiveTrials
```

Campaign logic handles:

- ten-boss definitions;
- sequential unlocks;
- level and previous-clear requirements;
- score thresholds;
- star ratings;
- first-clear rewards;
- repeat-clear rewards;
- campaign persistence.

The code uses `Heartpierce` for Ignathia. The legacy `data/bosses.json` entry
still says `"wyrmfire-run"` for Ignathia, so that JSON entry is stale relative
to the current C++ implementation and should be updated if JSON becomes an
active runtime source.

Current campaign order:

| # | Boss | Layer | Challenge | Level |
|---:|---|---|---|---:|
| 1 | Grubnok, the Burrow-King | Ashen Warrens | Frenzy Dig | 1 |
| 2 | Sir Rattlebone | Copper Crypt | Bone Duel | 5 |
| 3 | Morwenna the Ashen Witch | Iron Hollows | Cinder Dance | 10 |
| 4 | Thessaly, the Bog Hydra | Silvermire | Head Hunt | 15 |
| 5 | Aldric, the Gilded Knight | Aurelite Keep | Parry Duel | 20 |
| 6 | Isolde, Runewarden | Runic Archives | Rune Recall | 27 |
| 7 | Seraphel, the Fallen Star | Stargazer Spire | Star Trace | 34 |
| 8 | Ignathia, the Ember Wyrm | Wyrmbone Caverns | Heartpierce | 42 |
| 9 | Xar'Mal, the Lich Emperor | Voidglass Sanctum | Soul Dodge | 50 |
| 10 | The Hollow King | The Aether Vault | Five Trials | 60 |

### `core/include/ed/boss_minigames.hpp` and `core/src/boss_minigames.cpp`

This is the largest gameplay state-machine file. Every boss minigame has
deterministic state, start/update/input/score functions as appropriate, and no
raylib dependency. The client renders and routes input to these states.

Implemented minigames:

#### Frenzy Dig

The player rapidly taps visible nugget targets. Spawn limits and target
placement were tuned to avoid impossible/overlapping arrangements. Attack and
gear-derived values affect scoring.

#### Bone Duel

A timing/strike duel with a moving needle and focus-scaled success zone.
Starting side randomization and miss/heart behavior were corrected during
development.

#### Cinder Dance

A rhythm chart with notes, lane judgment, groove, timing windows, and feedback.
It supports standalone mode and a configurable gauntlet mode. Gauntlet mode
prevents immediate phase failure when groove reaches zero so the Five Trials
controller can own the phase result.

The state supports:

- configurable duration;
- configurable note count;
- four lanes;
- `PERFECT`, `GOOD`, and `MISS` feedback;
- feedback lane and timer;
- deterministic chart generation.

#### Head Hunt

A grid of real and decoy targets. Selecting decoys or missing targets affects
hearts and score.

#### Parry Duel

A direction-response duel with left/down/right attacks, reaction windows,
heart loss, attack count, and feints.

The Hollow King uses a 12-attack version. A feint is intentionally answered by
waiting; timing out a feint advances the attack without costing a heart.
Incorrect directional input costs a heart. The graphical client supports arrow
keys and clickable direction panels.

#### Rune Recall

A deterministic sequence-memory challenge. Sequences are randomized per round,
grow with progression, and are capped to a readable maximum.

#### Star Trace

The player clicks highlighted stars in order. Each round generates a fresh
randomized placement rather than appending onto stale positions. Overlapping
star placement was corrected, and later sequences are long enough to remain
meaningful.

#### Heartpierce

Heartpierce replaced the old Wyrmfire Run after the revised
`BOSS_8_IGNATHIA_SPEC.md` was adopted.

Core mechanics:

- hold to rise and release to fall;
- a lance/player vertical position;
- a moving heart waypoint;
- overlap gauge;
- multiple phases with different target movement parameters;
- slower and less frequent retargeting than the initial implementation;
- heart loss and temporary invulnerability;
- phase 3 heart darts with telegraphing;
- Focus-based overlap widening;
- Echo/gear scaling through the surrounding campaign/stat systems.

The visible UI includes the rail, target heart, lance, overlap zone, gauge,
phase, timer, hearts, and controls.

#### Soul Dodge

Xar'Mal's challenge is a deterministic bullet-hell survival encounter.

State includes:

- normalized player coordinates;
- duration and elapsed time;
- hearts, hits, and invulnerability timer;
- deterministic pattern state and wave timer;
- active hazards;
- slow/precision movement state;
- finished/cleared flags.

Hazard types:

- spiral projectiles;
- rain projectiles;
- lasers with telegraphs;
- expanding ring hazards.

Non-laser waves spawn multiple objects. The number of objects was preserved
while spatial spread was widened so waves are less clustered. Collision
checking uses the same normalized positions rendered by the client:

- laser collision checks horizontal distance to its lane;
- rain checks point distance;
- spiral/ring checks the calculated angle/radius position;
- a successful hit removes one heart and starts one second of invulnerability.

The standalone encounter lasts 60 seconds. The Hollow King remix uses a shorter
40-second duration.

#### Crown Strike

The final Hollow King phase is a timing challenge with a moving needle and
three strikes. The success zone is centered around 0.5 and is currently 10%
wide before Focus scaling, matching the wider visual green zone. Three
successful strikes clear the phase.

#### Five Trials

The Hollow King controller shares hearts across five phases:

1. **Burst:** six-second rapid-tap trial requiring 48 taps.
2. **Rhythm:** 30-second Cinder Dance chart with 55 notes, four constrained
   lanes, receptors, D/F/J/K input, and feedback.
3. **Parry:** 12-attack Parry Duel with visible direction/feint instructions.
4. **Soul Dodge remix:** 40-second Soul Dodge arena with shared-heart display
   and visible hazards.
5. **Crown Strike:** three timing strikes.

One heart is restored after phase 2, capped at the initial maximum. Failed
phases or depletion of shared hearts end the attempt.

The temporary developer phase-skip key was removed after boss development was
complete. There are no remaining `]`, F6, or campaign phase-skip references in
the current client.

### `core/include/ed/pcg.hpp` and `core/src/pcg.cpp`

Implements the deterministic PCG32 random generator used by minigames,
dungeons, target placement, and other seeded systems. Seeds are kept in state
or passed to start functions so test cases can compare independent runs.

### `core/include/ed/postgame.hpp` and `core/src/postgame.cpp`

Contains ascension, Echo, post-campaign, and other long-term progression
operations. These systems consume campaign/progression state and update
run-scoped and account-scoped currencies.

### `core/include/ed/save.hpp` and `core/src/save.cpp`

`SaveSnapshot` groups:

- `GameState`;
- `InventoryState`;
- `CampaignState`.

Serialization is a line-oriented text format. It stores game state, resource
maps, campaign arrays, gear fields, socketed gem IDs, and equipped IDs.
Socketed gems were added as a backward-compatible optional eighth gear field:

```text
gear=slot|itemId|name|tier|upgradeLevel|sockets|legendary|gem1,gem2
```

Older seven-field gear lines remain readable. Local save writes are atomic.
The serializer is also used by cloud-save requests.

### `core/include/ed/cloud.hpp` and `core/src/cloud.cpp`

Implements the client-side cloud-save request path. The graphical client can
upload/download serialized snapshots through the authenticated API while still
maintaining local account-bound saves.

## 5. Graphical client architecture

`client/src/app/main.cpp` is the main application and currently owns:

- raylib window and main loop;
- `AppState`;
- screen selection;
- input routing;
- all active minigame states;
- campaign attempt initialization;
- rendering for mining, inventory, crafting, campaign, dungeon, profile, and
  account-related flows;
- autosave/manual save behavior;
- local/cloud account flows.

The client runs update logic in `updateEffects`, routes active minigame input
through `handleActiveCampaignInput`, and renders through screen-specific
functions such as `drawCampaignScreen`, `drawInventoryScreen`,
`drawProfileScreen`, and `drawCraftingScreen`.

### Active fight input isolation

When a boss attempt is active, boss input is processed before global controls.
This prevents movement/navigation, sign-out, save controls, and unrelated
screen actions from accidentally affecting the fight. `BACKSPACE` abandons an
active attempt.

### Profile stats screen

The Profile menu now calls `calculatePlayerStats` and displays:

- mining power and mining bonus;
- attack, defense, max HP, speed, pierce, luck;
- crit chance and crit damage;
- focus, XP bonus, and lifesteal.

The displayed values include equipped gear and socketed gem effects.

### Inventory gem workflow

1. Open Inventory.
2. Select a gear piece with `A`/`D` or the mouse.
3. Press `G` to insert the first available inventory gem.
4. Press `R` to remove the selected piece's last socketed gem.
5. Press `E` to equip the piece so its stats and gems affect derived stats.

The UI displays `used sockets / total sockets` for every gear card.

## 6. Terminal playtest

`client/src/playtest/main.cpp` provides a text-oriented executable for quick
manual checks of mining, dungeon, and reward behavior. It links through
`emberdeep_client` and therefore uses the same shared core logic as the
graphical application.

## 7. Data files

The `data/` directory currently contains:

| File | Purpose |
|---|---|
| `materials.json` | Material definitions and mining values |
| `layers.json` | Layer names, tiers, and unlock requirements |
| `gems.json` | Gem IDs, names, rarity, effects, and priority |
| `bosses.json` | Campaign boss content definitions |
| `perks.json` | Vow/perk definitions |
| `dungeon_zones.json` | Dungeon zone content |

The current core defaults in `data_model.hpp` and `campaign.cpp` are the
authoritative runtime references used by the tested C++ systems. Data files are
content references and should be synchronized whenever a mechanic is renamed.
In particular, `data/bosses.json` still contains the old Ignathia challenge
name `wyrmfire-run`; the code uses `Heartpierce`.

## 8. Server and persistence architecture

### 8.1 Local saves

Guest progress and signed-in progress are stored locally. Account-specific
names are:

```text
emberdeep-guest.sav
emberdeep-<username>.sav
```

The active account profile is stored in `emberdeep.profile`. Signing out does
not delete saves. The client loads the account's local save before attempting
cloud synchronization.

### 8.2 API

`server/src/api_main.cpp` implements a small HTTP API with:

- `GET /health` and `GET /healthz`;
- `GET /readyz`;
- `POST /v1/auth/register`;
- `POST /v1/auth/login`;
- authenticated `GET /v1/saves/latest`;
- authenticated `PUT /v1/saves`.

Save payloads are serialized snapshots encoded as hexadecimal inside a simple
JSON field. Authentication uses bearer tokens. The fallback local backend
stores users and saves under the configured API data root.

The API validates usernames and minimum password length, hashes local fallback
password material with OpenSSL SHA-256, checks bearer authentication for save
routes, and returns explicit HTTP error responses.

### 8.3 PostgreSQL

With PostgreSQL enabled, the API can use `server/src/postgres_backend.cpp`.
Database schema changes are in:

- `server/migrations/0001_init.sql`;
- `server/migrations/0002_add_access_token_revoked_at.sql`;
- `server/migrations/0003_extend_existing_access_tokens.sql`.

PostgreSQL support includes account records, password/token handling, save
revisions, token lifetime, and health/readiness integration.

### 8.4 Deployment

Deployment assets include:

- `docker-compose.yml`;
- `server/Dockerfile`;
- `deploy/Caddyfile`;
- `deploy/README.md`;
- `.env.example`;
- PostgreSQL initialization and existing-volume upgrade scripts.

The production profile uses PostgreSQL 16, the C++ API, and Caddy for HTTPS.
The intended commands are:

```bash
cp .env.example .env
# Replace every placeholder with strong secrets.
docker compose up -d --build --pull always
curl https://your-domain.example/healthz
curl https://your-domain.example/readyz
docker compose ps
```

Authenticated sessions default to a configurable 30-day lifetime. A migration
extends existing active sessions once so a server restart does not invalidate
the desktop client's stored token.

The deployment validation todo remains blocked only because a Docker daemon was
unavailable in the development environment; local C++ build and test
validation succeeded.

## 9. Controls

### Global navigation

| Input | Action |
|---|---|
| `1` | Mining |
| `2` | Inventory |
| `3` | Crafting |
| `4` | Campaign |
| `5` | Dungeon |
| `6` | Profile |
| `F5` | Manual save |
| `F` | Sign out when no Cinder Dance fight is active |
| `BACKSPACE` | Abandon active campaign attempt |

### Mining

| Input | Action |
|---|---|
| `SPACE` | Mine |
| Mouse click | Mine within the mining area |
| Layer navigation controls | Move between unlocked layers |

### Inventory

| Input | Action |
|---|---|
| `A` / `D` | Select gear |
| `W` / `S` | Scroll |
| Mouse wheel | Scroll |
| `U` | Upgrade selected gear |
| `E` | Equip selected gear |
| `G` | Insert first available gem |
| `R` | Remove selected gear's last gem |

### Crafting

| Input | Action |
|---|---|
| `A` / `D` | Select recipe |
| `W` / `S` or wheel | Scroll recipe list |
| `M` | Smelt four matching ore |
| `ENTER` | Craft selected recipe |

### Bosses

Controls are shown on the relevant campaign screen. The important mappings are:

- Frenzy Dig: click/space target;
- Bone Duel: space/click timing strike;
- Cinder Dance: `D`, `F`, `J`, `K` or lane clicks;
- Head Hunt: `1`-`9` or grid clicks;
- Parry Duel: left/down/right arrows or direction panels;
- Rune Recall: number keys;
- Star Trace: click highlighted stars;
- Heartpierce: hold space/mouse to rise, release to fall;
- Soul Dodge: WASD/arrows, Shift for precision;
- Five Trials: phase-specific controls as listed above.

The former temporary phase-skip shortcut has been removed.

## 10. Tests and validation

The test suite currently covers:

- campaign unlocking and rewards;
- all boss minigame state machines;
- deterministic patterns;
- Soul Dodge density, collision, and invulnerability;
- Heartpierce state;
- Crown Strike timing;
- Five Trials heart restoration;
- Parry feint behavior;
- rhythm feedback and chart density;
- smelting, crafting, upgrades;
- dungeon determinism and rewards;
- formulas and progression;
- inventory and socket rules;
- gem insertion/removal;
- gem-derived stats;
- save serialization including socketed gems;
- local save backend behavior;
- profile writing.

Current validation commands:

```bash
cmake --build build -j 4
ctest --test-dir build --output-on-failure
git diff --check
```

The latest verified suite contains **42 passing tests** and the graphical
client builds successfully.

## 11. Development history and completed work

The major implementation stages completed during the session were:

1. Read and understood the updated game specification while ignoring
   TypeScript-oriented hints.
2. Built the shared C++20 core, clients, progression, inventory, crafting,
   saves, backend, and deployment foundation.
3. Added account flows, local/cloud persistence, PostgreSQL support, token
   persistence, health/readiness endpoints, and deployment assets.
4. Fixed layer state-reset behavior, save persistence after server relaunch,
   admin test state, and inventory gear scrolling.
5. Planned and implemented all ten campaign bosses in five development stages.
6. Implemented early boss minigames:
   Frenzy Dig, Bone Duel, Cinder Dance, Head Hunt, Parry Duel, Rune Recall,
   and Star Trace.
7. Replaced Ignathia's original Wyrmfire Run with Heartpierce from
   `BOSS_8_IGNATHIA_SPEC.md`.
8. Implemented Xar'Mal's Soul Dodge bullet hell and tuned projectile density,
   collision, invulnerability, and deterministic patterns.
9. Implemented The Hollow King's Five Trials, including shared hearts, Burst,
   rhythm, Parry, Soul Dodge remix, Crown Strike, timing feedback, and phase
   transitions.
10. Fixed Hollow King phases 3 and 4 by adding functional UI/input, correct
    feint behavior, actual hazard rendering, and removing duplicate Soul Dodge
    updates.
11. Widened Crown Strike's hit zone.
12. Removed the temporary phase-testing shortcut after boss work was complete.
13. Implemented equipment sockets and gems, including insertion/removal,
    stat effects, save persistence, inventory controls, and tests.
14. Corrected percentage gem application so bonuses are accumulated and applied
    after equipment totals.
15. Added the Profile derived-stats screen.

## 12. Known caveats and future work

### Content-source synchronization

The C++ campaign uses `Heartpierce`, but `data/bosses.json` still says
`wyrmfire-run`. If JSON loading is introduced or restored, update that entry
and any documentation that still uses Wyrmfire terminology.

### Gem selection UX

`G` inserts the first gem in the inventory map. A future inventory UI should
allow selecting a specific gem, display gem names/effects, and show each
socket individually rather than only showing a used/total count.

### Stat semantics

The current `PlayerStats` model exposes lifesteal and several gem-derived
values, but not every stat necessarily has a complete combat consumer in all
game modes. Adding a stat to the profile does not automatically create a new
combat mechanic; the consumer must be wired into the appropriate core system.

### Rendering polish

Bosses and equipment still use procedural/basic-shape visuals. The mechanics
and UI feedback are functional, but art, animation, sound, menus, and richer
transitions remain possible future work.

### Backend validation

Local compilation and tests are validated. Full production Docker validation
requires a running Docker daemon and configured deployment secrets/database.

### Save migrations

The line-oriented save format is intentionally simple and backward-tolerant,
but future schema changes should add an explicit migration/version strategy
before changing field meaning.

## 13. Recommended change workflow

For a gameplay change:

1. Add or update the state and pure logic in `core/include/ed` and
   `core/src`.
2. Add deterministic Catch2 coverage under `tests/core`.
3. Wire client initialization, update, input, and rendering in
   `client/src/app/main.cpp`.
4. Update data JSON and the relevant specification/documentation if content
   names or values changed.
5. Run:

   ```bash
   cmake --build build -j 4
   ctest --test-dir build --output-on-failure
   git diff --check
   ```

For a persistence change:

1. Update the model.
2. Update serialization and deserialization.
3. Preserve compatibility with older saves where practical.
4. Add a round-trip test.
5. Check both local and cloud-save paths.

For a backend change:

1. Update the API/fallback backend.
2. Update PostgreSQL implementation and migrations when necessary.
3. Update health/readiness behavior and deployment documentation.
4. Validate authentication, persistence, restart behavior, and error paths.

