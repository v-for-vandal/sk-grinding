# Prototype 2: task for the implementing agent

> Give the agent this prompt: **"Read `plan/prototype-2.md` in this repository and carry it out."**
> Everything it needs is linked from here.

## 1. Your task

You continue **Skill Practice**, the SKSE plugin (CommonLibSSE-NG, C++) for LoreRim 5.0+ (Skyrim
AE 1.6.1170, SKSE 2.2.6). Prototype 1 ([`prototype-1.md`](prototype-1.md)) built six main skills,
the panel, decay, rust and spell inaccuracy. Prototype 2 turns it into the **whole mod in a first
playable form**:

1. **All sub-skills of all main skills** from the design, with XP routing. Where the design has
   not settled a skill yet, use the prototype assumption of §5 (and say so in the panel / log).
2. **Continuous effects applied to the game** for every sub-skill whose effects are designed,
   and **shown in the panel** with their current value.
3. **Perk changes:** sub-skill requirements on existing perks (purchase and effects), main-level
   changes, restored Ordinator perks.
4. **Correct start initialization** of every skill: new game, mid-playthrough install, skills
   added to the config later.
5. **Everything else in [`impl/`](../impl/README.md) that is decided and not built yet.**

The user (the designer) tests in the game; you build. You probably cannot run Skyrim yourself,
so every milestone ends with a clean build, passing unit tests, an updated package in `dist/` and
steps for the user in `plan/prototype-2-testing.md` (§8).

Read [`AGENTS.md`](../AGENTS.md) first. The most important rule still holds: **sub-skills earn
XP; the main skill level is computed from sub-skill levels by a formula. There is no "XP goes to
the main skill" step anywhere.**

## 2. Scope per skill

What exists in the design (status of 2026-11-15). "Designed" = decided in the skill's
`design.md`; "proposal" = a list or rule marked as a proposal or with an open question; "—" =
not designed. Perk rows = perks with a filled *Resolution* / *Sub-skill requirements* in the
worksheets.

| Skill | Sub-skills | XP | Continuous effects | Perk changes |
|---|---|---|---|---|
| One-Handed | designed | designed (+ concept windows) | designed: damage, glancing, attack stamina, attack speed (if R-37 check 4), dual-wield / with-shield / with-magic factors | 27 of 37 (+ ORD-LA-05/08 restored here) |
| Two-Handed | designed | designed | as One-Handed | none yet |
| Archery | designed | designed (routing not built) | designed: damage; accuracy (launch noise + bow sway, **numbers missing**) | none yet |
| Block | designed | designed (Q-BLK-2 other sources open) | designed: block stamina cost, bash damage | 19 of 19 |
| Heavy Armor | designed (AE / modded materials open, Q-ARM-3) | designed (hits × slot weights, casting window, movement script) | designed: armor rating, stamina on hit, speed, material rating (**tier tables F1/F2 missing**), casting factor | 4 of 17 + 7 restored Ordinator |
| Light Armor | as Heavy Armor, + `clothing` | as Heavy Armor | as Heavy Armor + clothing flat armor | 4 of 18 + 7 restored Ordinator |
| Smithing | categories designed, materials open (Q-SMI-5) | designed, formulas wait for R-15 | — | 5 restored Ordinator |
| Destruction | proposal (Q-DES-1) | vanilla per cast | — (Q-DES-2) | none yet |
| Restoration | designed | designed ([`heal-xp.md`](../impl/heal-xp.md)) | designed: help, wards, sun, poison magnitude | 1 restored Ordinator |
| Alteration | designed | vanilla per cast | — (only the floor rule, done in core) | 5 restored Ordinator |
| Conjuration | designed | designed (summon damage, fallback combat time) | — | none yet |
| Illusion | proposal (Q-ILL-1) | open (Q-ILL-2) | — (Q-ILL-3) | none yet |
| Shared magic (forms, archetypes) | designed | every cast | accuracy (done); others none | — |
| Enchanting | listed | open (Q-ENC-1, Q-ENC-2) | — | none yet |
| Speech | designed | designed | — | none yet |
| Alchemy | designed | mostly designed (Q-ALC-3 open) | — | free-text notes only |
| Lockpicking, Pickpocket, Sneak | one each | vanilla | — | none yet |

