# Boss 8 — Ignathia, the Ember Wyrm (Revised Mechanic)

> **Replaces** the original "Wyrmfire Run" (3-lane dodge runner + mash bursts) in Section 10.2 of the main spec.
> **Why:** the runner needed scrolling hazard generation, lane collision, pickups and a second mash phase. This version is a single-axis "hold to rise, release to fall" tracking game: about 100 lines of physics, no hazard spawner, no collision shapes.
> Everything not listed here (boss location, legendary drop, rewards, hearts, Focus rules, star reward structure) stays as in the main spec.

---

## 1. Summary

| | |
|---|---|
| **Name** | Heartpierce |
| **Type** | Continuous tracking (hold-to-rise, release-to-fall), similar in feel to a fishing minigame |
| **Location** | Wyrmbone Caverns |
| **Length** | 3 phases, about 15 s each, roughly 45–60 s total |
| **Inputs** | Hold mouse button or Space (rise); release (fall) |
| **Fail condition** | Hearts reach 0 |
| **Legendary on first clear** | Wyrmfang Breaker (unchanged) |

**Fantasy:** Ignathia thrashes in the cave, and her glowing heart moves behind her scale plates. You hold a charged lance on a vertical rail and must keep it level with the heart. While the lance overlaps the heart, a "Heartpierce" gauge fills and one scale plate cracks per phase. When the lance drifts away, the gauge drains. If it empties, her flames burn you (lose a heart).

---

## 2. Look (Pixel Style)

- **Left two thirds:** a large Ignathia portrait (about 64×64 px, scaled ×5): a long serpentine body in ember red with bone-white spikes, a wedge head with a glowing orange eye, a chest with **three layered scale plates** (stacked arcs).
- **Right third:** a **vertical rail** (a dark stone column with a groove, about 80 px wide on a 720 px tall screen).
  - **Heart marker (target):** a glowing red-orange pixel heart that slides along the rail. Its zone is shown as a translucent band around it.
  - **Lance (player):** a steel-blue / teal bar with a pixel spearhead at the top, sliding in the same groove.
- **Gauge:** a vertical (or arc) **Heartpierce gauge** to the left of the rail. Gold fill, with 3 tick marks for phases.
- **Feedback:**
  - Overlap: the lance flashes white, ember particles burst from the heart, a low hum rises in pitch with the gauge.
  - No overlap: the heart marker dims, the gauge flickers red, embers drift down.
  - Phase clear: one scale plate cracks and falls, a short screen shake, Ignathia roars (sprite squash).
  - Heart lost: screen flash red, the gauge resets, and 1 s of invulnerability with a blink.
- **Hearts HUD** (shared widget) at top left; a thin phase label at top ("Phase 2: Ember Heat").

---

## 3. Rules

1. All positions are **normalized on the rail**: `0.0` = bottom, `1.0` = top.
2. **Lance** (`lanceY`, `lanceV`):
   - While the input is **held**: `lanceV += THRUST * dt`.
   - While **released**: `lanceV -= GRAVITY * dt`.
   - Clamp `lanceV` to `±MAX_V`, then `lanceY += lanceV * dt`.
   - At the top or bottom limit: clamp `lanceY` and set `lanceV = -lanceV * RESTITUTION` (a soft bounce).
3. **Heart** (`heartY`) moves by easing toward a **waypoint**:
   - Every `retarget` seconds (random in the phase range), pick a new waypoint `[0.1, 0.9]` at least `0.2` away from the current one.
   - Move toward the waypoint using smoothstep easing, capped at the phase's `heartSpeed`.
4. **Overlap:** the lance overlaps the heart if `|lanceY - heartY| <= (lanceHalfH + zoneHalfH)`. (Both sizes are widened by Focus, see Section 5.)
5. **Gauge** (`0..1`, per phase):
   - Overlap: `gauge += gain * dt`.
   - No overlap: `gauge -= drain * dt`.
   - `gauge >= 1.0` → the phase is cleared.
   - `gauge <= 0` → **lose a heart**, set `gauge = START_GAUGE`, grant `1.0 s` of invulnerability (no heart loss while invulnerable, the gauge still moves).
6. After the final phase is cleared, the fight ends in victory.
7. **Phase 3 only: "darts."** Every 4–6 seconds, the heart makes a sudden jump to a new position. A **0.4 s telegraph** (the heart flashes white at its destination) warns the player first. This is the only extra mechanic.

### Global constants (start values)
| Constant | Value |
|---|---|
| `THRUST` | 2.4 (rail units / s²) |
| `GRAVITY` | 1.6 |
| `MAX_V` | 1.2 (rail units / s) |
| `RESTITUTION` | 0.3 |
| `lanceHalfH` | 0.10 (lance height 0.20) |
| `START_GAUGE` | 0.25 |
| `INVULN_TIME` | 1.0 s |

### Phase parameters
| Phase | Name | `zoneHalfH` | `heartSpeed` (/s) | `retarget` (s) | `gain` (/s) | `drain` (/s) | Darts |
|---|---|---|---|---|---|---|---|
| 1 | Cold Scales | 0.13 | 0.35 | 1.4 – 2.2 | 0.25 | 0.12 | No |
| 2 | Ember Heat | 0.11 | 0.55 | 1.0 – 1.7 | 0.22 | 0.14 | No |
| 3 | Wyrmfire Fury | 0.09 | 0.80 | 0.7 – 1.3 | 0.20 | 0.16 | Yes, every 4–6 s |

All values live in `data/bosses.json` so they can be tuned without code changes.

---

## 4. Clear Conditions & Stars

