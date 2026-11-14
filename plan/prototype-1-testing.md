# Prototype 1: in-game checklist

0.1.4 (review fixes, built blind): build and check first as described in
[`prototype-1-review.md`](prototype-1-review.md).

Build: `dist/SkillPractice.zip` (MO2 archive). M0–M6 are implemented; M7
concept-window XP is excluded. The C++/Papyrus builds, pure tests and browser
checks pass. **Successful in-game validation is still pending.** Use a separate test save.

Startup hotfix 0.1.1: replace the previous MO2 mod from the rebuilt archive.
Load hotfix 0.1.3 also replaces recursive SKSE task reposting with a queue
drained by the player update. Expect `[load] complete`, then
`[load] first player update: complete` and periodic `[health]` lines.
Save loading and panel interaction are confirmed by the player and the
2026-11-12 log (`first player update: complete` and subsequent health entries).
Beam sway is player-confirmed; missile spread remains unresolved.
The test session saved all 76 sub-skills and six main skills, but persistence
after reload and individual XP awards still need verification.

Diagnostics correction: MCM Helper writes a UTF-8 BOM to its user INI. Earlier
builds skipped the first section header, so a leading `[Log]` section did not
enable Debug or Verbose XP. The rebuilt archive strips the BOM before parsing.
Reinstall the archive and restart the game; expect `[cfg] log.level` and
`[cfg] log.xp_verbose` changes, followed by debug entries. Existing test actions
need not all be repeated: first repeat one qualifying fixed-XP action to check
that its `[xp]` record is present.
Expect `[boot] SkillPractice 0.1.1`, `main-task initialization thread=...`,
and `[ui] DOM ready thread=...` with the same thread ID. A file-view mock.json
fetch error must no longer appear. First check that the main menu stays
responsive before loading a save. The old log captured a loader-thread ID as
main, which could cause main-thread context events to repeatedly enqueue themselves.
The fix is built/tested locally; its in-game result still needs confirmation.

