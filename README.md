# Emberdeep: The Hollow Crown

Emberdeep is an offline-first C++20 and raylib incremental mining RPG. It features satisfying clicker mining, deep equipment crafting, socketed gemstones, turn-based dungeon combat, 10 unique skill-based campaign boss minigames, and extensive post-game progression including Ascension prestige, Ember Perks, and the endless Deep Strata.

All core gameplay rules, calculations, and save logic live in a modular `core` library shared by both the graphical client and the terminal playtest harness.

---

## Requirements

- **C++20 compliant compiler** (Clang 15+, GCC 12+, or MSVC 2022)
- **CMake 3.25+**
- **raylib** 5.0+

### macOS (Homebrew)
```bash
brew install cmake raylib
```

### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential cmake libraylib-dev
```

---

## Build and Run

### 1. Build the project
```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/homebrew
cmake --build build -j 4
```

### 2. Run tests
```bash
ctest --test-dir build --output-on-failure
```

### 3. Launch the game
```bash
./build/client/emberdeep_graphical
```

Or run the terminal playtest harness:
```bash
./build/client/emberdeep_playtest
```

---

## Game Overview & Screens

Navigate between screens using top-row number keys (`1` through `8`) or the menu sidebar button:

### 1. Mine (`1`)
- Mine through 10 distinct geological layers from **Ashen Warrens** down to **The Aether Vault**.
- Click the central block or press `Space` to mine.
- Switch between unlocked layers with `Q` and `E`.
- **Post-Game — The Deep Strata:** After conquering all 10 Campaign bosses, Layer 10 gains an endless Depth counter. Each depth exponentially scales block HP ($\times 2.6^d$), XP ($\times 2.4^d$), Aetherium ore ($\times [1 + 0.5d]$), and awards rare **Aether Shards**.
  - Adjust depth with `[` / `]` or `-` / `+` (or click `[-]` / `[+]`).
  - Toggle auto-advancing depth on block destruction with `T` (or click `[Auto: ON/OFF]`).

### 2. Inventory (`2`)
- Manage raw ores, smelted ingots, dungeon trophies, gems, and crafted gear.
- Scroll with the **Mouse Wheel** or `W` / `S`.
- Navigate gear cards with `A` / `D` or direct mouse click.
- **Gem Sockets:** Gear from Tier 4 onward contains sockets. Press `G` to socket available gems or `R` to remove gems.

### 3. Crafting & Smelting (`3`)
- Smelt 4 ores into 1 ingot (`C`).
- Craft tiered pickaxes and armor pieces using ingots, gold, and dungeon trophies.
- Scroll through recipes with `W` / `S` and craft with `Enter`.

### 4. Campaign (`4`)
- Challenge 10 unique bosses featuring distinct mini-game mechanics:
  1. **Grubnok the Undermother** — Frenzy Dig (speed clicks)
  2. **Sir Rattlebone** — Bone Duel (rhythm needle gauge)
  3. **Morwenna the Ashen** — Cinder Dance (4-lane rhythm game: `D`, `F`, `J`, `K`)
  4. **Thessaly the Blind** — Head Hunt (directional reflexes)
  5. **Aldric the Unbroken** — Parry Duel (timed directional parrying: `A`, `W`, `D`)
  6. **Isolde the Weaver** — Rune Recall (memory sequence recall: `1`–`4`)
  7. **Seraphel the Sunken** — Star Trace (path timing)
  8. **Ignathia the Molten** — Heartpierce (charge and release)
  9. **Xar'mal the Devourer** — Soul Dodge (bullet dodging: `A` / `D`)
  10. **The Hollow King** — The Five Trials (multi-phase gauntlet)
- **Echo Campaign:** After conquering The Hollow King, all 10 bosses become replayable in the post-game Echo Gauntlet with escalating difficulty, bonus rewards, and rank progression.
- Preview or quit any boss fight at any time with `Esc`.

### 5. Dungeon (`5`)
- Undertake 5-floor turn-based expeditions against zone enemies that spawn together.
- Combat proceeds automatically with visual turn order and damage floats.
- Press `Space` or `Esc` to fast-forward the current floor.
- Conclude runs with a detailed victory rewards summary.

### 6. Gem Hollow (`6`)
- A daily mining event featuring 12 hidden veins for mining gemstones and arcane dust.

### 7. Enchanting (`7`)
- Enhance equipment substats by spending gold and arcane dust.

### 8. Profile & Ascension (`8`)
- View lifetime miner statistics, bosses cleared, and deepest depth reached.
- **Ascension Prestige:** Reset run progress after defeating Boss 10 to earn permanent **Ember Marks**.
- **Ember Perks:** Spend Ember Marks on persistent account upgrades:
  - **Potential:** Extends the level cap from 60 up to 100 (+5 cap per 3 ranks).
  - **Power:** +5% Mining Power per rank.
  - **Wealth:** +5% Gold earned per rank.
  - **Wisdom:** +5% XP gained per rank.
  - **Armory:** Start every ascension run equipped with a Tier-3 starter gear set.
  - **Smelting:** Improved smelting efficiency.

---

## Save System

- **Multi-Save Slot Management:** Click the `[SAV]` button in the top bar to open the local save manager modal.
- **Seamless Switching:** Select and load any local save file (`emberdeep*.sav`) directly without restarting the game.
- **Create New Saves:** Create fresh named save slots on the fly from the UI dialog.
- **Atomic Background Autosaves:** Background worker thread periodically writes progress safely to disk without causing frame drops.
- **Manual Save:** Press `F5` or click `[SAVE]` in the top bar for an immediate atomic save.

---

## Repository Structure

```text
├── client/
│   ├── src/app/                 # Modular raylib graphical application
│   │   ├── main.cpp             # Game bootstrap, initialization & main loop orchestrator
│   │   ├── app_state.hpp        # Central AppState structure, screens, timers & animation state
│   │   ├── save_system.hpp/.cpp # Multi-save management, background thread & slot modal UI
│   │   ├── game_actions.hpp/.cpp# Mining hits, block generation, layer selection & triggers
│   │   ├── game_updates.hpp/.cpp# Per-frame logic, boss cooldowns, floating texts & screen shakes
│   │   ├── campaign_minigames.hpp/.cpp # UI & mechanics for all 10 boss mini-games
│   │   ├── ui_screens.hpp/.cpp  # Screen rendering (Mine, Inventory, Crafting, Profile, etc.)
│   │   └── input_handler.hpp/.cpp # Keyboard and mouse dispatch routing across screens
│   └── src/playtest/            # Headless terminal playtest harness
├── core/                        # Platform-agnostic game logic and data models
│   ├── include/ed/              # Public headers (mining, combat, stats, postgame, save)
│   └── src/                     # Core mechanics, formulas, and state simulation
├── data/                        # JSON data definitions (materials, layers, bosses, perks)
├── sprites/                     # Pixel art assets for mining blocks and ingots
└── tests/                       # Catch2 automated test suite (47 unit and integration tests)
```
