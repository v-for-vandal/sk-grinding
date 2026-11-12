# Decay: active time, idle meter, sub-skill decay

- **Status:** done (pure tests passed; active-time runtime checks pending).
- **Design:** design-v2 §5.5 (decay, idle meter), §4.4.3 (rust uses the same meter),
  §11.1 (active game time).
- **Depends on:** [`subskill-state.md`](subskill-state.md); rust itself is in
  [`main-skill-level.md`](main-skill-level.md).
- **Research:** none.

## What it does

Counts **active** game time, keeps the idle meter `U` of every sub-skill, and lowers `current`
towards the effective floor when a sub-skill is not practised.

## Active game time

- A tick every `tick_seconds` real seconds on the main thread (an update hook, or a timer thread
  that posts `AddTask`; choose one and log which).
- Each tick: `Δ = Calendar::GetHoursPassed() − last` (game hours).
  - `Δ ≤ 0` or the game is paused / in a menu that stops time → nothing.
  - `Sleep/Wait Menu` open, or `Δ > max_tick_game_hours` (sleeping, waiting, fast travel, jail,
    long loading) → not active: only update `last`. Log `[time] skipped Δ` once per jump.
  - else `dt = Δ × debug.time_scale` is active time; add `Δ` (without the scale) to the saved
    counter `activeHours`.
- Jail rule (§5.5) is **not** in prototype 1.

## Idle meter and decay (per sub-skill)

```
U grows:   U = min(idle_max_hours, U + dt)
U shrinks: U = max(0, U − c_dec × raw),  c_dec = 1 / (q × pace_sub)     // on XP award
ramp(U)  = 0                                     for U < grace_hours
         = (U − grace) / (ramp_end − grace)      for grace ≤ U < ramp_end
         = 1                                     for U ≥ ramp_end
rate     = decay_rate / 24 × ramp(U)             // levels per active hour (Q-DYN-4 option (a))
```

- Decay over one interval `dt` while `U` goes from `U0` to `U0 + dt` is the integral of the ramp,
  in closed form (piecewise linear: zero part, triangle part, flat part). Do not sum per frame.
- Apply: `current.sub(levels, floor = effectiveFloor)`. `ownFloor`, `potential`, `peak` are not
  touched.
- `decay_rate`: the sub-skill entry's own value, else the file's `[dynamics] decay_rate`.
- **Debug:** `debug.no_grace = true` → `grace_hours = 0` and `ramp_end_hours = 0`
  (`ramp = 1` whenever `U > 0`). `debug.time_scale` multiplies `dt` for decay, `U` and rust.
- Lazy evaluation is allowed (store a timestamp per sub-skill and catch up on use / UI / tick), but
  the simplest correct version for ~120 sub-skills is: every tick, update all sub-skills. That is
  fine (a few µs).

## Config

`[decay]`, `[debug] time_scale, no_grace` in [`config/SkillPractice.toml`](../config/SkillPractice.toml);
`[dynamics] decay_rate, pace_sub` in [`config/skills/*.toml`](../config/skills/).

## Prototype notes

- Player Update posts no worker: all decay runs on the main thread at the
  configured tick interval. Paused, loading and sleep/wait updates advance the
  calendar baseline without advancing active hours; large jumps are logged/skipped.
- The closed-form ramp is tested against fine numerical integration. Integration
  tests cover floor stopping, rust growth/recovery and equivalent split intervals.
- Trace logs summarize ticks; debug logs record crossed sub-skill levels.
  Actual menu/time-jump behaviour still requires the in-game checklist.

## Test

- Unit: the closed-form integral equals a fine numeric sum for intervals inside, across and after
  the ramp.
- In game, `time_scale = 1000`, `no_grace`: a sub-skill above its floor visibly falls in the panel
  and stops exactly at the effective floor; `U` reaches `idle_max_hours`.
- Sleep 8 hours: `activeHours` does not change, no decay happens (`[time] skipped` in the log).
- Fast travel: same.