Rules for the gaps:

- **No invented design.** A skill with "—" gets **no** continuous effect; the panel shows "no
  effects designed yet" for its sub-skills. A missing number (archery accuracy, armor tier
  tables) gets a placeholder in `config/` marked `# PLACEHOLDER (prototype 2)` and is listed in
  the testing document as "needs the designer's numbers".
- Perks without a resolution in the worksheets keep their LoreRim requirements.
- A worksheet resolution written as free text that you cannot turn into a formula (Alchemy) is
  listed in the testing document and asked, not guessed.

## 3. Already done (core, Linux, unit-tested)

In `PracticeCore` (`plugin/src/Core.*`, `Config.cpp`, `Requirements.*`), covered by
`PracticeTests`; the DLL was **not built** on Windows after these changes:

- **Cross-links and the Alteration floor** ([`impl/cross-links.md`](../impl/cross-links.md)):
  effective floor in the catch-up zone and the decay stop, `config/cross-links.toml`, snapshot
  `effFloor` / `floorFrom`.
- **Perk requirement formulas** ([`impl/perk-requirements.md`](../impl/perk-requirements.md)
  *Core implementation*): parser, evaluation with hysteresis, `config/perks/*.toml` (One-Handed
  and Block), `Model::updateRequirements()`, co-save and snapshot key `requirements`.
- No own-floor cap (`floor_cap` removed).
- Shared magic forms and archetypes; pseudo-skill `magic` in the panel.
- `scripts/package-prototype.ps1` copies `cross-links.toml` and `perks/*.toml`.
- Panel display rows (`[[effect]]`) for weapon damage, glancing, attack stamina and speed, block,
  restoration (not applied yet).

## 4. Read these, in this order

| # | File | Why |
|---|---|---|
| 1 | [`AGENTS.md`](../AGENTS.md), [`prototype-1.md`](prototype-1.md) §5–§7 | core model; build, package, milestone and rule conventions (still valid) |
| 2 | [`prototype-1-testing.md`](prototype-1-testing.md), [`prototype-1-review.md`](prototype-1-review.md) | what is verified in game and what is still open from prototype 1 |
| 3 | [`design-v2.md`](../design-v2.md) §3–§6, §11–§14 | rules (§4.4.4 start conversion, §5.6 sub-skill perks, §5.7 effect curve, §6 cross-links) |
| 4 | [`impl/README.md`](../impl/README.md) and **every** impl file whose status is not `done` | the specification of this prototype |
| 5 | [`skills/README.md`](../skills/README.md), then each skill's `design.md` | sub-skills, XP, effects |
| 6 | the perk worksheets with resolutions: [`one-handed.perks.md`](../skills/weapons/one-handed.perks.md), [`block/perks.md`](../skills/block/perks.md), [`heavy-armor.perks.md`](../skills/armor/heavy-armor.perks.md), [`heavy-armor.perks-ordinator.md`](../skills/armor/heavy-armor.perks-ordinator.md), [`light-armor.perks.md`](../skills/armor/light-armor.perks.md), [`light-armor.perks-ordinator.md`](../skills/armor/light-armor.perks-ordinator.md), `smithing/`, `alteration/`, `restoration/`, `alchemy/` `perks-ordinator.md` | perk changes |
| 7 | research with open checks: [R-17](../research/R-17-raw-use-points.md), [R-23](../research/R-23-block-stamina.md), [R-25](../research/R-25-perk-condition-injection.md), [R-26](../research/R-26-spell-aim.md), [R-27](../research/R-27-new-game-init.md), [R-28](../research/R-28-mage-ward.md), [R-29](../research/R-29-restored-ordinator-perks.md), [R-33](../research/R-33-requiem-mass-effect.md)–[R-37](../research/R-37-melee-non-damage-levers.md) (index: design-v2 §15.2) | checks you prepare and the user runs |
| 8 | [`config/`](../config/), [`refs/lorerim/`](../refs/lorerim/), [`research/tables/`](../research/tables/) (Git LFS) | numbers, keywords, LoreRim data |

