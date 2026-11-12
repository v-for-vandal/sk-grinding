# Prototype 1: task for the implementing agent

> Give the agent this prompt: **"Read `plan/prototype-1.md` in this repository and carry it out."**
> Everything it needs is linked from here.

## 1. Your task

You implement the first playable prototype of **Skill Practice**, an SKSE plugin
(CommonLibSSE-NG, C++) for the LoreRim 5.0+ mod list (Skyrim AE 1.6.1170, SKSE 2.2.6). The
repository so far holds only the design. The prototype must let the user (the designer) install the
mod, load their level-50 character or start a new game, and **watch** how sub-skills, floors,
potential, peaks, decay and rust behave while playing, with debug switches that speed things up.

The user tests in the game; you build. You probably cannot run Skyrim yourself, so every
milestone ends with: a clean build, passing unit tests, an updated package in `dist/`, and a short
checklist the user can run in game (in `plan/prototype-1-testing.md`, see §6).

Before anything else read [`AGENTS.md`](../AGENTS.md): it states the core model in a few lines.
The most important rule: **sub-skills earn XP; the main skill level is computed from sub-skill
levels by a formula. There is no "XP goes to the main skill" step anywhere.**

## 2. What the prototype contains

1. Install and open instructions (MO2 package, how to install PrismaUI, how to open the panel).
2. A **status panel** with, per sub-skill: own floor ("current floor"), effective floor, current,
   potential, peak, the idle meter and decay rate (the sub-skill's "rust"); per main skill: the
   computed level, the written level, rust `R` and its idle meter.
3. **Start conversion** of the existing character (level-50 save) and of a new game.
4. The **weapon keyword layout** (KID config, keyword records, DLL mapping, runtime dump).
5. A simple **MCM** with a *Debug* page: XP multiplier ×1 / ×10 / ×100, and *Make all XP equal
   to 100* (every action gives a fixed 100 XP from our own events, so testing does not depend on
   the vanilla XP hook working; [`xp-capture.md`](../impl/xp-capture.md) *Debug: fixed XP per
   action*).
6. In the same page: faster forgetting (time scale) and "no grace period".
7. Sub-skills of six main skills only: **One-Handed, Two-Handed, Block, Smithing, Destruction,
   Restoration**.
8. **Spell inaccuracy:** launch noise of `missile` spells and aim sway of `beam` spells, plus
   console instructions to learn a missile and a beam Destruction spell.
9. **Logging** good enough to debug the user's reports from the log alone.

**Not in the prototype** (do not build, even if the design describes it): anti-grind (budgets,
diminishing repetition, damage pool, crafting bucket), books and trainers, any perk work
(requirements, our own perks), continuous effects on game values (weapon damage, block stamina,
heal magnitude), cross-links, other main skills (armor, archery, alteration, …), jail rule,
external increments → potential, Legendary block, archetype lists, Smithing
`units × tier` formulas.

## 3. Read these, in this order

| # | File | Why |
|---|---|---|
| 1 | [`AGENTS.md`](../AGENTS.md) | core model, conventions |
| 2 | [`design-v2.md`](../design-v2.md) §3, §4.2, §4.4, §5.1–5.5, §12–14 | the rules you implement |
| 3 | [`impl/README.md`](../impl/README.md) | what an impl block is |
| 4 | [`impl/plugin-foundation.md`](../impl/plugin-foundation.md) | project, build, ESL, logging, package |
| 5 | [`impl/subskill-state.md`](../impl/subskill-state.md) | values, XP award, co-save |
| 6 | [`impl/main-skill-level.md`](../impl/main-skill-level.md) | formula, rust, writes, start conversion |
| 7 | [`impl/decay.md`](../impl/decay.md) | active time, idle meter, decay |
| 8 | [`impl/xp-capture.md`](../impl/xp-capture.md) | vanilla XP hook, context, routing per skill |
| 9 | [`impl/weapon-class.md`](../impl/weapon-class.md) | weapon keywords |
| 10 | [`impl/spell-classifier.md`](../impl/spell-classifier.md) | spell form and sub-direction |
| 11 | [`impl/spell-aim-inaccuracy.md`](../impl/spell-aim-inaccuracy.md) (section *Prototype 1*) | missile noise, beam sway |
| 12 | [`impl/status-ui.md`](../impl/status-ui.md), [`refs/frameworks.md`](../refs/frameworks.md) §6 | panel (PrismaUI) |
| 13 | [`impl/mcm-config.md`](../impl/mcm-config.md) | config files, MCM |
| 14 | [`research/R-17-raw-use-points.md`](../research/R-17-raw-use-points.md), [`R-26`](../research/R-26-spell-aim.md), [`R-27`](../research/R-27-new-game-init.md) | runtime checks you prepare and the user runs |
| 15 | [`config/`](../config/) | all numbers; `config/skills/*.toml` define the sub-skills |
| — | per-skill design when a rule is unclear: [weapons](../skills/weapons/design.md), [block](../skills/block/design.md), [smithing](../skills/smithing/design.md), [magic](../skills/magic/design.md), [destruction](../skills/destruction/design.md), [restoration](../skills/restoration/design.md) | background |
| — | [`refs/frameworks.md`](../refs/frameworks.md), [`refs/vanilla-skill-xp.md`](../refs/vanilla-skill-xp.md), [`refs/lorerim/weapons.md`](../refs/lorerim/weapons.md) | background |

The impl files are the specification. If an impl file and the design disagree, the impl file
wins for the prototype (its *Prototype* notes are deliberate simplifications); tell the user.

## 4. Prototype assumptions (not design decisions)

| Topic | Prototype choice | Open in design |
|---|---|---|
| Form sub-skills | per school: `destruction/missile`, `restoration/self`, … | Q-CAT-1 |
| Archetype | only `<school>/standard` | R-7 |
| Destruction sub-directions | the proposal list | Q-DES-1 |
| Raw XP | engine use points × `base_mult` (= vanilla `UseMult`) | Q-XP-9 |
| Decay units | levels per active day | Q-DYN-4 |
| Hysteresis of main writes | +1.0 / −0.2 | Q-ML-6 |
| External changes of a main skill | added to `base` (kept) | Q-XP-4 |
| New game init | lazy: first XP event after the race menu | R-27 |
| Start floors | all non-concept sub-skills | Q-ML-9 |
| Opening the panel | hotkey F7 (config) | Q-UI-1 |
| Config format | TOML (+ MCM Helper INI / JSON) | Q-UI-4 |
| All numbers | placeholders in `config/` | many |

## 5. Milestones

Work on a branch `prototype-1`. Commit at the end of each milestone (do not push unless the user
asks). After each milestone update the *Status* line of the impl files you implemented
(`in progress` / `done`) and add a short **Prototype notes** section to an impl file wherever the
code does something the file did not say (a hook you had to choose, an API that differs).

**M0. Skeleton.** Project per [`plugin-foundation.md`](../impl/plugin-foundation.md): build,
logging with tags, TOML config loading (all files, values logged), the ESL generator with the 22
weapon keywords, packaging into `dist/`. Unit-test project with the `k` solver and
`levelFromXP` already (they are pure functions). *Done when:* the package installs, the game
starts, `[boot]` lists 6 managed skills and the sub-skill count of `config/skills/`, and all
keywords resolve.

**M1. Telemetry (R-17).** The `UseSkill` vfunc hook, log only (call the original): `[use]`
lines; log-only hook on `PlayerSkills::AdvanceSkill`; sinks for the context events of
[`xp-capture.md`](../impl/xp-capture.md) with timestamps and a frame counter; module owner of the
hooked vfunc. Write the user's checklist for R-17 checks 1–4 (which actions to do, what to send
back). *Done when:* the build is ready for the user's test. Continue with M2 meanwhile; when the
user sends the log, write the results into `research/R-17-raw-use-points.md` and adjust
`xp-capture` if needed.

