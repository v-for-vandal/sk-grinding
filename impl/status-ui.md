# Status panel and debug panel (PrismaUI)

- **Status:** done (browser checks passed; PrismaUI runtime validation pending).
- **Design:** design-v2 §12.1 (PrismaUI panel, decided), §13 (debug); Q-UI-1 (how to open):
  prototype uses a hotkey.
- **Depends on:** [`subskill-state.md`](subskill-state.md), [`main-skill-level.md`](main-skill-level.md),
  [`decay.md`](decay.md), [`weapon-class.md`](weapon-class.md),
  [`spell-classifier.md`](spell-classifier.md), [`spell-aim-inaccuracy.md`](spell-aim-inaccuracy.md).
- **Research:** framework facts in [`refs/frameworks.md`](../refs/frameworks.md) §6 (web research
  2026-11-12); things marked *(verify)* there are checked in the first in-game run.

## Framework

- PrismaUI 1.5.1 (Nexus 148718, not in LoreRim: the user installs it, see
  [`plan/prototype-1.md`](../plan/prototype-1.md) §6). HTML / CSS / JS views rendered by
  Ultralight on top of the game.
- C++ side: the single header `PrismaUI_API.h` from the framework's release 1.5.1 (vendor it into
  `plugin/extern/`). At `kDataLoaded`: `RequestPluginAPI(V2)` (fall back to V1 if null), then
  `CreateView("SkillPractice/index.html", OnDomReady)`, then `Hide` it.
