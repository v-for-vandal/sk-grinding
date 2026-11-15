# Cross-links and the Alteration floor (effective floor)

- **Status:** in progress: core implemented and unit-tested (prototype 2); runtime part and
  in-game check pending ([`plan/prototype-2.md`](../plan/prototype-2.md)).
- **Design:** design-v2 §5.3 (zones), §5.4 (floor), §6 (cross-links, non-transitive: decided,
  former Q-CL-1); [`skills/alteration/design.md`](../skills/alteration/design.md) (Q-ALT-1).
- **Depends on:** [*subskill-state*](subskill-state.md), [*decay*](decay.md).
- **Research:** none.

## What it does

The **effective floor** of a sub-skill B is derived, never stored:

```
effectiveFloor(B) = min(hard_cap, max(
    ownFloor(B),
    max over links A→B of c × ownFloor(A),          // cross-links
    cAlt × scale × AlterationLevel   if B is magic   // Alteration term
))
```

- **Non-cumulative:** only the largest term applies; the same pair listed twice keeps the larger
  `c`.
- **Non-transitive:** only **own** floors are read. A floor that B got from A is not passed on.
- It is used in two places: the **catch-up zone** of an XP award (`current < effectiveFloor` →
  `zone_catch_up`, with the zone split at the effective floor) and the **decay stop**
  (`current` never decays below it). The own floor alone grows with practice (every zone, at
  `floor_rate`, §5.4) and is what the rust cap (`peak − floor`) reads: cross-links do not
  touch rust (§4.4.3).
- **Alteration term:** `AlterationLevel` is the written level of the main skill when the plugin
  manages Alteration, else `Model::externalLevels["Alteration"]`, which the runtime sets from the
  player's base actor value (0 until then). Magic sub-skills: every sub-skill of the listed
  schools plus the shared forms and archetypes (`include_shared`).

## Code (core, no game)

- `Sub::links` (`LinkIn{skill, sub, c}`: index into `Model::all()`, then the sub-skill index),
  resolved at config load from [`config/cross-links.toml`](../config/cross-links.toml)
  (`readLinks` in `Config.cpp`); `Sub::magic`; `Model::alteration`.
- `Model::effectiveFloor(sub, &source)`: `source` = `"own"`, the linked sub-skill id or
  `"Alteration"`.
- `Model::award`: zone boundary at the effective floor, compared with a 1e-6 tolerance and at most
  8 zone steps (a floor of `c × other floor` is not exactly representable as `LevelXP`; without
  the tolerance the split could loop forever).
- `Model::tick`: `current.subtract(…, effectiveFloor)`.
- Snapshot: `effFloor` (was = own floor) and the new `floorFrom` per sub-skill
  ([`status-ui.md`](status-ui.md); the panel already shows `effFloor`).
- PracticeSim: action `{"action": "external", "av": "Alteration", "level": 50}`.

## Config

[`config/cross-links.toml`](../config/cross-links.toml):

- `[[link]]`: `from`, `to` (lists of ids; `*` matches any run of characters), `coefficient`
  (0, 1], `symmetric` (also `to → from`). A pattern that matches no registered sub-skill is a
  config warning, not an error (Archery and Armor are not managed yet).
- `[alteration]`: `enabled`, `coefficient` (cAlt = 0.2), `scale` (2: main 0–100 → sub-skill
  0–200), `schools`, `include_shared`.

Table of §6.2 as configured: 1H↔1H ×1/2, 2H↔2H ×1/2, 1H↔2H ×1/4, archery ↔ archery ×1/2,
`light-armor ↔ clothing` ×1/2; concept sub-skills, Block, Smithing and magic none.

Decided (2026-11-15):

- `1h-*` / `2h-*` include `1h-unarmed`, `1h-other` and `2h-other` (§6.2 says "same hand class");
  concept sub-skills do not match these patterns.
- `archery-*` links every archery sub-skill with every other (§6.2: "bow ↔ crossbow sub-skills").

## Runtime (needs the game, [`plan/prototype-2.md`](../plan/prototype-2.md))

- Set `externalLevels["Alteration"]` from the player's base Alteration on load and on every tick
  (only while Alteration is not a managed skill).
- Nothing else: awards and ticks already use the effective floor.

## Test

- Unit tests (`PracticeTests`): links 1H ×0.5 and 2H ×0.25, non-cumulative, non-transitive (an
  explicit chain), concept sub-skills unlinked, catch-up zone up to the effective floor (the
  award loop ends), decay stops at the effective floor, snapshot `floorFrom`, Alteration term on
  Destruction / Restoration / shared sub-skills and not on weapons.
- In game: raise `1h-sword` (debug *Set current* and XP until its own floor is ~60); `1h-mace`
  shows effective floor 30 from `1h-sword`; mace XP below 30 runs at ×3; with *no grace* and time
  scale, a mace at 50 decays to 30, not to its own floor. With Alteration 50 (console
  `player.setav alteration 50`), every Destruction / Restoration / form sub-skill shows effective
  floor 20 from Alteration.