The impl files are the specification. If an impl file and the design disagree, the impl file
wins for the prototype; tell the user. If a block is `proposed` (variant not chosen), do its
research checks first, pick the variant the file prefers when the checks allow it, record the
choice in the file (*Prototype notes*), and tell the user.

## 5. Prototype assumptions (not design decisions)

| Topic | Prototype choice | Open in design |
|---|---|---|
| Destruction sub-directions | the proposal list (as prototype 1) | Q-DES-1 |
| Illusion sub-directions | the proposal list; classified by magic-effect keywords like the other schools (`config/magic-directions.toml`) | Q-ILL-1 |
| Illusion XP | vanilla use points per cast, routed to the sub-direction, the form and the archetype (as other schools) | Q-ILL-2 |
| Enchanting XP | vanilla use points; `enchanting-weapons` / `enchanting-armor` / `enchanting-other` by the enchanted item's type; staves and wands as in [`enchanting/design.md`](../skills/enchanting/design.md) if R-10 allows, else `wands-staffs` gets nothing | Q-ENC-1, Q-ENC-2 |
| Smithing materials | the placeholder list in `config/skills/smithing.toml`; raw vanilla points | Q-SMI-5, R-15 |
| Armor AE / modded materials | `heavy-other` / `light-other` | Q-ARM-3 |
| Armor material tiers F1 / F2 | placeholder table in config (tier 1 +5 / +10, tier 2 +10 / +20, tier 3 +15 / +30 % at 100 / 200); tier of each material from [`refs/lorerim/armor-material-tiers.md`](../refs/lorerim/armor-material-tiers.md) once filled, else tier 1 | user's tables |
| Archery accuracy | the spell `missile` numbers (σ 6° at 0, none from 75) and a bow sway like the beam sway | Q-WPN-7 numbers |
| Attack speed | only if R-37 check 4 passes; placeholder curve | numbers |
| Thief skills, Speech, Alchemy, Conjuration, Alteration, Enchanting, Smithing effects | none | each skill's design |
| Reference paths and budgets of new skill files | your proposal per skill (§6 M1), approved by the user before balancing | per skill |
| Alchemy cross-links | none (design-v2 §6.2 says "none"; Q-ALC-1 mentions ×1/2: ask) | Q-ALC-1 |
| External changes of a main skill | added to `base` (as prototype 1) | Q-XP-4 |
| Hysteresis of main writes | +1.0 / −0.2 (as prototype 1) | Q-ML-6 |

## 6. Milestones

Work on a branch `prototype-2` from `design-2`. Commit at the end of each milestone (push only
when the user asks). After each milestone update the *Status* line of the impl files you touched
and add *Prototype notes* where the code differs from the file. Keep `ui/mock.json`
(`PracticeTests config ui/mock.json`; keep `"debugMode": true`) and the `PracticeSim` actions in
step with the snapshot.

**M0. Build and the full registry.**
- Clean Windows build of the current tree (`scripts/build-prototype.ps1`); fix the DLL sources
  for the core changes of §3. CTest passes.
- Remove the prototype-1 list of managed skills (`Config.cpp`, "unsupported prototype skill"):
  every skill with a file in `config/skills/` is managed.
- Write `config/skills/*.toml` for every missing main skill (Archery, Heavy Armor, Light Armor,
  Alteration, Conjuration, Illusion, Enchanting, Speech, Alchemy, Lockpicking, Pickpocket,
  Sneak) with the sub-skills of §2, groups, concept sub-skills `k = 0`
  (`heavy-armor-casting`, `light-armor-casting`), the fixed `k` of the movement sub-skills
  ([armor](../skills/armor/design.md) *Movement in armor*), `start_split` flags (§M1), and a
  **reference path and budgets** for `scripts/normalize_conversion.py`: propose them in the file
  header (three-member paths where a skill has three or more main sub-skills, as the magic
  schools), run the script, regenerate `plugin/tests/normalization.json`, and list the proposals
  for the user.