- **Clear (1★):** complete all 3 phases before hearts reach 0.
- **2★:** clear with **at most 1 heart lost**.
- **3★:** clear with **no hearts lost** and an overall **overlap ratio ≥ 70%** (time overlapping ÷ total fight time).
- Tracked stats for the result: `heartsLost`, `overlapRatio`, `totalTime`. `score = overlapRatio × 100`.
- Star rewards, first-clear rewards, repeat rewards and the legendary drop work as in Section 10.1 of the main spec.
- **Echo ranks:** modifier `mod = 1 + 0.08 × (R − 1)` (cap 2.0) multiplies `heartSpeed`, divides `retarget`, and shrinks `zoneHalfH` by `1/mod` (never below 0.06).

---

## 5. Assist (Focus & Hearts)

- Hearts: `3 + Ward` (as for every boss).
- **Focus** (0–25%): multiply `lanceHalfH` and `zoneHalfH` by `(1 + focus)`. This makes overlapping easier without changing the heart's movement.
- No other assist. The mechanic stays skill-based: Focus never changes gain/drain rates.

---

## 6. Controls

| Input | Action |
|---|---|
| Hold **left mouse button** or **Space** | Rise |
| Release | Fall |
| `Esc` | Pause |

All input goes through `minigames/engine/input`, so keys are rebindable. A gamepad button hold can be added later. No touch or swipe support is needed.

---

## 7. Implementation Notes (C++)

### Config in `data/bosses.json`
```json
{
  "id": "boss_08_ignathia",
  "minigame": "heartpierce",
  "config": {
    "thrust": 2.4, "gravity": 1.6, "maxV": 1.2, "restitution": 0.3,
    "lanceHalfH": 0.10, "startGauge": 0.25, "invulnTime": 1.0,
    "phases": [
      { "name": "Cold Scales",    "zoneHalfH": 0.13, "heartSpeed": 0.35, "retarget": [1.4, 2.2], "gain": 0.25, "drain": 0.12, "darts": false },
      { "name": "Ember Heat",     "zoneHalfH": 0.11, "heartSpeed": 0.55, "retarget": [1.0, 1.7], "gain": 0.22, "drain": 0.14, "darts": false },
      { "name": "Wyrmfire Fury",  "zoneHalfH": 0.09, "heartSpeed": 0.80, "retarget": [0.7, 1.3], "gain": 0.20, "drain": 0.16, "darts": true, "dartInterval": [4.0, 6.0], "dartTelegraph": 0.4 }
    ],
    "stars": { "twoStarMaxHeartsLost": 1, "threeStarMinOverlap": 0.70 }
  }
}
```

### Class sketch
Implements `IMinigame` (Section 22 of the main spec). Rename the enum value `MinigameKind::WyrmRun` to `MinigameKind::Heartpierce`.

```cpp
class HeartpierceGame final : public IMinigame {
  // state
  float lanceY = 0.5f, lanceV = 0.f;
  float heartY = 0.5f, waypointY = 0.5f, retargetTimer = 0.f;
  float gauge, invuln = 0.f, dartTimer = 0.f, dartTelegraph = 0.f;
  int phase = 0, heartsLeft; float totalTime = 0.f, overlapTime = 0.f;
  Rng rng;   // seeded: same seed => same heart path (testable)
 public:
  void update(float dt, const InputState& in) override;
  // ...
};
```

### `update(dt, in)` order (fixed 60 Hz)
1. If paused or finished, return.
2. Step the lance (Section 3, rule 2).
3. Step the heart: update `retargetTimer`, pick a new waypoint when it expires, ease toward it with the speed cap. In phase 3, run the dart timer and telegraph.
4. Compute `overlap`; update `gauge`, `overlapTime`, `totalTime`.
5. If `gauge >= 1`: advance the phase (crack a plate, reset gauge to `startGauge`, play effects); if it was the last phase, set the result to cleared.
6. If `gauge <= 0` and not invulnerable: `heartsLeft--`, reset the gauge, start invulnerability; if hearts hit 0, set the result to failed.
7. Update particles and screen shake.

### Tests (Catch2)
- Lance physics: holding rises, releasing falls, top and bottom bounce stays within `[0, 1]`.
- Same seed ⇒ identical heart path over 60 s (determinism).
- Waypoints are always ≥ 0.2 apart and within `[0.1, 0.9]`.
- Gauge fills while overlapping and drains while apart; reaching 1 advances the phase, reaching 0 costs one heart and respects invulnerability.
- Stars: 1★ / 2★ / 3★ boundaries (hearts lost and overlap ratio).
- Focus widens overlap detection exactly by `(1 + focus)`.
- Echo modifier is capped (never below `zoneHalfH` 0.06).

### Effort estimate
Roughly one small file plus a draw function. It reuses the shared hearts HUD, countdown, pause and results screen from the minigame engine, and needs no scrolling world or hazard spawner.

---

## 8. Optional Polish (Not Required)

- The hum pitch rises with the gauge; a soft "ting" when the lance first enters the heart zone.
- Slight camera zoom on Ignathia during phase 3.
- Accessibility: a high-contrast option that outlines the heart zone in solid white and draws the lance in a bright solid color; reduced-motion mode removes shake and particles but keeps the overlap flash.

---

## 9. Changes Needed in the Main Spec

1. **Section 10.2, Boss 8:** replace the challenge text with a one-line summary and point to this file.
2. **Section 22:** rename `MinigameKind::WyrmRun` to `Heartpierce`; `BossDef.config` for boss 8 uses the JSON above.
3. **Phase 7 of Section 24:** Boss 8 now needs no scrolling or hazard-spawner code.
4. The Hollow King gauntlet (Boss 10) does **not** reuse Boss 8, so it needs no changes.
