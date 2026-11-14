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
- **Debug mode** (MCM toggle, `ui.debug_mode`, snapshot field `debugMode`): off → only the bar
  view of *Sub-skills*, no summary line, no tabs; on → the summary line, the *Debug* tab and the
  *Debug view* checkbox (the sub-skill table instead of the bars).
- Plain HTML + JS (ES2022 at most), no build step needed; no WebGL; light CSS (rendering is on
  the CPU at ≤ 60 FPS: no blur, shadows, big gradients).
- The page must work in a normal browser with mock data: if `window.spAction` does not exist,
  load `mock.json` (a saved snapshot) instead. So the UI can be developed and checked without the
  game.

## Data flow

- **C++ → JS:** `InteropCall(view, "spUpdate", json)` with the whole snapshot as one JSON string
  (format below). Sent **only** when the view opens and after each action from it; no periodic
  refresh. **Decided (2026-11-14, design-v2 §12.1):** the game is paused while the panel is open,
  so nothing else changes the values; a refresh loop would only rebuild the page (Ultralight
  renders on the CPU) and add code. If the pause fails, the values are those of the moment of
  opening; reopening the panel updates them. Revisit if the panel ever stays open while the game
  runs (e.g. a HUD widget).
- **JS → C++:** `RegisterJSListener(view, "spAction", OnAction)`; JS calls
  `spAction(JSON.stringify({action, ...args}))`. `OnAction` parses the JSON, logs `[ui] action`,
  and runs the action on the main thread (PrismaUI runs callbacks on the main thread since 1.1;
  check the thread id once and log it; if it is not the main thread, queue the action for our next
  tick).
- JS `console.*` output → our log through `RegisterConsoleCallback` (V2) as `[ui] js: …`.
- The snapshot is built on the main thread from our live state; JS never reads game data.

Snapshot (JSON, one object):

```json
{ "state": "running|pending", "activeHours": 12.4, "debugMode": false, "debug": {"xpMult":100,"timeScale":1,"noGrace":false,"fixedXp":false,"writeMain":true},
  "skills": [ { "av": "OneHanded", "name": "One-Handed", "F": 81.37, "toNext": 0.37, "written": 81, "base": 80,
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
- Open: send a snapshot, `Show`, `Focus(view, pauseGame = true)` (mouse cursor, game paused and controls
  disabled while open). Close (the same key, `Esc` handled in JS → `spAction({action:"close"})`, or
  a *Close* button): `Unfocus`, `Hide`.
- `ui.inspector = true` (config): `CreateInspectorView` + `SetInspectorVisibility` for the
  Ultralight inspector (DOM, console). Off by default (first open is slow).

## `Sub-skills` tab

**Bar view** (default; the only view without debug mode). One collapsible block per main skill,
title: name, the written level and a small bar (about the width of the word *Destruction*, same
colours as the sub-skill bars) with the progress to the next written level: `toNext =
clamp((F − written) / main.hysteresis_up, 0, 1)` (written becomes `n + 1` at `F ≥ n + up`),
1 at `main.max_level`; tooltip *Next level* with the percentage. With *Write main skills* off,
`written` does not move, so the bar fills and stays full (debug only). One row per sub-skill: name (last part of the id, prefix
`1h-`/`2h-` dropped), a bar, the current level (integer). The bar (first sketch, 2026-11-14;
colours and sizes are placeholders, CSS variables `--bar-*` in `style.css`):

- `[0, floor]` colour 1 (dark gold), `[floor, current]` colour 2 (bright gold); the floor is the
  effective floor;
- the rest of the bar is empty: black at 15 % opacity, thin light outline;
- `[current, potential]` colour 2 hatched (diagonal stripes);
- one scale for all bars: 0–100, extended in steps of 25 when a value is higher;
- tooltip on each part, with its level: floor *Drilled into the bones*, current *Learned*,
  potential *Been there once*. Our own element (`#tip`) following the mouse, not the `title`
  attribute: Ultralight passes native tooltips to the host app, and PrismaUI is not known to draw
  them *(verify: hover works in game)*. Re-evaluated after each snapshot, since the bars
  are rebuilt under a still cursor.

Filter and *Hide untouched* apply here too.

**Debug view** (table, debug mode + checkbox *Debug view*). Header line (debug mode only): mod state (pending init / running), active game hours, `xp_mult`, `time_scale`,
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

- Options (both views): *Hide untouched* (rows where current = floor = potential = 0), text filter. A value
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

`[ui]` in [`config/SkillPractice.toml`](../config/SkillPractice.toml): `toggle_key`,
`inspector`.

## Prototype notes

- UI simulator ([`tools/ui-sim/`](../tools/ui-sim/README.md)): the panel in a browser backed by
  `PracticeSim` (the real `PracticeCore` model, no game), with simulated player actions, time,
  settings and an aim-formula view. For panel work and balance; not a game check.

- 0.1.1: mock fetching is restricted to HTTP(S) browser previews. In PrismaUI's
  file view, the native spAction binding may arrive after app.js executes; that
  interval must not start mock mode. File-startup regression checks this case.

- Vanilla HTML/CSS/JS view in `ui/`, integration in `plugin/src/UI.cpp`.
- Browser checks in `ui/test.html` pass in headless Chrome: all 6 / 76 rows,
  filter, sorting, tabs, JSON actions, reinitialization guard and Escape. Since the bar view
  (headless Firefox): 76 bars, *Debug view* toggles the table and back, debug mode off hides the
  debug controls. `mock.json` has `debugMode: true`.
- Serve `ui/` over HTTP for mock mode. A direct `file://` page cannot fetch JSON
  under normal browser security rules. `ui/mock.json` is generated by PracticeTests.
- API 1.5.1 V2 falls back to V1; absence is nonfatal. Runtime F7/focus, game
  controls, callback thread and live updates remain checks for the user.

## Test

- Browser: open `ui/index.html` with `mock.json`: both tabs render, sorting and filter work.
- Game: `[boot]` logs the PrismaUI interface version; F7 opens the panel with the cursor, F7 / Esc
  closes it and the game controls come back; the game is paused while it is open; no crash when opening it in combat.
- *Give XP* from the panel: the row and the main line change at once (the snapshot after the
  action).
- Without PrismaUI installed: the game starts, `[ui] PrismaUI not available`, XP still works.