- ESL: everything new of this prototype (globals of M5, our perk records of M4 / M5, armor
  material keywords if needed); `tools/esp-gen` reads the config, a unit test checks ESP ↔
  config.
- *Done when:* `[boot]` lists 18 managed skills and the sub-skill count; the level-50 save loads
  with unchanged main skills.

**M1. Start initialization** ([`impl/main-skill-level.md`](../impl/main-skill-level.md)
*Design change: split of the main XP*, design-v2 §4.4.4, [R-27](../research/R-27-new-game-init.md)).
- Replace the `2 × (main − 15)` mapping by the split of the main XP over the sub-skills with
  `k > 0` and `start_split = true` (bisection on `M_i`, saturation, leftover into `base`).
  `start_split = false`: concept sub-skills, Smithing materials with tier > 1
  ([smithing](../skills/smithing/design.md)), anything the skill design excludes. Remove
  `[init] floor_per_level` / `floor_zero_at` except where the shared forms still use them
  (design-v2 §4.4.4: forms `max(0, 2 × (best school − 15))`).
- **Every case:** new game (lazy init after the race menu, as prototype 1); **mid-playthrough
  install** (a save without our co-save data: initialize every skill at `kPostLoadGame` from the
  player's current base levels); **a skill or sub-skill added to the config later** (only the
  new ones initialize; existing progress untouched); *Initialize now* / *Re-initialize all*.
- Unit test: every skill file × main ∈ {15, 20, 25, 40, 60, 100}: after init `F = main` within
  0.01; a mid-game save with half the skills known keeps them.
- R-27 checks 1–4 into the testing document.

**M2. XP routing for all skills** ([`impl/xp-capture.md`](../impl/xp-capture.md); per-skill rules
in each `design.md`).
- Weapons: Archery routing by class (Q-WPN-2 keywords). **Concept window rules**
  (`dual-wield`, `with-shield`, `with-magic`; [weapons](../skills/weapons/design.md) *Window
  rules*): the M7 of prototype 1 that was skipped.
- Armor: hits taken × slot weights to the type and the material sub-skill; casting window to
  `*-armor-casting`; **movement**: [`impl/armor-movement-xp.md`](../impl/armor-movement-xp.md)
  (our `REQ_PassiveArmorTraining.pex` in the package, native `SkillPractice.ArmorMovement`).
- Restoration: [`impl/heal-xp.md`](../impl/heal-xp.md) (needs *player-hit-hook*, M3).
- Conjuration: summon damage with the combat-time fallback ([conjuration](../skills/conjuration/design.md)
  Q-CON-2; write the block file `impl/summon-xp.md` first, status `chosen`). Bound weapons train
  both sub-skills (Q-WPN-6).
- Alteration, Illusion: per cast like Destruction ([`impl/spell-classifier.md`](../impl/spell-classifier.md),
  `config/magic-directions.toml` rules for each school).
- Speech, Alchemy, Enchanting, Smithing, Lockpicking, Pickpocket, Sneak: as their design (or §5).
  Speech trading: log the daily total per merchant as the design says.
- **Mod-list XP preset** and per-skill user factors ([`xp-capture.md`](../impl/xp-capture.md)
  *Design change*, `config/xp-presets.toml`).
- *Done when:* the fixed-XP mode and a normal session give `[xp]` lines naming the expected
  sub-skills for every skill (checklist per skill in the testing document); no `[ctx]
  unresolved` in a normal session.

**M3. Shared hooks and the effect curve** (planned blocks of [`impl/README.md`](../impl/README.md):
write each block file before coding it).
- *effect-curve*: design-v2 §5.7 from config (`curve` / `linear` rows), the same code for the
  panel and the game.
- *player-hit-hook*: hits where the player attacks or is hit; coexists with Precision 2.0.4
  (pre-hit API, [R-37](../research/R-37-melee-non-damage-levers.md) check 3) and Parrying RPG.
- *hit-damage-multiplier*: one multiplier per player hit after vanilla and Requiem perks
  (design-v2 Q-DYN-9 option 3): weapon damage, `dual-wield` bonus, `bash` damage, glancing hits.