**M2. State, formula, save, init, panel (read-only).** [`subskill-state.md`](../impl/subskill-state.md),
[`main-skill-level.md`](../impl/main-skill-level.md) (formula, writes, start conversion; rust
fields exist but stay 0 until M4), co-save, the PrismaUI view with the `Sub-skills` tab of
[`status-ui.md`](../impl/status-ui.md) (develop the page in a browser with `mock.json` first), debug buttons *Dump state*, *Give XP*, *Set current*,
*Initialize now*, *Re-initialize all*. *Done when:* the level-50 save loads with unchanged main
skills and plausible floors; Give XP raises a sub-skill and the main level follows; save / load
keeps everything.

**M3. XP routing.** [`xp-capture.md`](../impl/xp-capture.md) for the six skills (with
suppression), [`weapon-class.md`](../impl/weapon-class.md), [`spell-classifier.md`](../impl/spell-classifier.md),
`Smithing` material / category tables, the fixed-XP mode (its event sinks; the MCM toggle comes
in M5, until then the TOML key), the dump buttons, the `Equipped` part of the `Debug` page.
Build the fixed-XP mode first if R-17 shows that `UseSkill` gets no points.
*Done when:* the `[xp]` lines of the test checklist name the expected sub-skills; no
`[ctx] unresolved` in a normal session.

**M4. Active time, decay, rust.** [`decay.md`](../impl/decay.md), rust in
[`main-skill-level.md`](../impl/main-skill-level.md). *Done when:* with time scale ×1000 and no
grace, a sub-skill falls to its floor and a neglected main skill rusts; sleeping does not count.

**M5. MCM.** [`mcm-config.md`](../impl/mcm-config.md): quest, script, `config.json`, INI reading
on journal close. *Done when:* every control changes behaviour and is logged.