1. **Install PrismaUI.** Download [Prisma UI 1.5.1](https://www.nexusmods.com/skyrimspecialedition/mods/148718).
   MO2 → Install a new mod from archive → enable it. It has an SKSE DLL and
   `PrismaUI/` data, no ESP. Its Media Keys Fix SKSE requirement is already in
   LoreRim. After the first launch inspect
   `Documents\My Games\Skyrim Special Edition\SKSE\PrismaUI.log`.
   Official documentation does not name 1.6.1170 explicitly: if the DLL fails
   to load, stop and send that log.
2. **Install Skill Practice.** MO2 → Install a new mod from archive → select
   `dist/SkillPractice.zip` → enable it and `SkillPractice.esp`. Put the ESP
   after Skyrim.esm. SKSE 2.2.6, Address Library, KID, OCF, SkyUI and MCM Helper
   are already in LoreRim. Launch the Default profile via SKSE. For clean numbers
   set SSL MCM points per level and level multiplier to zero (R-16).
   Do not use Legendary resets; this prototype does not block them.
3. **Check boot.** In `Documents\My Games\Skyrim Special Edition\SKSE\SkillPractice.log`
   expect `[boot] managed=6 subskills=76`, 22 weapon keywords resolved, magic
   keyword resolution and hook-owner lines. Missing optional form rules are
   skipped with a warning. A missing PrismaUI API is nonfatal to XP but prevents
   using the panel. Send the log if configuration or hooks fail.
4. **Open the panel.** F7 opens it with a cursor and pauses the game.
   F7, Escape or Close closes it. Open it outside other menus. Verify controls
   return and the game resumes after closing. Sub-skills shows own/current floor, effective floor,
   current, potential, peak, idle U, decay/day, zone, worth and last raw XP;
   each main skill shows computed/written level, base, rust and idle meter.
   Debug shows equipped classes and spell forms. Filter and column sorting work.
5. **Enable diagnostics.** MCM → SkillPractice → Debug → Log level Debug,
   Verbose XP on. Close the journal; expect `[cfg]` changes in the log.
   XP multiplier, forgetting speed, fixed XP, no grace, main writes, vanilla
   passthrough and both aim switches are here. Restart/load and verify settings
   persist. Leave passthrough off except when explicitly comparing vanilla.
6. **Existing level-50 character.** Load the test copy of the save. Main skills
   must keep their pre-install base values. Each non-concept sub-skill starts at
   `clamp(2 * (its main skill - 15), 0, 75)`; concepts start at zero. Character
   level 50 is not substituted for the individual main skill values.
   Use Dump state before and after a save/reload: values must survive.
7. **Panel actions.** Give XP to `1h-sword`; current/potential/peak and its main
   formula should rise, floor grows from raw XP. Set current to a test level
   (e.g. 90). Initialize now handles pending skills. Re-initialize all requires
   I am sure and wipes progress using current main levels as the new baseline.
   Dump state, Dump weapon classes, Dump player spells and Reload config write
   tagged logs. Do not reinitialize the save whose persistence you are testing.
8. **Learn test spells.** In the console (`~`), first verify matching SPEL forms
   in this load order. The following IDs refer to Skyrim.esm:
   ```text
   help "Ice Spike" 4
   player.addspell 0002B96C    ; Ice Spike: Destruction missile/frost
   player.addspell 0002DD29    ; Lightning Bolt: beam, fire-and-forget/shock
   player.addspell 0002DD2A    ; Sparks: beam, concentration/shock
   player.addspell 0001C789    ; Fireball: explosion (optional comparison)
   tgm                        ; no magicka limit; tgm again turns this off
   ```
   Verify the equipped form and direction in Debug. If they differ, use Dump
   player spells and send the log; LoreRim may replace vanilla effects.
9. **Fixed XP.** Enable Make all XP equal to 100; leave XP multiplier ×1.
   Hit with a sword/greatsword, punch, block, bash, cast and craft once each.
   Every qualifying action gives raw 100 per routed sub-skill (`src=fixed`);
   shield bash gives `bash` 100 and its shield row 50. Every spell feeds form,
   direction and standard, once per cast (concentration: once per cast start).
   Use this mode if normal engine points are absent. Tempering is not covered
   by fixed mode. Smelting/tanning and alchemy/enchanting are excluded.
10. **Normal routing / fast growth.** Turn fixed XP off and XP multiplier ×100.
    Hit with a sword (`1h-sword`), greatsword (`2h-sword`), punch (`1h-unarmed`),
    block with a shield (`heavy-shield`/`light-shield`), parry (`parry`), bash
    (`bash`, plus half to a shield), craft/temper, cast Ice Spike and heal.
    Ice Spike should feed destruction/missile, frost and standard; self heal
    restoration/self, help and standard. Smithing feeds its material and category,
    while clothing jewelry feeds jewelry only. Main levels follow the formula.
    Dump weapon classes also tests post-KID coverage and multiple keywords.
11. **Decay and rust.** Raise sword current above its floor. Enable forgetting
    ×1000 and no grace, then stop using One-Handed: current falls to its floor,
    potential/peak stay, R grows and computed main falls. Practise a mace:
    One-Handed rust should recover while unused sword proficiency may decay.
    R has a cap; floor/potential/peak never decay. Sleep 8 hours and fast-travel:
    activeHours must exclude those jumps. Paused menus must add no active time.
12. **Weak versus strong skill.** Compare early progress of a low-floor
    Two-Handed/Destruction row with a high-floor One-Handed row. Levels below
    the conversion rise checkpoint (25) count little towards the main skill.
    This affects the main formula, not how much sub-skill XP the action awards.
13. **New game / R-27.** Start a test character. No start conversion or vanilla
    suppression should occur before RaceSex has closed and that skill's first
    XP event (or Initialize now). Check `[ctx]` RaceSex opens/closes and `[init]`
    baselines. Try Alternate Perspective's mirror/start selection and note
    when Norden/SSL change skills. Save/load before first practice: pending
    skills must stay pending. Report whether this initialization is late enough.
14. **R-17 check 1.** With debug logging, perform each action three times,
    leaving two seconds between different actions: 1H/2H hit, punch, shield
    block, weapon block, shield/weapon/torch bash, forge craft, temper, smelt,
    tan, Destruction hit, self heal, heal other, ward absorbing a spell.
    Note the action order and game time. `[use]` records skill, points, third
    argument type/ID/name, microseconds and frame. Nonzero pre-UseMult points
    are the evidence needed; zero/no calls means fixed mode is the fallback.
15. **R-17 checks 2–4.** Read a skill book; separately test SSL level-up on the
    test save. Look for `path=AddSkillExperience` and `[main] external`.
    The second hook is log-only AddSkillExperience (the pinned CommonLib has
    no PlayerSkills::AdvanceSkill declaration/ID); absence is not proof of full
    path coverage. `[ctx]` versus `[use]` timestamps show event ordering.
    Send the complete log including `[boot]` original-module ownership lines.
16. **Missile aim / R-26 checks 1–2.** Initialize Destruction and set
    destruction/missile to 0 on a test save, then shoot Ice Spike at a wall
    20 times; repeat at 75. Expect spread at 0 and none at 75. For the fixed
    launch probe, edit the installed TOML `aim.debug_missile_yaw_deg=10`, Reload
    config, and fire again; then restore it to 0. Test dual casting, multiple
    projectiles and a lobber if available. Compare TDM lock-on on/off; report
    whether homing cancels the noise. NPC missiles and Fireball stay unchanged.
17. **Beam aim / R-26 checks 4–6.** Set destruction/beam to 0. Charge Lightning
    Bolt, then hold Sparks for at least three seconds: crosshair/aim should
    wander smoothly and the spell should follow it. Stop casting: disturbance
    stops. Repeat at 75: no sway. Repeat in first/third person with default
    SmoothCam and TDM, and compare their disabled state if aiming is overwritten.
    `[aim] sway start/stop` confirms caster-state detection; report any movement
    without a corresponding casting action. Toggle both aim MCM controls too.
18. **Send a report.** Reproduce with Debug logging and Verbose XP enabled;
    send `SkillPractice.log`, `PrismaUI.log` for UI issues, the action/time/order,
    equipped item/spell, camera/TDM state and a screenshot of the panel. Logs
    are replaced on the next start, so copy them before launching again.
    Game-only R-17/R-26/R-27 results are pending until these logs are received.