- *projectile-launch-hook*: the existing spell launch hook generalized for arrows and bolts
  (archery accuracy); the open prototype-1 issue "missile noise has no effect"
  ([`prototype-1-review.md`](prototype-1-review.md) §3) is fixed here first.

**M4. Continuous effects in the game.** For each, the block file, its research checks first,
then the code; every effect is player-only and reverts exactly (deltas against the last applied
value).
- Weapons: damage, glancing hits, effective-level factors of `dual-wield` / `with-shield` /
  `with-magic`; attack stamina and attack speed ([`impl/weapon-effects.md`](../impl/weapon-effects.md),
  R-37 checks 1, 2, 4).
- Block: [`impl/block-stamina-cost.md`](../impl/block-stamina-cost.md) (R-23), bash damage.
- Armor: [`impl/armor-effects.md`](../impl/armor-effects.md) (armor rating by type and material,
  clothing flat armor, stamina on hit, movement speed; R-33, R-34, R-36 checks) and the casting
  factor ([armor](../skills/armor/design.md) *Casting in armor*).
- Restoration: help / ward / sun / poison magnitude ([restoration](../skills/restoration/design.md),
  R-28 for wards).
- Archery: damage, accuracy (M3 hook).
- Cross-links and the Alteration floor at runtime: `model.externalLevels[av]` from the player's
  **base** actor value for Alteration and every unmanaged main skill a requirement formula names
  (with all skills managed this is only for safety), on tick and on load.
- Automatic sub-skill perks (design-v2 §5.6): only the placeholder rows exist; build the granting
  mechanism (add / remove with hysteresis `[perks] hysteresis`) and keep the placeholders
  disabled unless the user designs real ones.

**M5. Perk changes** ([`impl/perk-requirements.md`](../impl/perk-requirements.md),
[`impl/perk-tree-nodes.md`](../impl/perk-tree-nodes.md), R-25, R-29).
1. **Config rows.** Convert every worksheet resolution into `config/perks/*.toml` rows (format:
   `config/perks/one-handed.toml`): One-Handed 1H-04, 1H-12, 1H-23 (missing today), Heavy Armor,
   Light Armor, the restored Ordinator perks of Heavy / Light Armor, Smithing, Alteration,
   Restoration. Free-text resolutions you cannot formalize: ask (§9).
2. **Requirement globals in our ESL**: one `GLOB` (short, 0) per row with `requires`, EditorID
   `Requirement::global` (`SPO_Req_<handle without '-'>` unless the row sets `global`); test that
   ESP and config agree.
3. **Write the globals**: after every award, decay tick, game load, config reload and when the
   perks menu opens (`StatsMenu`): `model.updateRequirements()`, set the changed `TESGlobal`
   values; on load set all. Log `[req] <handle> met|lost` (debug).
4. **Condition injection at `kDataLoaded`**, per row, perk found by FormKey (log and skip when
   absent): Requires gets `GetGlobalValue <global> == 1` (AND); `main_level = N` changes the
   comparison value of the perk's own `GetBaseActorValue <skill>` item, `"drop"` removes it (for
   restored Ordinator perks it is the Ordinator skill); `has_perk_replace` swaps the `HasPerk`
   item (put FormKeys into the rows); entry points (perk-owner tab) and the effects of abilities
   the perk gives get `GetIsReference Player == 0 OR GetGlobalValue <global> == 1` unless
   `effects_gated = false`; R-25 checks first; a condition dump of one perk of each kind.
5. **Changed effects** (`magnitudes`, e.g. Fight or Flight `StaminaRate` 5 → 2) and **restored
   tree nodes** (Rushing Tide and Hissing Dragon under Takedown; the Heavy / Light Armor,
   Smithing, Alteration, Restoration restorations): [`impl/perk-tree-nodes.md`](../impl/perk-tree-nodes.md),
   R-29 checks first.

**M6. Panel** ([`impl/status-ui.md`](../impl/status-ui.md)).
- All 18 skills and the pseudo-skill `magic`; long lists stay usable (collapse per skill, the
  "hide untouched" filter).
