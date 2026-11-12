# R-17. Raw skill-use points with `UseMult = 0`

- **Status:** new (runtime; done in milestone 1 of [`plan/prototype-1.md`](../plan/prototype-1.md)).
- **For:** [`impl/xp-capture.md`](../impl/xp-capture.md); design-v2 Q-XP-9, Q-ML-7 (pace),
  §15.1 (telemetry).

In LoreRim every skill AVIF has `UseMult = 0`, so vanilla skill XP after the multiplier is 0. The
design takes the **raw** use points as base XP. Which engine call carries them, and what it tells
us about the source, is not known yet.

## Checks

| # | Question | How to check | Result decides |
|---|---|---|---|
| 1 | Is the `PlayerCharacter` `UseSkill` vfunc called with non-zero points in LoreRim for: 1H / 2H hit, unarmed hit, shield block, weapon block, shield bash, weapon / torch bash, smithing craft, tempering, smelting, tanning, Destruction hit, Restoration self heal, heal other, ward absorbing a spell? What is the third argument (`TESForm*`) in each case? | log `[use] skill points arg` (form type, FormID, name) for each action, 3 times | the hook point; whether `a_arg` gives the item / spell |
| 2 | Are there other paths that change skill XP or level (`PlayerSkills::AdvanceSkill` called directly, Papyrus `AdvanceSkill` / `IncrementSkill`, SSL, Little Lessons)? | log-only hook on `PlayerSkills::AdvanceSkill` (Address Library ID from CommonLib); read a skill book; SSL level-up | external-increment handling (design-v2 §4.1, later) |
| 3 | Order of `UseSkill` relative to `TESHitEvent`, the swing / bash animation events, `TESSpellCastEvent`, `ItemCrafted` | timestamps (µs) and a frame counter in the log | whether the pending queue of xp-capture is needed |
| 4 | Does another loaded plugin already hook this vfunc (Experience, Precision, …)? | at install, log the module that owns the original pointer | hook order / chaining |
| 5 | Raw points per action (weapon base damage? magicka cost? blocked damage?), for the pace estimate | the same log, 30 min of normal play | `base_mult`, `pace_sub`, `pace_skill` in `config/skills/*.toml` (Q-ML-7, Q-DYN-15) |

## How to decide

- Check 1 passes for the prototype skills → keep the vfunc hook.
- Points are 0 for some action (e.g. lockpicking: SSL zeroes `fSkillUsageLockPick*`, see
  `refs/vanilla-skill-xp.md` §3) → that action needs its own event-based base XP.
- No context in `a_arg` → context events and player state (xp-capture *Context*).