- PrismaUI missing or the API null → log `[ui] PrismaUI not available` (warn) and run without UI;
  everything else works (only the panel's debug buttons are unavailable).
- All callbacks are **plain function pointers** (no capturing lambdas): use static functions and
  global state.

## Web side

- Files: `Data/PrismaUI/views/SkillPractice/index.html`, `app.js`, `style.css` (repo: `ui/`). One
  view for everything; two tabs inside it: *Sub-skills* and *Debug*.
- Plain HTML + JS (ES2022 at most), no build step needed; no WebGL; light CSS (rendering is on
  the CPU at ≤ 60 FPS: no blur, shadows, big gradients).
- The page must work in a normal browser with mock data: if `window.spAction` does not exist,
  load `mock.json` (a saved snapshot) instead. So the UI can be developed and checked without the
  game.

## Data flow

- **C++ → JS:** `InteropCall(view, "spUpdate", json)` with the whole snapshot as one JSON string
  (format below). Sent when the view opens and then at most every `ui.refresh_ms` (250 ms) while
  it is visible; never while hidden.
- **JS → C++:** `RegisterJSListener(view, "spAction", OnAction)`; JS calls
  `spAction(JSON.stringify({action, ...args}))`. `OnAction` parses the JSON, logs `[ui] action`,
  and runs the action on the main thread (PrismaUI runs callbacks on the main thread since 1.1;
  check the thread id once and log it; if it is not the main thread, queue the action for our next
  tick).
- JS `console.*` output → our log through `RegisterConsoleCallback` (V2) as `[ui] js: …`.
- The snapshot is built on the main thread from our live state; JS never reads game data.

Snapshot (JSON, one object):

```json
{ "state": "running|pending", "activeHours": 12.4, "debug": {"xpMult":100,"timeScale":1,"noGrace":false,"fixedXp":false,"writeMain":true},
  "skills": [ { "av": "OneHanded", "name": "One-Handed", "F": 81.37, "written": 81, "base": 80,
                "R": 0.0, "Umain": 3.1, "rustPerDay": 0.0, "external": 0, "pending": false,
                "subs": [ { "id": "1h-sword", "ownFloor": 55.2, "effFloor": 55.2, "current": 75.0,
                            "potential": 75.0, "peak": 75.0, "U": 3.1, "decayPerDay": 0.0,
                            "zone": 1.0, "worth": 0.0, "lastXP": 0.0 } ] } ],
  "equipped": [ { "hand": "right", "kind": "weapon|spell|shield|none", "name": "Iron Sword",
                  "class": "SPO_WeapClass_1H_Sword", "sub": "1h-sword",
                  "form": "", "direction": "", "projectile": "", "formLevel": 0, "sigma": 0, "sway": 0 } ] }
```

## Opening and closing

- Hotkey: our own `BSTEventSink<RE::InputEvent*>` on `BSInputDeviceManager`; key-down of
  `ui.toggle_key` (DX scan code, default F7 = `0x41`) toggles the panel. Ignored while a game menu
  other than ours is open, or in the console.
- Open: send a snapshot, `Show`, `Focus(view, pauseGame = false)` (mouse cursor, game controls
  disabled while open). Close (the same key, `Esc` handled in JS → `spAction({action:"close"})`, or
  a *Close* button): `Unfocus`, `Hide`.
- `ui.inspector = true` (config): `CreateInspectorView` + `SetInspectorVisibility` for the
  Ultralight inspector (DOM, console). Off by default (first open is slow).

## `Sub-skills` tab

Header line: mod state (pending init / running), active game hours, `xp_mult`, `time_scale`,
`no_grace`, `fixed_xp`, `write_main_skills`.

One collapsible block per managed main skill, title `One-Handed  81.37 (written 81)`:

- main-skill line: `F` (2 decimals), written level, `base`, rust `R` (levels), `U_main` (hours),
  current rust growth (levels per active day, from the ramp), number of external changes seen;
- sub-skill table, sortable by any column, one row per sub-skill:

  | Column | Value |
  |---|---|
  | Sub-skill | id |
  | Own floor | `ownFloor` (level, 1 decimal) — "current floor" |
  | Eff. floor | `effectiveFloor` |
  | Current | `current` |
  | Potential | `potential` |
  | Peak | `peak` |
  | Idle U | hours (the sub-skill's "rust" meter) |
  | Decay | current decay rate, levels per active day (0 in the grace period) |
  | Zone | ×3 / ×1.5 / ×1 that the next award would get |
  | Worth | `M_i(L_i) − M_i(L0_i)`, its part of the main skill's `X` |
  | Last XP | last raw award |

- Options: *Hide untouched* (rows where current = floor = potential = 0), text filter. A value
  that changed since the previous snapshot is highlighted for a second.

## `Debug` tab

- Equipped, per hand: weapon and its class / sub-skill, or spell with form, sub-direction and
  projectile type; for a spell, the form level and the current missile `σ` / beam sway amplitude.
- Buttons → actions (each logs what it did):
  - `initNow` *Initialize now* (only while pending);
  - `reinitAll` *Re-initialize all* (needs the checkbox *I am sure* first);
  - `dumpState` *Dump state to log*;
  - `dumpWeapons` *Dump weapon classes* ([`weapon-class.md`](weapon-class.md));
  - `dumpSpells` *Dump player spells* ([`spell-classifier.md`](spell-classifier.md));
  - `reloadConfig` *Reload config* (TOML files and MCM INI);
  - `giveXP {id, raw}`: sub-skill list + number input (goes through the normal award, source
    `debug`);
  - `setCurrent {id, level}`: testing only (raises potential and peak if needed).
- MCM debug settings are shown read-only (they are changed in MCM).

## Config

`[ui]` in [`config/SkillPractice.toml`](../config/SkillPractice.toml): `toggle_key`, `refresh_ms`,
`inspector`.

## Test

## Prototype notes

- Vanilla HTML/CSS/JS view in `ui/`, integration in `plugin/src/UI.cpp`.
- Browser checks in `ui/test.html` pass in headless Chrome: all 6 / 76 rows,
  filter, sorting, tabs, JSON actions, reinitialization guard and Escape.
- Serve `ui/` over HTTP for mock mode. A direct `file://` page cannot fetch JSON
  under normal browser security rules. `ui/mock.json` is generated by PracticeTests.
- API 1.5.1 V2 falls back to V1; absence is nonfatal. Runtime F7/focus, game
  controls, callback thread and live updates remain checks for the user.

- Browser: open `ui/index.html` with `mock.json`: both tabs render, sorting and filter work.
- Game: `[boot]` logs the PrismaUI interface version; F7 opens the panel with the cursor, F7 / Esc
  closes it and the game controls come back; values update while the panel is open and the game
  runs behind it; no crash when opening it in combat.
- *Give XP* from the panel: the row and the main line change on the next refresh.
- Without PrismaUI installed: the game starts, `[ui] PrismaUI not available`, XP still works.
