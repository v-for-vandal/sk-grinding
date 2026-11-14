# Main skill level: formula, rust, writes, start conversion

- **Status:** done (prototype implementation; R-27 runtime validation pending).
- **Design:** design-v2 §4.4 (whole section): §4.4.1 formula, §4.4.3 rust, §4.4.4 start and
  writes, §4.4.5 normalization.
- **Depends on:** [`subskill-state.md`](subskill-state.md), [`decay.md`](decay.md) (active time,
  idle-meter ramp).
- **Research:** [R-27](../research/R-27-new-game-init.md) (when to initialize on a new game).

## What it does

For every managed main skill: computes the main level from the sub-skill levels, keeps its rust,
writes the integer level to the player's actor value with hysteresis, and does the start
conversion when the mod meets a character for the first time.

## Conversion tables (at `kDataLoaded`)

Per sub-skill `i` of a skill file ([`config/skills/*.toml`](../config/skills/)), with the same
model as [`scripts/normalize_conversion.py`](../scripts/normalize_conversion.py) (port it, do not
invent a new one):

```
rise(L) = x²(3 − 2x),  x = clamp(L / T1, 0, 1)                // T1: entry or [conversion] rise_T1
fall(L) = (1 + e^(−T/w)) / (1 + e^((L − T)/w))               // 0 when (L − T)/w > 700
gn(L)   = rise(L) · fall(L)
I_i(L)  = Σ_{l=0}^{L−1} cost_i(l) · gn(l + 0.5)               // table, L = 0 … hard_cap
```

- **k:** a sub-skill with `k = …` in its entry uses that value (concept sub-skills: 0). Every other
  group gets `k_g = M_g / Σ_{i ∈ path, group(i) = g} I_i(path_level)`, with budgets `M_g` from
  `[budget]` exactly as in the script (absolute `main_from` / `main_to`, or a share of what is
  left of `N = mainXP(start_level → target_level)`).
- `M_i(L) = k_i · I_i(L)`; for a fractional level `L = n + f`:
  `M_i(L) = k_i · (I_i(n) + f · cost_i(n) · gn(n + 0.5))`.
- Log at load, per skill: `N`, `k` per group, and `main level from path at path_level` (must be
  `target_level`). A unit test compares `k` with the script's output for all six files.

## Formula (any time)

```
L_i   = max(current_i, peak_i − R)                         // floats (LevelXP.toFloat)
X     = Σ_i (M_i(L_i) − M_i(L0_i))                         // L0_i: saved at start conversion
F     = levelFromXP(base, X)                               // [main] curve of the skill file
F     = clamp(F, min_level, max_level)
```

`levelFromXP(base, X)`: start at the fractional level `base`; for `X ≥ 0` walk up the main curve
`cost(L) = improve_mult × L^exponent + improve_offset` (the remaining part of the current level
first); for `X < 0` walk down the same way. Do it in `double`: this is a computed value, not
stored state.

Recompute after every XP award, after a decay tick that changed anything, after load and after
init.

## Rust `R` and `U_main`

Per main skill, with `r = rust_rate / 24` (levels per active hour) and `pace_skill`, `q` from
config ([`decay.md`](decay.md) for the ramp and the time source):

- tick (`dt` active hours, already × `debug.time_scale`): `U_main += dt` (≤ `idle_max_hours`);
  `R += r × ∫ ramp(U_main) dt` (same closed form as sub-skill decay);
- once per vanilla award (one engine skill use), for every main skill that is the parent of a
  sub-skill the award fed, with the award's `raw` (before zone multipliers and before per-sub-skill
  factors such as the bash shield share): `R = max(0, R − c × raw)` with
  `c = r / (q × pace_skill)`, and `U_main = max(0, U_main − raw / (q × pace_skill))`. **Not once
  per sub-skill** (decided): a spell feeds three sub-skills and a craft two, which would make rust
  recover 3× / 2× faster than with a weapon hit for the same vanilla points. A debug *Give XP*
  to one sub-skill counts as one award;
