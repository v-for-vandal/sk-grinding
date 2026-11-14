# R-27. When to run the start conversion on a new game

- **Status:** lazy-init prototype built; LoreRim runtime checks pending.
- **For:** [`impl/main-skill-level.md`](../impl/main-skill-level.md) (*Start conversion*,
  *When*); design-v2 §4.4.4, Q-ML-9.

The start conversion turns the main skill levels into sub-skill floors and stores them as
`base`. On a new game the skills are not final at `kNewGame`: the race menu adds race bonuses,
and LoreRim then changes skills again. If we convert too early, those changes become "external
changes" (prototype: added to `base`; final design: potential, Q-XP-4) instead of start values.

LoreRim mods involved (from the profile, 2026-11-11): **Alternate Perspective** 4.1 (start
room, then a chosen start), **Norden Starting Skills Lorerim**, **Static Skill Leveling
Rewritten** (points at level-up), Experience.

## Checks

| # | Question | How to check | Result decides |
|---|---|---|---|
| 1 | What does Norden Starting Skills do, and when (race menu close, a menu or book in the start room, a script after the start is chosen)? | read its scripts / ESP (static), then watch the skill values in the log on a new game | the trigger |
| 2 | Is the RaceSex menu reopened during the Alternate Perspective start (e.g. a mirror)? | log every RaceSex menu open / close on a new game | whether "first close" is enough |
| 3 | Does SSL give points at character creation (level 1)? | log external changes on a new game with SSL points ≠ 0 | whether SSL must be switched off first (R-16) |
| 4 | Is "first XP event after the race menu was closed" late enough, and never too late (e.g. XP during the start room)? | prototype 1 logs `[init]` with the skill values at that moment | keep the rule, or wait for a quest stage of the start quest |

## Local source and new-game observation (2026-11-14)

- The major/minor selector is `Starting Skills.esp`, shipped in MO2's
  `LoreRim - MCM and INI Settings`. Its source is
  `Scripts/Source/StartingSkills_Script.psc`. The separate
  `Norden Starting Skills Lorerim` mod supplies `Interface/StartingSkills_movie.swf`.
- `OpenMenu()` subscribes to the SKSE mod event `StartingSkills_SetSkills` and
  opens `CustomMenu`. The UI receives maximum major/minor counts and bonuses
  from quest properties (`iMaxMajors`, `iMaxMinors`, `iMajorValue`, `iMinorValue`).
- `OnSetSkills` parses pipe-separated entries containing actor-value index,
  major/minor category, and final level. It calls `CloseMenu()` **before**
  looping through `Player.SetActorValue` calls. Thus neither that mod event
  nor `CustomMenu` closing proves that the final values have been applied.
  There is no completion notification in this source. Do not initialize
  immediately on either event; an integration would need to await final writes
  or add a completion notification after the loop.
- The received log contains new-game pending initialization and no per-skill
  `[init]` records. The reported Block 15 in our panel versus 20 in the game
  therefore does not demonstrate a missed external increment: the panel was
  showing the uninitialized model default. The corrected runtime snapshot
  displays the live base actor value for pending skills without initializing
  them or assigning sub-skill floors.
- Lazy initialization still reads the live base value at first qualifying use;
  finish major/minor selection before practicing or using Initialize now.
  For an already initialized skill, `synchronize()` detects base-value changes
  and preserves them as external adjustments. Such late adjustments do not
  retroactively recalculate starting floors. A first-use log after selection
  is still needed to validate final bonuses and floors in game.

## Prototype rule

Implementation logs RaceSex menu opens/closes and every skill initialization.
Pending state survives co-save round trips; an existing save without our data
initializes at post-load. See checklist step 13. No claim about Norden/SSL event
ordering is made before receiving a new-game log.

Lazy: initialize a skill at its first XP event after the RaceSex menu has been closed at least
once, or with the debug button *Initialize now*. All external changes before that are part of
the start values by construction.
