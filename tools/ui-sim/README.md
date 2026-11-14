# UI simulator: the status panel without the game

Runs the PrismaUI panel ([`ui/`](../../ui/), [`impl/status-ui.md`](../../impl/status-ui.md)) in
a normal browser, backed by **the real model**: `PracticeSim` links the same `PracticeCore`
(`plugin/src/Core.cpp`, `Config.cpp`) as the DLL and reads the same [`config/`](../../config/).
For fast work on the panel and on the balance of the numbers.

## What it does and what it does not

- **Real:** sub-skill XP, zones, floors, potential, peak, decay, rust (once per award), the main
  level formula and the written level with hysteresis, config loading and *Reload TOML*. The
  panel's own buttons (*Give XP*, *Set current*, *Re-initialize all*, *Reload config*) work.
- **Simulated:** player actions. A preset ([`presets.json`](presets.json)) names the sub-skills
  directly (what `runtime::route` would pick) and the vanilla use points, which are
  **placeholders until R-17**. The *Points* field overrides them.
- **Formula only:** the aim view draws missile scatter (σ from the form level) and the beam sway
  path with the same formulas as `Aim.cpp`. That a projectile really deviates in the game is not
  shown here (R-26).
- **Not available:** everything in the game: hooks, context resolution (which weapon / spell),
  threads, co-save, PrismaUI focus and input, Ultralight rendering. *Dump …* and *Close* only log
  "needs the game".

## Run

Build `PracticeSim` (a target of [`plugin/CMakeLists.txt`](../../plugin/CMakeLists.txt); it needs
only `toml++` and `nlohmann-json`, no CommonLib):

```
# Windows: scripts/build-prototype.ps1 builds it (build/prototype/Release/PracticeSim.exe);
# core only (no CommonLib, ESP, Papyrus): scripts/build-prototype.ps1 -CoreOnly
# Linux / any CMake with toml++ and nlohmann-json found:
cmake -S plugin -B build/sim -DPRACTICE_PLUGIN=OFF && cmake --build build/sim --target PracticeSim
```

Then:

```
python tools/ui-sim/serve.py                       # finds build/prototype or build/sim
python tools/ui-sim/serve.py --sim path/to/PracticeSim --base 30 --port 8077
```

Open `http://127.0.0.1:8077/`. Only localhost is served. `--base` is the start level of every
main skill (the start conversion as in the game); *Re-initialize* in the panel changes it.

## Pieces

| File | Role |
|---|---|
| `plugin/sim/PracticeSim.cpp` | the model as a process: one JSON action per stdin line → one snapshot line (the game's snapshot plus a `sim` object) |
| [`serve.py`](serve.py) | HTTP server (standard library): serves `ui/`, injects the bridge into `index.html`, forwards `POST /sim/action` |
| [`dev-bridge.js`](dev-bridge.js), [`dev.css`](dev.css) | replaces the native `window.spAction` with HTTP calls; adds the *Simulator* panel |
| [`presets.json`](presets.json) | simulated player actions |

Nothing here is packaged with the mod (`scripts/package-prototype.ps1` copies only
`ui/index.html`, `app.js`, `style.css`). The panel files are not changed: the bridge is
injected only by `serve.py`.

Actions accepted by `PracticeSim` besides the panel's own: `use {skill, subs, points, times}`,
`tick {hours}` (active hours; decay and rust also × `debug.time_scale`), `set {section, key,
value}`, `equip {equipped}`, `reinitAll {base}`, `snapshot`.
