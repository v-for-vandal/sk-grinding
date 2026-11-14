# MCM and configuration

- **Status:** done (Settings page with debug-mode group built; in-game MCM registration/controls pending).
- **Design:** design-v2 §12.2 (MCM Helper + config file), §13 (Debug page), Q-UI-4 (format: TOML
  for the plugin's own files, prototype choice; JSON only where MCM Helper needs it).
- **Depends on:** [`plugin-foundation.md`](plugin-foundation.md) (ESL quest, Papyrus build).
- **Research:** none.

## Configuration files

| File (installed path under `Data/`) | Repo | Content |
|---|---|---|
| `SKSE/Plugins/SkillPractice/SkillPractice.toml` | [`config/SkillPractice.toml`](../config/SkillPractice.toml) | global settings and debug defaults |
| `SKSE/Plugins/SkillPractice/skills/*.toml` | [`config/skills/`](../config/skills/) | one file per managed main skill |
| `SKSE/Plugins/SkillPractice/smithing-materials.toml` | [`config/smithing-materials.toml`](../config/smithing-materials.toml) | material keywords |
| `SKSE/Plugins/SkillPractice/magic-directions.toml` | [`config/magic-directions.toml`](../config/magic-directions.toml) | sub-direction keywords |
| `SPO_WeaponClass_KID.ini` | [`config/kid/`](../config/kid/) | read by KID, not by us |
| `MCM/Config/SkillPractice/config.json`, `settings.ini` | written in this task | MCM page and defaults |

- TOML parser: `toml++` (header-only, vcpkg `tomlplusplus`).
- Read at `kDataLoaded`; *Reload config* (debug panel) re-reads all of them. A parse error logs
  the file, line and message and keeps the previous values (at first load: built-in defaults
  equal to the repo files).
- Every value read is logged once at `debug` level (`[cfg] key = value`).

## MCM (MCM Helper 1.6.2, in LoreRim)

- Mod name (folder and INI names): `SkillPractice`.
- Records in `SkillPractice.esp`: quest `SkillPractice_MCM` (Start Game Enabled), script
  `SkillPractice_MCMScript extends MCM_ConfigBase` with an empty body; a reference alias
  `PlayerAlias` filled with the player (Specific Reference `PlayerRef`), script
  `SKI_PlayerLoadGameAlias`. This is the standard MCM Helper setup (its wiki, *Getting Started*);
  follow the wiki if it differs.
- **No custom Papyrus logic.** All controls are settings stored by MCM Helper in
  `Data/MCM/Settings/SkillPractice.ini` (defaults in `MCM/Config/SkillPractice/settings.ini`).
  The DLL reads the defaults and then the user file (user values win) at `kPostLoadGame`,
  `kNewGame` and every time the `Journal Menu` closes (MCM lives there), and logs each change
  (`[cfg] debug.xp_mult 1 → 100`).
- Under MO2 the user INI is written to the overwrite folder or the mod folder; reading
  `Data/MCM/Settings/SkillPractice.ini` through the virtual file system finds it either way.

**Page `Settings`** (one page in prototype 1). The first control is the toggle *Debug mode*
(`groupControl: 1`); every other control (and the header *Debug* above them) has
`groupCondition: 1` and `groupBehavior: "hide"` *(verify in game)*, so they are shown only in
debug mode. Debug mode only changes what is shown (MCM controls, panel debug views); the values
of hidden controls still apply.

| Control | Type | INI key | Values → config key |
|---|---|---|---|
| Debug mode | toggle | `[General] bDebugMode` | `ui.debug_mode` ([`status-ui.md`](status-ui.md)) |
| XP multiplier | enum | `[Debug] iXpMult` | ×1 / ×10 / ×100 → `debug.xp_mult` 1 / 10 / 100 |
| Make all XP equal to 100 | toggle | `[Debug] bFixedXP` | `debug.fixed_xp` (value: `debug.fixed_xp_value`, TOML only); see [`xp-capture.md`](xp-capture.md) *Debug: fixed XP per action* |
| Forgetting speed | enum | `[Debug] iTimeScale` | ×1 / ×10 / ×100 / ×1000 → `debug.time_scale` |
| No grace period | toggle | `[Debug] bNoGrace` | `debug.no_grace` |
| Write main skills | toggle | `[Debug] bWriteMain` | `main.write_main_skills` |
| Vanilla XP passthrough | toggle | `[Debug] bPassthrough` | `debug.vanilla_passthrough` |
| Missile launch noise | toggle | `[Aim] bMissile` | `aim.missile_enabled` |
| Beam aim sway | toggle | `[Aim] bBeam` | `aim.beam_enabled` |
| Verbose XP log | toggle | `[Log] bXpVerbose` | `log.xp_verbose` |
| Log level | enum | `[Log] iLevel` | trace / debug / info / warn → `log.level` |

Each control has a help text (MCM Helper `help`) saying what it does and that it is for
testing. Enum values are stored as an index; the DLL maps the index to the value.

## Prototype notes

- Generator creates the start-enabled quest at SkillPractice.esp|0x816, its
  PlayerRef alias and VMAD scripts; ESP read-back validates their presence.
- To support an existing save without relying on SEQ generation, the DLL starts
  this quest on post-load/new-game if it is neither running nor starting.
- CK's compiler is copied locally under .tools, with a CLR 4 activation config;
  the original compiler/game directory is untouched. Our empty script compiles
  with zero warnings and errors against the MCM Helper public SDK.
- Default INI values load first, then the MO2-visible user INI; enum indices
  are range checked. The final log level and all changes are logged.
- Closing the journal re-reads only the two MCM INI files (0.1.4); the TOML files are re-read
  at load, new game and by *Reload config*.
- A key missing in the user's `SkillPractice.toml` is taken from the compiled copy and logged
  as a `[cfg]` warning (runtime code reads every key with `.at()`, which would throw inside a
  hook). Rows of `smithing-materials.toml` and the Destruction / Restoration rows of
  `magic-directions.toml` are checked at load.
- First-load TOML failures use compiled copies of the repository TOML files.
  Reload failures retain the previous complete configuration. Pure tests verify
  compiled defaults match repository files. Duplicate sub-skill entries are
  reported and skipped; unsupported main skills are rejected in this prototype.

## Test

- Change *XP multiplier* to ×100, close the journal: one `[cfg]` line; the next sword hit gives
  100 times the XP of the previous one (`[xp]` lines).
- Toggle *No grace period* with *Forgetting speed* ×1000: decay starts at once in the panel.
- Restart the game: the MCM values are still set and are applied after load.
