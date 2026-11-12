# R-27. When to run the start conversion on a new game

- **Status:** new (static + runtime).
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

## Prototype rule

Lazy: initialize a skill at its first XP event after the RaceSex menu has been closed at least
once, or with the debug button *Initialize now*. All external changes before that are part of
the start values by construction.
