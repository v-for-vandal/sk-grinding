# Prototype 1: code review 2026-11-14 and 0.1.4 follow-up

A static review of `prototype1` at `45dd9a6`. The fixes below were made on a Linux machine
**without MSVC and without the game**. The pure core (`Core.cpp`, `Config.cpp`, `CoreTests.cpp`)
was built with g++ and its tests pass. The game-side sources (`Telemetry.cpp`, `State.cpp`,
`Aim.cpp`, `Classification.cpp`) were **not compiled**.

## 1. First: build on Windows

- Build the Release DLL and the tests with the commands in [`plugin/README.md`](../plugin/README.md).
  Fix any compile errors in the files listed above; keep the behaviour described in section 2.
- Expect `[boot] SkillPractice 0.1.4` in the log.

## 2. Done in 0.1.4 (blind): check in game

| # | Change | Files | How to check |
|---|---|---|---|
| R1 | A managed-skill `UseSkill` call on a worker thread is suppressed and replayed on the main thread by the next player update (the same FIFO as the deferred context events). Before, it passed through and leaked vanilla XP, which `synchronize()` then counted as an external change of the main skill. Unmanaged skills still pass through at once. | `Telemetry.cpp` (`use`, `useOnMain`), `State.cpp` (`managed`) | log level debug: look for `[use] deferred from thread=`; after such lines there must be `[xp]` lines and **no** `[main] external` lines during normal play. |
| R3 | Hooks and event sinks run inside `runtime::guarded`: an exception is logged as `[boot] exception in <where>` and does not reach engine code. A key missing in the user's `SkillPractice.toml` is taken from the compiled copy (`[cfg] … missing …, using built-in default`). Rows of the materials / directions files are checked at load. | `Runtime.h`, `Config.cpp`, `Telemetry.cpp`, `Aim.cpp` | delete one key (e.g. `context_window_ms`) from the installed TOML: one `[cfg]` warning at start, the game works. No `[boot] exception` lines in a normal session. |
| R4 | Smithing material keywords are now also read from ammo (before: arrows and bolts always went to `other-smithing`). | `Classification.cpp` (`has`) | craft arrows of a material listed in `smithing-materials.toml`: the material sub-skill gets XP. If LoreRim's ammo has no `WeapMaterial*` keywords, report it (then this needs a design decision). |
| R6 | Closing the journal re-reads only the MCM INI files (`reloadINI`), not all TOML files. | `Telemetry.cpp`, `State.cpp` | change an MCM value, close the journal: one `[cfg]` line, **no** `[cfg] normalization` lines; the panel's *External* counter is not reset. |
| R7 | `Defaults.h` is generated as byte arrays (MSVC C2026 limit ~16 KB per string literal; `smithing-materials.toml` was 13.9 KB) and is regenerated when any `config/*.toml` changes. | `CMakeLists.txt` | the build passes; the test *compiled defaults match repository TOML* passes after editing a TOML without a manual reconfigure. |
| R8 | Test: `WeaponClasses.h` order equals `tools/esp-gen/keywords.json` (FormID = 0x800 + index). | `CoreTests.cpp` | `PracticeTests` passes. |
| R9 | A player update on a thread other than the recorded main thread is logged once as a warning (before: the plugin silently did nothing). | `Telemetry.cpp` | no such warning in a normal session. |
| R10 | `classify()` is cached per spell FormID (cleared on config reload); the `[init]` line shows the highest start floor of the skill (before: the first sub-skill's, possibly a concept one at 0); UTF-8 BOMs removed from three sources. | `Classification.cpp`, `State.cpp` | `[spell]` dump unchanged compared with 0.1.3. |

## 3. Open: for the Windows agent

- **Missile noise has no effect** (player report 2026-11-12). Suspected cause: `Projectile::Launch`
  applies noise only on the recorded main thread (`Aim.cpp`), and player spell launches may run on
  another thread. Steps:
  1. Read the first 20 `[aim] player launch thread=… main=…` lines of a session where Ice Spike /
     Firebolt are cast. If `thread != main`, that is the cause; also check whether
     `[aim] classified` lines appear at all.
  2. If so: compute the per-school missile σ on the main thread (each player update) and keep it
     in atomics; in the hook, decide "missile form" without touching the model (e.g. a per-spell
     form cache filled on the main thread at equip / cast and read under a lock), then apply the
     noise on any thread. Do not read `model` from a worker thread.
  3. If the threads match: the engine overrides `angleX` / `angleZ` later; look at what else in
     `LaunchData` sets the direction (origin / contact normal, `useOrigin`, the caster's
     aim) and log it before and after `original`.
  Record the result in [R-26](../research/R-26-spell-aim.md) and
  [`impl/spell-aim-inaccuracy.md`](../impl/spell-aim-inaccuracy.md).

## 4. Waiting for a design decision (do not change)

- **Rust recovery per sub-skill award.** `Model::award` lowers `R` and `U_main` for every
  sub-skill that gets XP. One spell cast feeds three sub-skills (form, direction, archetype), a
  craft two, a shield bash 1.5 × raw, so for the same vanilla points rust recovers 3× / 2× /
  1.5× faster than with a weapon hit. Alternative: lower it once per vanilla award (the raw of the
  event). The owner decides; see [`impl/main-skill-level.md`](../impl/main-skill-level.md) *Rust*.

## 5. Noted, not changed

- The bash context lives `context_window_ms` (750 ms): a block right after a bash counts as a bash.