- **Effects:** each effect row shows its **current value** at the sub-skill's effective level
  (and the level the effect uses when a concept factor lowers it), `applied` true / false, and
  the curve in the tooltip; a sub-skill without designed effects says so.
- `effFloor` with `floorFrom` ("from 1h-sword ×0.5", "from Alteration").
- **Perks:** per sub-skill the perks whose requirement names it, with the formula, met / lost and
  the `explain` tree; automatic perks with owned / lost.

**M7. MCM** ([`impl/mcm-config.md`](../impl/mcm-config.md) *Design change* notes): XP-scale page
(preset and per-skill factors), the panel hotkey control, switches for each effect group
(weapons, block, armor, restoration, archery, perk gating) so the user can compare with and
without (needed by [`playtesting/weapon-damage.md`](../playtesting/weapon-damage.md)).

**M8. User documents.** `plan/prototype-2-testing.md` complete (§8); set the playtesting
scenarios that become playable to `ready` in [`playtesting/README.md`](../playtesting/README.md).

## 7. In-game checks (put them into the testing document)

- **Init:** new game → nothing until the first action after character creation, then `[init]`
  for each skill with `F = main`; the level-50 save without our data → every main skill
  unchanged, sub-skills from the split; add a new sub-skill to a TOML, load → only it starts.
- **Cross-links:** raise `1h-sword` until its own floor is ~60 (debug *Set current* + XP);
  `1h-mace` shows effective floor 30 from `1h-sword`; mace XP below 30 runs at ×3; with *no
  grace* and time scale ×1000 a mace at 50 decays to 30, not to its own floor.
- **Alteration floor:** Alteration 50 → every magic sub-skill shows effective floor 20 from
  Alteration.
- **Perks menu:** 1H-05 Mace Focus is not buyable at `1h-mace` 44 with OneHanded ≥ 25, buyable
  at 45; 1H-10 shows OneHanded 35 (was 50); BLK-12 Torch Combat needs no Block level, only
  `bash` 25.
- **Gating:** buy Mace Focus, set `1h-mace` to 42: its power-attack bonus is gone, back at 45;
  between 43 and 45 it stays (hysteresis); a requirement inside the band stays met after reload.
- **NPCs:** a Requiem NPC with Mace Focus keeps its effects; no effect of ours touches NPCs.
- **Each effect:** the value in the panel equals the measured change (damage per hit, stamina
  per attack / per hit taken, armor rating in the inventory, speed, block cost, heal amount),
  at level 0, 50, 100, 200 (*Set current*); switching the effect group off in the MCM restores
  the vanilla value exactly.
- **XP per skill:** the action list per skill and the expected `[xp]` rows.

## 8. The user document: `plan/prototype-2-testing.md`

As in prototype 1 (§6 there): install / update steps, what changed since prototype 1, the steps
of §7 per milestone, the research checks you prepared (R-17, R-23, R-25–R-29, R-33–R-37), the
list of placeholders that need the designer's numbers, logs to send.

## 9. Rules and questions

- All rules of [`prototype-1.md`](prototype-1.md) §7 hold (player only, numbers in config, forms
  by FormKey, main thread, Address Library hooks, docs in English, ask instead of guessing).
- Do not change design decisions. Where the design is open, use §5 and list it; where a block is
  `proposed`, the research checks decide, and you tell the user what was chosen.
- Ask the user (collect, do not block other work):
  - reference paths and budgets of the new skill files (M0);
  - free-text perk resolutions (Alchemy ORD-ALC-*; ORD-ALC-09 names `alchemy/poison`, the
    sub-skill id is `alchemy-poisons`);
  - Alchemy cross-links (Q-ALC-1 vs design-v2 §6.2);
  - archery accuracy numbers, armor material tiers (F1 / F2 and the tier of each material);
  - anything a research check rules out.
- Docs fixes found while preparing this plan, not yet done: design-v2 Q-DYN-9 has an empty
  *A* although the weapons design says "option 3"; `Q-ML-9` is cited (prototype-1 plan, R-27,
  `SkillPractice.toml`) but does not exist in design-v2. Tell the user, do not fix silently.