**M6. Spell inaccuracy.** Section *Prototype 1* of
[`spell-aim-inaccuracy.md`](../impl/spell-aim-inaccuracy.md), R-26 checks 1, 2, 4, 5, 6 written
into the testing checklist. *Done when:* the user sees Ice Spikes scatter at low
`destruction/missile` and the crosshair wander while holding Sparks at low `destruction/beam`,
and both stop at level 75 (*Set current*).

**M7 (optional, ask the user first).** Window rules of the concept sub-skills (`with-shield`,
`dual-wield`, `with-magic`), [weapons](../skills/weapons/design.md) *Window rules*.

**M8. User documents.** `plan/prototype-1-testing.md` complete (§6).

## 6. The user document: `plan/prototype-1-testing.md`

Write it in English, short, as numbered steps. It must contain:

1. **Install PrismaUI** (not in LoreRim): download *Prisma UI* 1.5.1 from Nexus (Skyrim SE mod
   148718) → MO2 *Install a new mod from archive* → enable it (no plugin file; it is an SKSE DLL
   and data under `PrismaUI/`). Its requirement *Media Keys Fix SKSE* is already in LoreRim. Check
   after the first start: `Documents\My Games\Skyrim Special Edition\SKSE\PrismaUI.log` exists
   and has no errors (1.6.1170 support is not stated officially: if the DLL does not load, stop
   and send that log).
2. **Install the mod:** build or take `dist/SkillPractice.zip`; MO2 → *Install a new mod from
   archive*; enable it; put `SkillPractice.esp` anywhere after its masters; other requirements are
   in LoreRim.
   Optional for clean numbers: SSL off per R-16 (MCM points per level 0, level multiplier 0).
3. **Open the panel:** F7 (the mouse cursor appears, the game keeps running); F7 or Esc closes
   it. MCM → *SkillPractice* → *Debug* for the switches.
4. **Learn test spells** (console, `~`), with a check that the IDs are right in this load order:
   ```
   help "Ice Spike" 4          ; lists matching forms; the SPEL from Skyrim.esm is 0002B96C
   player.addspell 0002B96C    ; Ice Spike  — Destruction, missile (frost)
   player.addspell 0002DD29    ; Lightning Bolt — Destruction, beam, fire-and-forget (shock)
   player.addspell 0002DD2A    ; Sparks — Destruction, beam, concentration (shock)
   player.addspell 0001C789    ; Fireball — explosion (optional, to compare)
   tgm                         ; god mode: no magicka limit while testing (tgm again to turn off)
   ```
   Then check in the panel's *Debug* page that the equipped spell shows the expected form
   (`missile` / `beam`). If not, the classifier is wrong: send the *Dump player spells* log.
5. **Scenarios**, each with what should be seen:
   - level-50 save: load, open the panel: main skills unchanged, floors `= clamp(2 × (main − 15), 0, 75)`;
   - *Make all XP equal to 100* on: each hit, block, bash, cast, craft gives 100 XP to the expected
     rows (`src=fixed`); use it if normal XP does not arrive;
   - XP ×100: hit with a sword, block with a shield, bash, craft, cast Ice Spike, heal: which rows
     move, and that the main level follows the formula;
   - forgetting ×1000 + no grace: rows fall to their floors; a main skill rusts; using a mace keeps
     One-Handed from rusting while the sword falls; sleep / fast travel do not count;
   - a weak skill (Two-Handed, Destruction): low start floors, so the first levels count little
     towards the main skill (the rise up to level 25); compare how fast its main level moves with
     a strong skill;
   - new game: skills untouched until the first hit / cast after character creation, then
     `[init]` in the log;
   - spell inaccuracy: Ice Spike at a wall at low and at 75 `destruction/missile` (*Set current*);
     Sparks held at low and at 75 `destruction/beam`; first and third person;
   - the R-17 / R-26 / R-27 checks you prepared.
6. **Logs:** where `SkillPractice.log` (and `PrismaUI.log`) is, which MCM switches to set before reproducing a problem
   (log level debug, verbose XP), what to send (the log, the action, game time, a screenshot of
   the panel).

## 7. Rules

- Player only; never change NPCs.
- No hard-coded numbers that the config has; new numbers go to `config/` with a comment.
- Forms are referenced by FormKey (`Plugin.esp|0xLocalID`), resolved at `kDataLoaded`; a missing
  plugin or form is logged and the rule is skipped, never a crash.
- Game state is touched only on the main thread; the UI reads snapshots (status-ui *Threads*).
- Hooks only through Address Library IDs or vtables; log every hook installed.
- Docs in English. Do not change design decisions; if the code
  cannot follow the spec, write it in the impl file (*Prototype notes*) and tell the user.
- Questions in docs follow the repo convention (`- **Q-…** …` / `- **A:**`).
- When blocked on a decision that is the user's, ask; do not guess silently.
- Untrusted downloads (archives from the internet) go into their own new folder; never run code
  from inside them.