- prototype cap: `R ≤ max_i(peak_i − effectiveFloor_i)` (beyond that it changes nothing and
  would only slow down the recovery);
- cross-links do not touch `R` (§4.4.3); none exist in prototype 1 anyway.

## Writing the actor value

- `n` = the last written integer level (saved). Write `n + 1` (repeat while possible) when
  `F ≥ n + hysteresis_up`; write `n − 1` when `F < n − hysteresis_down`.
- Write with `player->AsActorValueOwner()->SetBaseActorValue(av, n)`. Only the base value:
  Fortify Skill modifiers stay as they are (§4.4.4).
- `[main] write_main_skills = false` (config) computes and shows `F` but never writes.
- On change: `RE::DebugNotification("<Skill name> increased to n")` / `"... decreased to n"` and a
  `[main]` log line (F, X, R, written).
- **External changes (prototype rule):** before writing, read the base value. If it differs from
  `n` (a quest reward, a script, SSL, the console), log `[main] external change d`, then add `d`
  to `base` and set `n` to the read value. So the external change is kept, not overwritten. The
  final rule (external increments → potential, §4.1, Q-XP-4) is not in prototype 1.
- Legendary reset is **not** blocked in prototype 1. Testers must not use it.

## Start conversion

For one main skill (all its sub-skills at once):

```
main  = player base actor value of the skill
floor = clamp(floor_per_level × (main − floor_zero_at), 0, floor_cap)   // non-concept sub-skills
floor = 0                                                             // concept (k = 0)
current = ownFloor = potential = peak = floor;  U = 0
base = main;  L0_i = floor;  R = 0;  U_main = 0;  n = round(main);  initialized = true
```

So the main level does not jump: `X = 0` and `F = base`. Q-ML-9 (which sub-skills get the floor)
is open; prototype 1 uses its proposal: all non-concept sub-skills.

**When:**

- Load of a save without our data for that skill (a save from before the mod, or a skill file
  added later): immediately at `kPostLoadGame`.
- **New game** (`kNewGame`): all skills are `pendingInit`. While pending, the plugin does not
  suppress vanilla XP for them and never writes them. Initialization happens at the **first XP
  event of that skill after the RaceSex menu has been closed at least once**, or from the debug
  panel button *Initialize now*. Reason: in LoreRim the start (Alternate Perspective, Norden
  Starting Skills, SSL) changes skills after character creation; see
  [R-27](../research/R-27-new-game-init.md).
- Debug panel *Re-initialize all*: wipe all state of all managed skills and run the start
  conversion again from the current actor values (testing only).

Log one `[init]` block per skill: main level read, floor given, number of sub-skills.

## Config

`[main]`, `[init]` in [`config/SkillPractice.toml`](../config/SkillPractice.toml); `[main]`,
`[conversion]`, `[budget]`, `[reference_path]`, `[dynamics] rust_rate, pace_skill` in
[`config/skills/*.toml`](../config/skills/).

## Prototype notes

- The pure formula, initialization and rust fields are implemented in Core;
  player-only actor-value writes and external changes are handled in State.cpp.
- The pinned library calls notifications `SendHUDMessage::ShowHUDMessage`.
- New sub-skills absent from a co-save receive a floor and zero-contribution
  baseline without resetting existing sub-skills. Removed IDs retain baselines.
- No Legendary blocking, books/trainers, cross-links or perk changes are included.

## Test

- Unit: `k` equals the script's output (6 files); `levelFromXP(15, N) = 100`; path members all at
  75, others 0 → `F = 100` for a fresh character (base 15, `L0 = 0`).
- Level-50 save: after load, every managed main skill shows the same level as before, the panel
  shows floors `= clamp(2 × (main − 15), 0, 75)`.
- New game: skills stay untouched until the first hit / cast / craft after character creation;
  then `[init]` lines appear with the skills of that moment.
- With `xp_mult = 100`: hitting with a sword raises `1h-sword` and, through the formula,
  One-Handed; switching to a greatsword with `time_scale = 1000` and `no_grace` makes One-Handed
  rust (R grows, F falls towards `F(current)`), and hitting with a mace keeps R near 0.
