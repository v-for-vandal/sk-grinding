# Sub-skill state: registry, values, XP award, co-save

- **Status:** chosen (prototype 1).
- **Design:** design-v2 §3 (glossary), §4.2 (pipeline), §5.1 (range, curve, storage), §5.3 (zones),
  §5.4 (floor), §6 (cross-links), §14 (persistence).
- **Depends on:** *plugin-foundation* ([`plugin-foundation.md`](plugin-foundation.md)): config
  loading, logging.
- **Research:** none.

## What it does

Holds every sub-skill of the managed main skills, applies an XP award to one sub-skill, and saves
everything to the SKSE co-save. Player only.

## Registry

- Built at `kDataLoaded` from `Data/SKSE/Plugins/SkillPractice/skills/*.toml` (repo:
  [`config/skills/`](../config/skills/)). One file = one managed main skill (`[main] actor_value`).
  Every `[[subskill]]` entry is one sub-skill: string id `name`, parent = the file's main skill,
  group, `T`, `w`, optional `T1`, optional fixed `k`, optional `curve`, optional `decay_rate`.
- Ids are global and unique (`1h-sword`, `heavy-shield`, `destruction/missile`, `iron`). A
  duplicate id is a load error (logged, the second one is ignored).
- Concept sub-skills = entries with `k = 0` (`dual-wield`, `with-shield`, `with-magic`).
- The registry also keeps, per main skill: the main XP curve (`[main]`), `[dynamics]`, and the
  solved `k` and `M_i` tables ([`main-skill-level.md`](main-skill-level.md)).
- Cross-links (§6.2) are **not** in prototype 1: effective floor = own floor.

## Values per sub-skill

| Field | Type | Meaning |
|---|---|---|
| `current` | `LevelXP` | present level, decays (§5.5) |
| `ownFloor` | `LevelXP` | grows with practice, never decreases, ≤ `floor_cap` |
| `potential` | `LevelXP` | best level reached (later also books / trainers) |
| `peak` | `LevelXP` | best level reached by practice; used by rust ([`main-skill-level.md`](main-skill-level.md)) |
| `idle` | `float` | idle meter `U`, active game hours ([`decay.md`](decay.md)) |
| `lastRawXP` | `float` | last raw award (UI only, not saved) |

`effectiveFloor` is derived: `max(ownFloor, cross-links)`; in prototype 1 it equals `ownFloor`.

`LevelXP` = `{ uint16 level; float progress; }` with `0 ≤ progress < cost(level)` (§5.1: never a
total-XP float). Helpers needed:

- `cost(L) = improve_mult × L^exponent + improve_offset` (the file's `[sub_curve]`, or the
  sub-skill's `curve` override). If `cost(L) ≤ 0` (e.g. `cost(0)` with offset 0), use `1e-3`
  so that one level step never divides by zero.
- `add(LevelXP&, xp)`: add XP, carry over level boundaries, stop at `hard_cap`.
- `sub(LevelXP&, levels)` for decay: remove a fractional number of **levels**
  (`levels × cost(level)` XP, carried down across boundaries), never below a given `LevelXP`.
- `toFloat()` = `level + progress / cost(level)` (UI, formulas).
- comparisons are lexicographic on `(level, progress)`.

## XP award (one sub-skill)

Input: sub-skill id, `raw` (already × `base_mult` × `debug.xp_mult`, see
[`xp-capture.md`](xp-capture.md)). Steps (design-v2 §4.2; anti-grind, trainers and perks are not
in prototype 1):

1. Apply pending decay of this sub-skill up to now ([`decay.md`](decay.md)).
2. Zone multiplier `z`: `zone_catch_up` if `current < effectiveFloor`, `zone_recall` if
   `current < potential`, else `zone_new`.
   - `split_zone_gain = true`: if the gain crosses the boundary (effective floor or potential),
     the part of the **raw** XP that is needed to reach the boundary at the higher multiplier gets
     that multiplier, the rest the lower one. Loop until all raw XP is spent (at most 2 steps).
3. `current += raw × z` (per part, as above).
4. `ownFloor += floor_rate × raw` (raw XP **before** the zone multiplier, every zone, §5.4);
   clamp to `floor_cap` (as a level: at `floor_cap` progress = 0) and to `≤ current`.
5. `potential = max(potential, current)`; `peak = max(peak, current)`.
6. Idle meter: `U = max(0, U − c_dec × raw)`, `c_dec = 1 / (q × pace_sub)` ([`decay.md`](decay.md)).
7. Tell the main skill: `onSubSkillXP(main, raw)` reduces rust and `U_main`, then recomputes the
   main level ([`main-skill-level.md`](main-skill-level.md)).
8. Log one `[xp]` line if `xp_verbose` ([`plugin-foundation.md`](plugin-foundation.md)): id, raw,
   zone multiplier(s), level before → after (as floats), floor, potential, peak, U, source.

All calls happen on the game's main thread (hooks and event sinks run there; anything that is not
must post a task with `SKSE::GetTaskInterface()->AddTask`).

## Co-save

SKSE serialization interface, unique id `'SKPR'`, one record type per kind, each record starts
with a `uint32` version (start at 1). Keyed by **string id**, never by index (Q-WPN-10), so adding
or removing sub-skills in config never shifts saved data.

| Record | Content |
|---|---|
| `'SUBS'` | count, then per sub-skill: id (uint16 length + bytes), current, ownFloor, potential, peak (each `uint16 level + float progress`), idle `U` |
| `'MAIN'` | count, then per main skill: actor value (uint32), `initialized` flag, `base`, `R`, `U_main`, last written level, then per sub-skill of that skill: id + `L_i^0` (float) |
| `'TIME'` | active game hours counter (double is fine here: it is a sum of small steps and not compared at fine precision), last game-hours stamp |
| `'STAT'` | mod state: `pendingInit` flag for a new game ([`main-skill-level.md`](main-skill-level.md)) |

- On load: unknown ids are kept in a side table and written back on save (so a config change does
  not delete progress). Ids in config but not in the save start "uninitialized" and get the start
  conversion of their main skill on the next init ([`main-skill-level.md`](main-skill-level.md)).
- `Revert` callback clears everything (new game / load of another save).
- No co-save data at all on load → the save is from before the mod: initialize immediately.

## Config

`[xp]` in [`config/SkillPractice.toml`](../config/SkillPractice.toml); per sub-skill fields in
[`config/skills/*.toml`](../config/skills/).

## Test

- Unit tests (no game, see [`plugin-foundation.md`](plugin-foundation.md)): `LevelXP` add / sub
  across many levels, at `hard_cap`, with `cost(0) = 0`; zone split at both boundaries;
  floor never above `floor_cap` or `current`.
- In game: give 10 000 XP to one sub-skill from the debug panel, save, load: identical values in
  the panel and in a state dump before and after.
- Remove one sub-skill from a TOML, load, save, put it back, load: its values are back.
