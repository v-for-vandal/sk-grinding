# Skill Practice Overhaul — Design Document (draft v2)

> Status: **draft v2**. Translated from the original draft (`design.md`, kept unchanged),
> with consistent terminology and decisions from the first review round applied.
> Companion document: [`grinding-analysis.md`](grinding-analysis.md).
> Technical reference notes: [`refs/`](refs/): [`aim-perturbation.md`](refs/aim-perturbation.md),
> [`per-target-budget.md`](refs/per-target-budget.md),
> [`perk-requirements.md`](refs/perk-requirements.md),
> [`dynamic-skill-effects.md`](refs/dynamic-skill-effects.md),
> [`frameworks.md`](refs/frameworks.md) (KID, OCF, AVG, CommonLibSSE-NG),
> [`vanilla-skill-xp.md`](refs/vanilla-skill-xp.md) (vanilla XP formulas and per-action values).
> **Per-skill design and perk worksheets: [`skills/`](skills/README.md)** (one folder per skill).
> **Implementation blocks: [`impl/`](impl/README.md)** (what to build, variants, dependencies);
> research entries with details: [`research/`](research/).
> LoreRim data worksheets: [`refs/lorerim/`](refs/lorerim/): [`materials.md`](refs/lorerim/materials.md),
> [`weapons.md`](refs/lorerim/weapons.md).
> Tools: [`scripts/normalize_conversion.py`](scripts/normalize_conversion.py) (conversion
> normalization, §4.4).

## How to answer questions in this document

Every open question has an ID and an empty answer line:

- **Q-XXX-n.** Question text.
  - **A:**

Write your answer right after `**A:**` (as many lines as you need, indented under it).
Leave it empty if you have not decided yet. Items marked **Proposal** are my suggestions;
answer with "ok", "no" or a modification. On the next round I will pick up every non-empty
`**A:**`, fold it into the text, and delete the question.

---

## 1. Context

- The mod is an **SKSE plugin** written in C++ with **CommonLibSSE-NG**, plus a small ESP/ESL
  (perks, keywords, globals, MCM quest) and a small amount of Papyrus (MCM glue only).
- Target modlist: **Lorerim 5.0+** (Requiem-based Wabbajack modlist). Important Lorerim facts
  already known:
  - Runtime: **Skyrim AE 1.6.1170 + SKSE 2.2.6** (CK 1.6.1378). Research of the installed
    LoreRim 5.0 profile (2026-11-11): [`refs/lorerim/`](refs/lorerim/), report in the research
    dump (§15.2).
  - Character level is **decoupled from skills**. It grows through the **Experience** mod from
    kills, discovering and clearing locations and quests (`bEnableSkillXP = false`). So slowing
    down main-skill growth does not slow down character leveling.
  - **Use-based skill leveling is disabled** in LoreRim. All 18 skill AVIFs have `UseMult = 0`
    and `ImproveOffset = 9999`. Skills grow only through **Static Skill Leveling Rewritten (SSL)**,
    which distributes 30 points per character level (max 5 per skill). Our mod replaces SSL
    (§4.1).
  - Clothing experience goes into **Light Armor** in Lorerim.
  - Trainings: Requiem's `iTrainingNumAllowedPerLevel = 5` is still in effect. *Simply Smart
    Training* carries unused lessons over to later levels. (The earlier note "limit removed" was
    wrong.)
  - Thu'um-improving perks live in the **Speech** tree in Lorerim.
  - Perk trees are a mix of Requiem + Magic Redone + Special Feats + Ordinator + LoreRim patches.
    The full list is in the per-skill worksheets `skills/<skill>/perks.md`.
  - Keywords must be referenced by **FormKey** (plugin + local ID). Requiem renamed many vanilla
    keywords (e.g. `ArmorMaterialEbony` is now `REQ_ArmorSet_Ebony`, same FormKey).
- Development and testing happen on a separate Windows machine with Skyrim, Lorerim and
  Creation Kit. This document and the implementation plan are prepared on Linux, without
  access to the game.
- The implementation plan will be executed by a less capable agent, so the design must be
  explicit: every number configurable, every rule stated as a formula or an algorithm.

## 2. Goals and principles

**Goal:** rework how skill experience is earned and give more granular, per-activity effects
of skills.

**Principles:**

1. **Just play.** Skills relevant to what the player actually does level up automatically;
   no menus to allocate anything.
2. **No grinding.** Repetitive, riskless actions must not produce meaningful progress
   (e.g. letting a skeever bite you for an hour must not level armor). See §11 and
   `grinding-analysis.md`.
3. **No omnipotence.** A character cannot stay at peak in everything at once: unused
   proficiencies fade (but never below what has been "drilled in").

## 3. Glossary (canonical terms)

| Term | Meaning |
|---|---|
| **Main skill** | One of the 18 vanilla skills (One-Handed, Smithing, …), i.e. the engine actor value. |
| **Sub-skill** | A new, mod-defined proficiency (e.g. `sword`, `ebony-armor`, `missile`). Each sub-skill belongs to exactly one main skill (its *parent*). |
| **Current** | Present level of a sub-skill. Design scale **0–200**, technical cap `hardCap` = 500 (§5.1). Goes up with use, decays without use. Drives sub-skill perks. |
| **Floor** | "Muscle memory". Current never decays below floor. Has two parts: **own floor** (earned by the sub-skill itself, capped at `floorCap` = 75) and **effective floor** (own floor raised by cross-links, see §6). |
| **Potential** | Best level ever reached, additionally raised by books, trainers and other external sources. Below potential, current grows faster ("recalling"). Invariant: `potential ≥ current`. |
| **Sub-skill XP** | Experience earned by a sub-skill from a gameplay event. |
| **Main skill level** | Computed from sub-skill levels (§4.4), not accumulated. Vanilla XP is suppressed; the only source of main-skill growth is sub-skill progress. |
| **Conversion** | `g(L)` and its integral `M_i(L)`: how much a sub-skill at level `L` is worth to its main skill, defined per sub-skill (rises from 0 over the first levels, then falls off, so `M_i` saturates; §4.4). |
| **Rust** | `R`, the main skill's own decay, in levels. Grows when the whole main skill is not practised, shrinks with its practice (§4.4.3). |
| **Idle meter** | `U`, active hours of "not practising", reduced in proportion to raw XP. Drives the grace period of sub-skill decay (§5.5) and of rust. |
| **Cross-link** | A rule that one sub-skill's own floor raises another sub-skill's effective floor. |
| **Use event** | Any event that grants XP to a sub-skill; reduces its idle meter in proportion to the raw XP (§5.5). |
| **Window rule** | A rule that grants XP depending on whether a complementary event happened within the last N seconds (dual-wield, with-shield, with-magic). |
| **Effect curve** | Shared shape of continuous effects: penalty to 0 at a checkpoint, then a bonus saturating towards a limit (§5.7). |
| **Effective level** | Level used by a sub-skill's continuous effects: `current × factor`, lowered by context penalties (`with-shield`, `dual-wield`, `with-magic`). Does not affect `current`, perks or the main skill (§5.7). |
| **Concept sub-skill** | A sub-skill about a way of fighting or casting, not an item (`dual-wield`, `with-shield`, `with-magic`). `k = 0`: no main-skill contribution. |
| **Zone multiplier** | XP multiplier that depends on where current is relative to floor and potential (§5.3). |

## 4. Core model: experience flow

### 4.1 Decisions

- **Level model.** The main skill level is **computed from the levels of its sub-skills**
  (§4.4). It can go down: the main skill **rusts** when the whole skill is not practised, and
  vanilla effects that read the main skill follow. (Replaces the earlier XP-flow model with
  monotonic main skills: [`old/main-xp-transfer.md`](old/main-xp-transfer.md).)
- **Vanilla XP is fully suppressed.** The engine's own skill XP awards are intercepted and
  zeroed. Main skills grow only through §4.2. (In LoreRim they are already zero, since
  `UseMult = 0`. The hook still provides the *raw* use amount as base XP; see Q-XP-9.)
- **SSL is disabled** (decided). Static Skill Leveling Rewritten no longer distributes skill
  points at level-up; our mod is the only source of main-skill growth. Preferably SSL is switched
  off via its MCM/INI without removing it from the load order (research R-16). Otherwise its
  distribution would turn into a stream of "external increments" (next point) and flood
  potential.
- **Per-sub-skill conversion.** Each sub-skill has its own conversion to main XP, possibly
  non-linear (§4.4). Example design target for `jewelry`: taking `jewelry` from 0 to 75 should
  produce in total only enough main XP to raise Smithing from 15 to 35.
- **Several sub-skills per event (decided, Q-XP-1: option (a)).** When one event feeds several
  sub-skills (sword hit with a shield → `1h-sword` + `with-shield`; ebony cuirass hit →
  `heavy-armor` + `ebony-armor`; fireball → form + archetype + sub-direction), every sub-skill
  converts its own XP and the contributions add up. This is safe because conversion is
  normalized per main skill against a reference path (§4.4.5), for **every** skill.
- **External increments become potential.** Any non-mod increase of a main skill (quest
  rewards, `AdvanceSkill`/`IncrementSkill` from scripts, Oghma Infinium, etc.) is intercepted
  and converted into a **potential** increase for **all sub-skills of that main skill**.
  Example: "Smithing +5" → potential of every Smithing sub-skill increases. Books and trainers
  follow the same idea with their own rules (§10).

### 4.2 XP pipeline (per gameplay event)

```
gameplay event
  → determine affected sub-skills and raw XP for each           (per-skill rules, skills/)
  → anti-grind modifiers                                       (§11)
  → trainer bonus (if active, ≤ +30%)                          (§10)
  → zone multiplier (×3 / ×1.5 / ×1)                           (§5.3)
  → current XP += result; own floor XP += floorRate × result   (§5.4)
  → potential = max(potential, current)
  → peak XP = max(peak XP, current XP)
  → main skill: reduce idle meter U_main and rust R by raw XP; recompute main level,
    write it with hysteresis                                    (§4.4)
  → re-evaluate sub-skill perks (with hysteresis)              (§5.6)
```

### 4.3 Open questions

- **Q-XP-2.** ~~Convert Δpeak raw or with the recall multiplier?~~ No longer applies: the main
  skill is computed from levels (§4.4), so there is no Δpeak conversion. Books and trainers
  speed up sub-skills (×1.5 recall up to potential), and the main skill follows. Accepted
  (comparable to vanilla books raising the skill directly); see
  [`old/main-xp-transfer.md`](old/main-xp-transfer.md) §2.
- **Q-XP-4.** External increments → potential: what is the exchange rate? Proposal:
  `+N main levels` → `+2N potential` (because sub-skills use a 0–200 scale) on every sub-skill
  of that main. And for XP-based script awards (`AdvanceSkill(x)`): convert x to the equivalent
  number of main levels at the current main level, then apply the same rule?
  - **A:**
- **Q-XP-5.** Vanilla XP multipliers (standing stones like the Warrior Stone, Well Rested,
  Lover's Stone, Fortify Skill Rate potions/effects, i.e. `<Skill>SkillAdvance` actor values):
  apply them to sub-skill XP, to main XP, or ignore?
  - **A:**
- **Q-XP-7.** Target pacing (how many hours of normal play from 15 to 100 in a main skill,
  how many hours from 0 to 100/200 in an actively used sub-skill). See §15.1 for what we know
  about vanilla and the proposal to measure it.
  - **A:**

- **Q-XP-9.** **Base XP when `UseMult = 0`.** The design uses the engine's per-action XP amount as
  base XP. In LoreRim the AVIF multiplier is 0, so after the multiplier the amount is 0. Proposal:
  hook the call that receives the **raw use points** before the AVIF multiplier
  (`PlayerCharacter::UseSkill`/`PlayerSkills::UseSkill` *(verify which one applies the
  multiplier)*). Our own per-skill base multipliers (config) then replace `UseMult`. If raw
  points are not reachable, compute base XP per event ourselves (per-skill rules in skills/). Runtime test:
  R-17.
  - **A:**

### 4.4 Main skill from sub-skill levels (decided)

The main skill level is **computed from the sub-skill levels via XP analysis**, not accumulated from XP. It can
go down when the whole skill is not used ("rust"), so every vanilla, Requiem or Ordinator effect
that reads the main skill follows it: prices, pickpocket chance, magnitudes, perk conditions
such as `GetActorValue Smithing >= 50`. The previous model (XP transfer with a peak rule) and
the comparison are in [`old/main-xp-transfer.md`](old/main-xp-transfer.md).

#### 4.4.1 Formula

```
M_i(L)      = k_i · Σ_{l=0}^{L-1} cost_sub_i(l) · gn_i(l + 0.5)     // main-XP "worth" of sub-skill i at level L
L_i^main    = max(current_i, peak_i − R)                            // R = rust of the main skill, §4.4.3
main        = level_from_xp(base, Σ_i (M_i(L_i^main) − M_i(L_i^0)))  // base, L_i^0: §4.4.4
```

- `gn = g/k` is the normalized conversion, a **rise** times a **fall**:
  ```
  g(L)    = k · rise(L) · fall(L)
  rise(L) = x²·(3 − 2x),  x = clamp(L / T1, 0, 1)          // smoothstep: 0 at L = 0, 1 from L = T1
  fall(L) = (1 + e^(−T/w)) / (1 + e^((L − T)/w))         // normalized logistic, halved around L ≈ T
  ```
  - **Rise (decided, T1 = 25).** The first levels of a fresh sub-skill are worth little to the main
    skill; the full rate is reached at `T1`. This removes the breadth bias of a plain falloff
    (§4.4.2). `T1` is global (config), with a per-sub-skill override.
  - **Fall** keeps its meaning: `T` is where the conversion has halved, `w` its width.
  - `k`, `T`, `w` are per sub-skill (config). Usually `k` is shared by a group of sub-skills (e.g.
    all Smithing materials), and `T` varies with how hard the sub-skill is.
  - **`k = 0`** means the sub-skill gives nothing to the main skill. Concept sub-skills
    (`dual-wield`, `with-shield`, `with-magic`, [weapons](skills/weapons/design.md)) use it; there is no separate mechanism.
    Normalization (§4.4.5) keeps such `k` fixed at 0.
  - The peak of `gn` is ≈ 1 only when `T` is well above `T1` (`T ≥ T1 + 3w`). For lower `T` the
    peak is lower (Q-ML-10).
  - The previous curve (fall only, `g(0) = k`) is kept in
    [`old/conversion-fall-only.md`](old/conversion-fall-only.md).
- **`M_i` is not "the XP needed to reach level L".** Each sub-skill XP point is weighted by `g` at
  the level where it is earned, so the falloff is inside `M_i`.
  - `M_i` is a fixed, monotonic, **saturating** curve per sub-skill (above `T + several w` it
    barely grows).
  - It is precomputed as a table over 0…hardCap.
- Each sub-skill can contribute only a **finite** amount to the main skill, so a high main skill
  requires breadth (several sub-skills). How much breadth is set per skill with `T`: high for
  skills where a specialist should go far (weapons), lower for skills where breadth is the theme
  (crafting, magic). The sub-skill itself keeps growing for its own perks; only its contribution
  to the main skill fades.
- **Rotation exploits are closed by construction.** The main skill depends on levels, not on
  accumulated XP. Letting a sub-skill decay and retraining it returns the main skill to the same
  value and gives nothing extra (G-N2, G-N26, G-S2). `peak` is still stored per sub-skill: the
  rust formula needs it.
- **The Experience skill cap is disabled** (decided, Q-XP-8). Experience caps skills by character
  level (`fSkillCapBase = 50`, `fSkillCapMult = 1`: `cap = 50 + level`, confirmed in R-22). It is
  switched off in Experience's INI/MCM, so main skills are limited only by our own model.
  Character-level progression stays with Experience.

#### 4.4.2 Breadth vs depth

A plain sum of levels is **not** used: ten sub-skills at 10 would be worth one at 100, and are
far cheaper. With `M_i(L)` the balance is set by `T1` (rise) and `T`/`w` (fall). Example for one
Smithing-like sub-skill (placeholder curve `cost(L) = 0.25·L^1.95 + 300`, k = 1, `T1 = 25`; in
brackets: the previous fall-only curve):

| T, w | 10 sub-skills at 10 | 4 at 25 | 1 at 100 | 1 at 200 |
|---|---|---|---|---|
| sub XP spent | 30.6k | 34.2k | 96.3k | — |
| main XP, T=90, w=15 | 4.0k (30.6k) | 18.3k (34.1k) | **66.0k** (69.9k) | 84.6k (88.5k) |
| main XP, T=60, w=12 | 4.0k (30.5k) | 17.9k (33.7k) | 33.3k (37.2k) | 34.5k (38.4k) |
| main XP, T=40, w=10 | 3.9k (30.3k) | 16.7k (32.1k) | 15.5k (19.3k) | 15.5k (19.4k) |

- **Per unit of effort**, levels 0–10 of a fresh sub-skill convert at ≈ 13% of `k` (previously
  100%). Spreading over many barely-trained sub-skills no longer pays. Breadth is still
  rewarded once each sub-skill is past `T1`.
- **Depth** loses little (≈ 4k of 70k at T=90), because the cheap first levels are a small part
  of the XP to 100. The lost part is recovered by `k` in normalization (§4.4.5).
- The earlier "possible tweaks" against a breadth bias (top-N, descending weights, a count cap
  per group) are **dropped**: the rise does this job.

#### 4.4.3 Rust: decay of the main skill

The main skill must not follow each sub-skill's decay. Switching sword → mace must not lower
One-Handed: the player is still a one-handed fighter. Switching sword → greatsword should lower
One-Handed. So the main skill has its **own** decay, driven by the activity of the whole skill.

- Each main skill has a **rust** `R ≥ 0`, in levels, and an **idle meter** `U_main`. The idle
  meter works exactly like the sub-skill one (§5.5):
  - `U_main` grows with **active** game time and shrinks by `c_dec × rawXP` of any of its
    sub-skills;
  - `R` grows at rate `r_skill` while `U_main` is past the grace period (same ramp as §5.5);
  - `R` shrinks with raw XP of **any** of its sub-skills: `R −= c × rawXP` (raw, before zone
    multipliers), and never goes below 0.
- **Cross-links do not reduce `R` or `U_main`.** Only the skill's own practice counts. Otherwise
  Two-Handed practice would keep One-Handed from rusting.
- **`r_skill` per main skill**, depending on how interesting the skill is to keep up in normal
  play: crafting skills (Smithing, …) rust **slowly**, weapon and armor skills **faster**.
- **Time is active game time only** (sleeping, waiting, fast travel and jail do not count), with
  the same counter as the crafting bucket.
- Lower bound: `current_i` (decided). The main skill never falls faster than its sub-skills; in
  the end it falls to `F(effective floors)`.

| Scenario | Sub-skills | `R` | One-Handed |
|---|---|---|---|
| Sword → mace | sword decays, mace grows | mace practice keeps `R ≈ 0` | stays at the sword's `F(peak)`: does not drop |
| Sword → greatsword | sword decays | grows | drops, finally to `F(effective floors)` |
| Back to one-handed | sword recovers (×3 below potential) | XP gradually reduces `R` | recovers gradually, not from one swing |

- The weakness with the mace is still real, but it comes from the **mace sub-skill**: sub-skill
  perk requirements and continuous effects (Q-DYN-7) read the sub-skill, not the main skill.
  Main skill = the class, sub-skill = the specific weapon or material.
- An occasional poke does not stop rust: `R` shrinks in proportion to XP.

#### 4.4.4 Start, external sources, perks, writes

- **Start.** At a new game, and when the mod is added to an existing save, the current main
  skill levels are converted into **own floors** of all sub-skills of that skill; current and
  potential start at the floor. The main skill level at that moment is stored as `base`, and the
  initial sub-skill levels as `L_i^0` (formula in §4.4.1), so the main skill does not jump at
  conversion and grows only from further practice.
  - **Mapping (decided, Q-ML-9):** `floor = clamp(2 × (main − 15), 0, floorCap)` (sub-skills use
    a 0–200 scale). A skill at the default 15 gives 0, so most sub-skills start from 0 and go
    through the rise (§4.4.1); race bonuses (+5 / +10) give floors 10 / 20, still below
    `T1 = 25`. Open: which sub-skills get it (Q-ML-9).
- **External sources.** Quest rewards, `AdvanceSkill`/`IncrementSkill`, Oghma Infinium and
  similar raise **potential of all sub-skills** of that main skill (no cross-links), as in §4.1.
  Through faster "recall" practice this also raises the main skill sooner.
  - **Fortify Skill** effects: if they can be redirected to sub-skills without much effort, do
    so. Otherwise they keep working as is: they change the AV's modifiers, and we write only the
    base value, so they raise the main skill and do not affect sub-skills.
- **Perks when the main skill drops.** Perks of other mods (LoreRim, Requiem, Ordinator) stay
  and behave as their authors intended: they keep working, or switch off through their own
  skill-level conditions. We do not change them. **Our own** perks follow "dormant below the
  threshold, active again when it recovers" (with hysteresis, §5.6).
- **Legendary is disabled entirely.** A computed main skill cannot be reset. LoreRim does not
  block it: Requiem only adds warning texts and `fLegendarySkillResetValue = 5` (R-22), so our
  DLL must block the reset itself.
- **Writes with hysteresis.** The main skill is recomputed on every sub-skill change, after
  decay, and on game load. The integer main level is written to the AV (base value) only when
  `F` leaves a hysteresis band around it (width: Q-ML-6).
- **UI.** The skills menu progress bar shows the fractional part of `F`. "Skill increased" and
  "skill decreased" notifications are ours.
- **Character level.** In LoreRim character XP comes from Experience (kills, locations,
  quests), not from skill increases, so a falling main skill does not affect character level
  (`bEnableSkillXP = false`, confirmed in R-22).

#### 4.4.5 Normalization

- Per main skill, a **reference path** is defined: a set of sub-skills each trained from 0 to a
  given level (e.g. 75). This path must take the main skill from 15 to 100.
- Each group of sub-skills gets a main-XP budget: absolute (e.g. "jewelry = Smithing 15→35"), or
  a share of the rest. The `k` values are solved from the budgets, not tuned by hand.
- **Tool:** [`scripts/normalize_conversion.py`](scripts/normalize_conversion.py), with a per-skill
  TOML config (Smithing: [`scripts/smithing.toml`](scripts/smithing.toml)). It computes:
  - `k` per group;
  - the main level each sub-skill gives alone, and its lifetime contribution;
  - `g(L)` tables and a check of the reference path;
  - optionally, the crafting pace in material units and active days.

  `I(0, L)` in the script is `M(L)/k`. The XP curves are vanilla-style; the real LoreRim values
  for the main skill can be read from its AVIF record (or by the DLL at runtime).
- To add: a table of `M_i(L)` and breadth/depth scenarios (as in §4.4.2), and a simulation of the
  rust scenarios of §4.4.3 with placeholder `r`, `c`, decay parameters.

#### 4.4.6 Open questions

- **Q-ML-6.** Hysteresis width for writing the main level (e.g. ±0.5 level) and recompute
  frequency.
  - **A:**
- **Q-ML-7.** How to compute `c` (rust removed per raw XP)? Requirement: normal play with the
  skill fully cancels the growth of `R`, occasional use does not. Proposal: from a reference
  pace `P_skill`, raw XP per active game hour of normal play, `c = r / (q × P_skill)`, where
  `q < 1` is the fraction of the normal pace that is enough to hold `R` at 0 (e.g. 0.3).
  - Problem: in LoreRim, skill XP is disabled (`UseMult = 0`), so the pace cannot be observed
    in normal play.
  - Sources for `P_skill`:
    - (1) vanilla per-action use values: AVIF `UseMult`/`Offset` from the **vanilla
      `Skyrim.esm`** (can be dumped with Mutagen, no internet), plus action-specific values
      documented on UESP (R-22);
    - (2) our own debug log in playtests: the raw use points hook (Q-XP-9, R-17) logs raw XP
      per active hour even though `UseMult = 0`.

    Proposal: (1) for first numbers, (2) to calibrate.
  - **A:**
- **Q-ML-8.** Values of `r_skill` and the grace period per main skill, in levels per active
  game day (decided: crafting slow, weapons and armor faster; active time only).
  - **A:**
- **Q-ML-9.** Start conversion (§4.4.4; the formula is decided): should all sub-skills of the
  skill get the start floor, or only "basic" ones (e.g. `iron`, `steel` for Smithing; the weapon
  types for One-Handed)? Proposal: all non-concept sub-skills (concept sub-skills start at 0).
  Related: Q-OTH-1.
  - **A:**
- **Q-ML-10.** Sub-skills with a low fall `T` (Smithing placeholders: jewelry `T = 15`, iron and
  leather 25, steel 35) have their conversion peak well below `k` with `T1 = 25` (`gn` peak 0.35 /
  0.61 / 0.76). Normalization compensates group-wide `k` (jewelry has its own group), but inside
  the material group these sub-skills lose share. Options: (1) accept; (2) per-sub-skill
  `T1 ≈ T / 2` for low-`T` sub-skills; (3) raise their `T`. The script warns when the peak is
  below 0.9.
  - **A:**

## 5. Sub-skill value dynamics

### 5.1 Range and XP curve

- **Design scale 0–200.** Perks, effect formulas, `floorCap`, conversion and pacing are
  calibrated against 0–200. 200 is the "design max", the level a dedicated specialist reaches
  late in a long playthrough.
- **No practical upper limit.** Above 200 the sub-skill keeps growing along the same curve. There
  is only a technical `hardCap = 500` (config), chosen to fit float32 storage (below). With the
  vanilla-like curve, 200→500 costs ≈ 144 × (0→75), so the cap is never reached in practice.
  For reference: 0→200 ≈ 11.4 × (0→75); 200→210 ≈ 1.6 ×; 200→250 ≈ 9.8 ×; 200→300 ≈ 24 ×.
- XP needed to go from level `L` to `L+1` uses the vanilla-style curve:
  `cost(L) = improveMult × L^1.95 + improveOffset`, with `improveMult`, `improveOffset`
  configurable per sub-skill (exponent configurable globally). Levels for current, own floor,
  potential and peak are derived from their XP using the same curve.
- **Optional soft cap above 200** (config, off by default):
  `cost(L) × e^((L − 200)/softCapScale)` for `L > 200`. With `softCapScale ≈ 10` each level
  above 200 is ~10% dearer than the previous one, so the practical ceiling is ≈ 220–230.
- **Storage (float32, no double).** Each value (current, own floor, potential, peak) is stored as
  `uint16 level` + `float progress` (XP inside the level, `0 ≤ progress < cost(level)`), **not**
  as a total XP float.
  - Reason: total XP in float32 is already imprecise at 200. The total is ≈ 576k there, so the
    step (ulp) is 0.0625 XP, and small awards (falloff tails, DR, fractions of 1 XP) get rounded
    or lost. At 1000 the ulp is 4 XP.
  - With per-level progress, the largest value is `cost(L)`. At `L = 500` it is ≈ 46k, so the ulp
    is ≈ 0.004 XP. That is fine for increments of ~1 XP and above. For `cost(L) < 2¹⁶`
    (`L ≲ 600`) the ulp stays ≤ 1/128 XP. This is what sets `hardCap = 500` with a margin.
  - Precondition: XP units are chosen so that the smallest meaningful award is around 1 XP or
    larger. If the XP scale changes, re-check `hardCap`.
  - Comparisons (`current ≥ potential`, peak) are lexicographic on `(level, progress)`.
    Differences such as Δpeak are computed as "progress across level boundaries", never by
    subtracting totals.
- **Effect formulas must saturate.** Every continuous effect (Q-DYN-7/9), and any other formula
  driven by a sub-skill level, must be bounded: either saturating (e.g.
  `Bmax × (1 − e^(−L/s))`) or with its level argument clamped to 200. A linear unbounded bonus is
  not allowed. Perk thresholds are ≤ 200 by convention.
- **External potential is capped separately.** Without a cap, books, trainers and external
  increments could raise potential without limit. Then ×1.5 would apply at every level (G-N9).
  See Q-BT-4.
- **Q-DYN-8.** Main skill range: 0–100 as in vanilla, or 0–200, or uncapped (main skills are
  float AVs and can go above 100)? Constraints: some vanilla/Requiem formulas may clamp at 100 or
  misbehave above it (R-14). The vanilla skill menu is designed for 0–100. With the Smithing
  placeholders, training all sub-skills to 75 already gives main ≈ 150 ([smithing](skills/smithing/design.md)).
  - **A:**
- **Q-DYN-14.** Enable the optional soft cap above 200 by default? If yes, which
  `softCapScale`?
  - **A:**

### 5.2 Initial values

- The start conversion (§4.4.4) also covers adding the mod to an existing save. The rest of
  mid-game installation and uninstallation is still deferred (Q-OTH-1).
- **Q-DYN-1.** ~~Starting values at new game?~~ Decided: main skill levels are converted into
  own floors of the sub-skills, at a new game and when the mod is added to an existing save
  (§4.4.4). Mapping: Q-ML-9.

### 5.3 Zone multipliers (configurable)

| Zone | Multiplier |
|---|---|
| `current < effectiveFloor` (catching up) | ×3.0 |
| `effectiveFloor ≤ current < potential` (recalling) | ×1.5 |
| `current ≥ potential` (learning new) | ×1.0 |

- **Q-DYN-2.** When a single gain crosses a zone boundary, split it (the part below the boundary
  gets the higher multiplier, the rest the lower)? Proposal: yes, split.
  - **A:**

### 5.4 Floor growth

- Own floor gains XP equal to `floorRate × (XP gained by current)`, `floorRate = 0.3`
  (configurable, per sub-skill override allowed; e.g. `speech-trading` uses a higher rate).
- Own floor never decreases. Own floor is capped at `floorCap = 75` (configurable).
- Note: because the XP curve is steep, 30% of the XP gives noticeably **less** than 30% of the
  level (e.g. current 100 → own floor ≈ 55). That is expected; tune `floorRate` if needed.
- Floor XP is taken from the XP **before** the zone multiplier (decided, Q-DYN-3), so the ×3 / ×1.5
  zones speed up current but not the floor.
- **Own floor grows in every zone** (decided, Q-DYN-3), at the normal `floorRate`, including while
  `current < effectiveFloor` (catching up to a floor given by a cross-link). So a cross-link
  never slows the own floor down (it grows exactly as without the link), and never speeds it up.
  The floor stays `≤ current` anyway (`floorRate < 1`).
  - Why not "no floor growth below the effective floor": then a cross-link would slow the own
    floor down. Why not a faster growth during the catch-up (also considered): (1) the
    cross-linked sub-skill would get two bonuses at once (current is already ×3, and the
    permanent floor too), so a sword master would make the mace floor permanent with much less
    mace practice than a beginner, and the floor would no longer mean "what you trained
    yourself"; (2) one more multiplier to tune; (3) no gain in play: the own floor matters only
    when it is above the effective floor, and during the catch-up it is below it by definition.

### 5.5 Decay

// TODO: Artem: Надо обсудить decay

- **Time is active game time only** (decided, Q-DYN-5): sleeping, waiting, fast travel and
  jail do not count. The same "active game hours" counter as the crafting bucket
  ([`refs/per-target-budget.md`](refs/per-target-budget.md) §10).
- **Idle meter instead of a "last use" timer** (decided, replaces the old Q-DYN-6 proposal).
  Each sub-skill has an idle meter `U` (in active game hours):
  - `U` grows by the elapsed active time: `U += dt`;
  - `U` shrinks with practice: `U −= c_dec × rawXP` (raw XP of this sub-skill, before zone
    multipliers; XP from cross-links does not count);
  - `0 ≤ U ≤ Umax` (`Umax` = 72, so a long break needs no more practice to undo than a 3-day
    one).

  A few hits on an enemy remove only a little of `U`, so they no longer stop decay
  (grinding-analysis G-N6). Only practice at a meaningful pace keeps `U` in the grace zone.
- Decay rate from the idle meter:
  - `U < 24`: no decay (grace period);
  - `24 ≤ U < 72`: decay rate ramps linearly from 0 to `decayRate_skill`;
  - `U ≥ 72`: decay rate = `decayRate_skill`.
- `c_dec` per sub-skill (config). How to compute it: **Q-DYN-15**.
- **Jail (decided, Q-XP-6).** Serving a sentence counts as inactivity, twice as fast: on release,
  every idle meter (sub-skills and main-skill rust, §4.4.3) first skips the grace period
  (`U = max(U, 24)`), then grows by `2 × jail time` (one day in jail = two days without
  practice). This replaces the vanilla jail skill loss, which is a no-op with vanilla XP
  suppressed. In LoreRim `Less Jail.esp` caps jail at **7 days**, about one day per 1000 gold of
  bounty (`iCrimeMaxNumberofDaysinJail = 7`, `iCrimeDaysInPrisonMod = 1000`, R-22).
- Decay lowers **current** only, never below effective floor. Floor and potential are
  unaffected. Main skills do not follow this decay directly; they have their own rust (§4.4.3).
- `decayRate_skill` is per sub-skill, from config/MCM. Skills that are hard to practise often
  decay slower (e.g. smithing materials), weapons faster.
- Calibration target: an unused weapon sub-skill fully returns to its floor in about
  **14 game days**.
- Decay is applied lazily (on each use event, on UI open, and on a periodic tick), computed
  from elapsed active game time — not by a per-frame timer. Between two events `U` grows
  linearly with time, so the decay over the interval has a closed form (integral of the
  piecewise-linear ramp).
- The main skill's rust (§4.4.3) uses the same idle-meter mechanics at the level of the whole
  main skill.
- **Q-DYN-4.** Units of `decayRate`: (a) fixed levels per game day — then a skill at 200 takes
  much longer to fall to floor than a skill at 60; or (b) relative to the gap at decay start —
  `rate = (current₀ − floor) / 12 days`, so "back to floor in 14 days" holds for any level.
  Recommendation: (b), or decay in XP rather than levels.
  - **A:**
- **Q-DYN-5.** ~~Game time includes sleeping, waiting, fast travel and jail.~~ Decided: active
  game time only (see above).
- **Q-DYN-6.** ~~Does any tiny use reset the timer (G-N6)?~~ Decided: no timer; the idle meter
  `U` is reduced in proportion to raw XP (see above).
- **Q-DYN-15.** How to compute `c_dec` (idle hours removed per raw XP)? Requirement: normal
  practice of the sub-skill keeps `U` in the grace zone, occasional use does not. **Proposal:**
  from a reference practice pace `P_sub` (raw XP per active hour while the sub-skill is being
  used, from playtests or the pacing baseline §15.1): `c_dec = 1 / (q × P_sub)`, where `q < 1`
  is the fraction of the normal pace that is enough to stop `U` from growing (e.g. 0.3). Same
  approach as `c` for the main-skill rust (experiment Q-ML-7). Add to the normalization
  script once `P_sub` is known.
  - **A:**

### 5.6 Sub-skill perks

- Each sub-skill has perks with level thresholds (perk design pending, owner: user).
- **Every sub-skill has continuous effects** (decided, Q-DYN-7): one or more effects that follow
  its level (curve §5.7), or its design states explicitly that it has none. Without them the
  sub-skill system would have no gameplay effect until perks are designed.
- A perk is **active** while `current ≥ threshold` and is removed when
  `current < threshold − hysteresis` (`hysteresis` configurable, default 2 levels).
- Implementation (to confirm in plan): perks are regular `BGSPerk` records in the ESP, added to /
  removed from the player by the DLL.
- **Q-DYN-9.** How should continuous effects (Q-DYN-7) and other numeric bonuses be implemented?
  Sub-skills are not actor values, so ESP perks cannot read them directly. Options (details in
  [`refs/dynamic-skill-effects.md`](refs/dynamic-skill-effects.md)): (1) mirror sub-skills into
  spare AVs, which does not scale; (2) step entries conditioned on globals; (3) **Proposal:**
  compute the bonuses in the DLL via a hook on entry-point evaluation or damage calculation, with
  formulas from config. ESP perks stay for discrete abilities. (4) **New candidate:** expose
  sub-skills as *functional* extra values of Actor Value Generator (installed, 2.2.9.4), with
  the getter in our DLL; CK perks read them via aliased placeholder AVs in conditions and in
  `Multiply 1 + Actor Value Mult` entry points. Pure data, scales to all sub-skills, but works in
  perk entry points only if R-18 confirms it ([`refs/frameworks.md`](refs/frameworks.md) §3.5).
  Decided either way: all
  continuous formulas must saturate or clamp their level argument at 200 (§5.1).
  - **A:**
- **Sub-skill requirements check `current`** (decided, Q-DYN-10), with the hysteresis of this
  section: a perk requirement is met while `current ≥ threshold` and stops being met below
  `threshold − hysteresis`. So access follows the actual skill and can be lost by decay; floor
  (capped at 75) and `potential` (raised by books / trainers) are not used.
- **A bought perk stops working while its requirement is not met** (decided, Q-DYN-11), and
  works again when it is met (same hysteresis). Global default on; config can exempt a perk.
  Implementation: the requirement is also injected into the perk's entry points and the effects
  of its abilities ([`impl/perk-requirements.md`](impl/perk-requirements.md)).
- **Q-DYN-12.** How to show sub-skill requirements to the player? The vanilla skills menu
  probably does not render custom conditions. Options: (a) append the text to the perk
  description (at runtime from the DLL, or via ESP override); (b) show it in the PrismaUI panel;
  (c) both. Proposal: (c) if runtime description edit is feasible, otherwise (b).
  - **A:**
- **Sub-skill requirements on existing perks: implementation (decided, Q-DYN-13).** One
  `GlobalVariable` per perk requirement in our ESL (1 when met, computed by the DLL with
  hysteresis and any formula, e.g. k-of-n). On `kDataLoaded` the DLL injects conditions from a
  config table into the perk's own conditions (purchase), and, per perk, into its entry points and
  the effects of its abilities (so a bought perk stops working, Q-DYN-11), with an exception for
  NPCs. It can also replace / remove `Block >= N` items and change magnitudes. No overrides of
  LoreRim `PERK` records; structural changes (new entry points) would go to an ESL generated at
  build time. Implementation: [`impl/perk-requirements.md`](impl/perk-requirements.md);
  background: [`refs/perk-requirements.md`](refs/perk-requirements.md). If R-18 succeeds, AVG
  extra values remain an alternative to globals ([`refs/frameworks.md`](refs/frameworks.md) §3.5).

### 5.7 Effect curve (shared)

Most continuous effects (Q-DYN-7) use one curve shape: a **penalty** that goes to zero at a
checkpoint `P`, then a **bonus** that keeps growing more and more slowly towards a limit `M`
that is never reached.

```
effect(L) = −pen · (1 − L/P)                      for L < P      // penalty, linear
effect(L) =  M · x^p / (x^p + h^p),  x = L − P     for L ≥ P      // bonus, saturating
```

- `pen` — penalty at level 0; `P` — checkpoint (no penalty, no bonus); `M` — the limit;
  `h` — levels after `P` at which half of `M` is reached; `p` — shape, default **1**.
- With `p = 1` the bonus is the simple hyperbola `M·x/(x + h)`: one function on the whole
  `[P, ∞)`, bounded by `M`, so it meets the "effects must saturate" rule (§5.1).
- Config may give an anchor point instead of `h`: "bonus `b` at level `La`" gives
  `h = (La − P) · (M/b − 1)` (for `p = 1`).
- Example (the requested shape: −20% at 0, 0 at 45, ≈ +25% at 100, ≈ +35% at 200, limit +45%):
  `pen = 20, P = 45, M = 45, h = 44`:

  | L | 0 | 20 | 45 | 60 | 100 | 150 | 200 | 300 | 500 |
  |---|---|---|---|---|---|---|---|---|---|
  | effect, % | −20 | −11.1 | 0 | +11.4 | **+25.0** | +31.7 | **+35.1** | +38.4 | +41.0 |

- The bonus is steepest right after `P` (`M/h` per level, ≈ 1%/level here), while the penalty
  part is flatter (0.44%/level). If a "plateau" right after `P` is wanted, `p > 1` makes the
  start flat (`p = 1.5`, `h` re-fit to +25% at 100: +38.5% at 200). Default stays `p = 1`.
- Penalty and bonus can use different `pen` per stat while sharing `P`, `M`, `h` (weapons below).
- The input is the sub-skill's **effective level** (below), not `current` directly.

**Penalties and bonuses (decided, general rule).**

- Only **concept sub-skills** (`with-shield`, `dual-wield`, `with-magic`, …) penalize other
  sub-skills (for now). Such a context penalty never changes a game value directly. It lowers the **effective level** of the
  affected sub-skill:
  ```
  effective = current × factor,   factor = f0 + (1 − f0) · min(1, Lc / C)
  ```
  `Lc` = level of the penalizing (concept) sub-skill, `C` = its checkpoint, `f0` = factor at
  `Lc = 0`, default **0.75** (global, per-concept override). From `C` on, `factor = 1`.
- The effective level is used **only** for our continuous effects. `current`, perk thresholds,
  the main skill and XP are not affected.
- A **bonus** is always a **multiplier of a built-in game value** (weapon damage, spell power /
  magnitude, cost, price, …), applied by the DLL after vanilla and LoreRim perks.
- Non-concept sub-skills (weapons, …) act on game values directly: their **own** curve, the
  penalty part below `P` included, is such a multiplier.
- Per action at most one context penalty applies to a weapon sub-skill (a shield, a second
  weapon and a spell/staff/wand in the other hand exclude each other). If two ever apply to the
  same sub-skill, their factors multiply.

## 6. Cross-links (floor influence)

### 6.1 Rules

- A cross-link `A → B` with coefficient `c` means:
  `effectiveFloor(B) = max(ownFloor(B), max over links A→B of c × ownFloor(A))`.
- **Non-cumulative**: links do not add up; only the maximum applies.
- **Non-transitive (Proposal)**: only **own** floors propagate. If `mace` got its effective floor
  from `sword`, it does not pass it further.
  - **Q-CL-1.** Confirm non-transitivity.
    - **A:**
- Cross-links never change current or potential. When effective floor rises above current, the
  ×3 catch-up multiplier applies.

### 6.2 Link table (current decisions)

| Group | Links |
|---|---|
| Weapons | same hand class (1H↔1H, 2H↔2H): ×1/2; across 1H↔2H: ×1/4 |
| Concept sub-skills (`dual-wield`, `with-shield`, `with-magic`) | none (decided, Q-CL-3) |
| Armor | `light-armor ↔ clothing`: ×1/2; `light-armor ↔ heavy-armor`: none; between material sub-skills: none (decided, Q-CL-4) |
| Smithing | none |
| Magic | none between sub-skills; **exception:** main Alteration level raises effective floor of all magic sub-skills ([alteration](skills/alteration/design.md)) |
| Others | none |

- **Q-CL-2.** Archery: do bow/crossbow sub-skills link to each other (×1/2 like a hand class?) and
  to melee (×1/4 or none)?
  - **A:**

## 7–9. Skills (moved to `skills/`)

Per-skill design (sub-skills, XP rules, effects, skill questions) and the LoreRim perk
worksheets live in [`skills/`](skills/), one folder per skill; index:
[`skills/README.md`](skills/README.md). Section numbers are kept so that old `§7.1`-style
references can still be found here:

| Was | Section | Design | Perk worksheets |
|---|---|---|---|
| §7.1 | Weapons (One-Handed, Two-Handed, Archery) | [`skills/weapons/design.md`](skills/weapons/design.md) | [one-handed](skills/weapons/one-handed.perks.md), [two-handed](skills/weapons/two-handed.perks.md), [archery](skills/weapons/archery.perks.md) |
| §7.2 | Armor (Heavy Armor, Light Armor) | [`skills/armor/design.md`](skills/armor/design.md) | [heavy-armor](skills/armor/heavy-armor.perks.md), [light-armor](skills/armor/light-armor.perks.md) |
| §7.3 | Block (shields, parry, bash) | [`skills/block/design.md`](skills/block/design.md) | [block](skills/block/perks.md) |
| §7.4 | Smithing | [`skills/smithing/design.md`](skills/smithing/design.md) | [smithing](skills/smithing/perks.md) |
| §8.1 | Magic: general structure (all schools) | [`skills/magic/design.md`](skills/magic/design.md); [destruction](skills/destruction/design.md), [illusion](skills/illusion/design.md), [restoration](skills/restoration/design.md) | [destruction](skills/destruction/perks.md), [illusion](skills/illusion/perks.md), [restoration](skills/restoration/perks.md) |
| §8.2 | Alteration | [`skills/alteration/design.md`](skills/alteration/design.md) | [alteration](skills/alteration/perks.md) |
| §8.3 | Conjuration | [`skills/conjuration/design.md`](skills/conjuration/design.md) | [conjuration](skills/conjuration/perks.md) |
| §8.4 | Enchanting | [`skills/enchanting/design.md`](skills/enchanting/design.md) | [enchanting](skills/enchanting/perks.md) |
| §8.5 | Wands and staves | [`skills/enchanting/design.md`](skills/enchanting/design.md) | — |
| §9.1 | Speech | [`skills/speech/design.md`](skills/speech/design.md) | [speech](skills/speech/perks.md) |
| §9.2 | Alchemy | [`skills/alchemy/design.md`](skills/alchemy/design.md) | [alchemy](skills/alchemy/perks.md) |
| §9.3 | Lockpicking, Pickpocket, Sneak | [`skills/lockpicking-pickpocket-sneak/design.md`](skills/lockpicking-pickpocket-sneak/design.md) | [lockpicking](skills/lockpicking/perks.md), [pickpocket](skills/pickpocket/perks.md), [sneak](skills/sneak/perks.md) |

Perk worksheet format and background (FormKey, overrides, bringing back Ordinator perks):
[`skills/perks-guide.md`](skills/perks-guide.md).

Tracking:
first pass is done by Artem for:
* blocking

## 10. Skill books and trainers

### 10.1 Skill books

- Reading a skill book does **not** raise the main skill (vanilla increment suppressed). Instead
  it raises **potential** of **all sub-skills** of the book's main skill by `bookPotential`.
- **Q-BT-1.** Value of `bookPotential` (proposal: +10 on the 0–200 scale)? Once per book (as in
  vanilla)?
  - **A:**

### 10.2 Trainers

- Training does **not** raise the main skill. Instead each lesson:
  - raises potential of all sub-skills of the trained main skill by `trainerPotential`
    (less than `bookPotential`); potential gains **stack**;
  - grants a **training bonus**: +`trainerBonusPct` to XP of all sub-skills of that main skill for
    24 game hours. Bonus percent **stacks** across lessons, capped at **+30%**; duration does
    **not** stack.
- **Q-BT-2.** Values: `trainerPotential` (proposal +4), `trainerBonusPct` (proposal +10% per
  lesson).
  - **A:**
- **Q-BT-3.** "Duration does not stack": does each new lesson reset the timer to 24 h from now
  (refresh), or does the bonus expire 24 h after the first lesson?
  - **A:**
- **Q-BT-4.** With many trainings (5 per level in LoreRim, unused lessons carry over) and stacking potential, a rich player can buy
  potential up to 200. Cap trainer-granted potential (e.g. not above `current + X`, or not above
  the trainer's own skill level × 2)? See grinding-analysis G-N9. Decided: external potential
  (books, trainers, external increments) is capped separately, because sub-skills have no
  practical upper limit (§5.1). The question is where that cap sits (e.g. ≤ 200).
  - **A:**
- **Q-BT-5.** Training price: keep vanilla (based on main skill level)?
  - **A:**

## 11. Anti-grinding

Full list of grinding methods (vanilla and new ones introduced by this design), with status:
[`grinding-analysis.md`](grinding-analysis.md).

### 11.1 Mechanisms decided so far

- Armor: cut-off by incoming damage vs sub-skill level (from the original draft): hits with
  damage `< X` grant XP only while the sub-skill is `≤ Y`.
- Poison XP on hit, not on application.
- Potion XP scaled by strength.
- Trading XP proportional (log) to saved gold relative to a reference price.
- Trainer bonus capped at +30%.
- **Main skill computed from sub-skill levels** (§4.4). Decay-and-retrain gives nothing extra; closes G-N2, G-N26, G-S2.
- **Decay idle meter** reduced in proportion to raw XP (§5.5). Closes G-N6.
- **Conversion rise** (§4.4.1): the first `T1 = 25` levels of a sub-skill ramp its value to the main skill up from 0 (smoothstep). Many barely-trained sub-skills do not add up to a high main skill.
- **Smithing daily XP bucket** (G-S1). A leaky bucket of raw Smithing XP (all Smithing
  sub-skills share one bucket). Above the bucket, XP falls off smoothly, using the same falloff
  as the per-target budget. The bucket refills **only in active game time**: time spent
  sleeping, waiting, fast travelling or in jail does not count. Implementation reuses the
  per-target budget machinery with key (player, `craft-smithing`), see
  [`refs/per-target-budget.md`](refs/per-target-budget.md) §10.
- **Smithing diminishing repetition by item type** (G-S3). Crafting the same base item again and
  again within a time window gives decreasing XP. This also limits craft-and-break-down loops if
  a breakdown mod is installed.
- **Smithing improving** gives XP only when quality actually increases (G-S5).

### 11.2 Proposal: generic anti-grind toolkit

A small set of reusable modifiers, enabled per XP source in config:

1. **Target worth** — combat XP only from hostile targets in combat (not teammates, summons,
   paralyzed/ragdolled, or essential-and-non-hostile), multiplied by
   `f(targetLevel / subSkillLevel)` (generalizes the armor rule above).
2. **Per-target budget** — each target gives full XP only up to a budget, then XP falls off
   smoothly, and the budget refills over game time. Details in
   [`refs/per-target-budget.md`](refs/per-target-budget.md). Decided:
   - key is **(target, category)**, not (target, sub-skill) — closes G-N27 (switching weapons,
     spells or armor on one durable target to get a fresh budget for each sub-skill);
   - **combat** sources: damage budget (`k × target max health` for offense; damage received
     per attacker for defense); **non-combat** sources: XP budget;
   - budgets are saved to the **co-save**;
   - debug: Debug page / console shows `spent / B` and the falloff multiplier for the crosshair
     target (see §13).
3. **Meaningful effect** — healing/restore XP by amount actually restored; buffs give XP only
   when in combat or when a hostile is nearby (to be refined).
4. **Diminishing repetition** — repeating the same action on the same object (same item crafted,
   same item traded back and forth, same spell cast out of combat) yields decreasing XP within
   a time window.

- **Q-AG-1.** Accept the toolkit as the general approach (with per-source tuning), or prefer
  individual rules per skill?
  - **A:**
- **Q-AG-2.** For armor, replace the "damage < X while level ≤ Y" rule with the more general
  target-worth rule, or keep both?
  - **A:**

Per-target budget questions (see [`refs/per-target-budget.md`](refs/per-target-budget.md)):

- **Q-AG-3.** Category list. Proposal: `offense-melee`, `offense-ranged`, `offense-magic`,
  `offense-poison`, `offense-summon`, `defense-physical` (armor + block + parry),
  `defense-magic` (wards, resistances), `support-heal` (healing others, decided in
  [restoration](skills/restoration/design.md)), `pickpocket`, `speech`. Merge or split any? For example,
  should armor and block share one defense budget, or have separate ones?
  - **A:**
- **Q-AG-4.** Offense damage budget `kOff`. Proposal: 1.75 × target max health, per category.
  Count only effective damage, clamped to remaining health so overkill does not count?
  Proposal: yes.
  - **A:**
- **Q-AG-5.** Defense budget: what measures the attacker's "strength" `S(attacker)`? Options:
  (a) attacker max health; (b) attacker level × const; (c) the attacker's expected damage
  output over N seconds. Proposal: (a), combined with the target-worth level ratio.
  - **A:**
- **Q-AG-6.** Falloff curve after the budget is used up:
  `m = max(mMin, 1 / (1 + (x − 1)^p))` with `p = 2`. Should `mMin` be 0 (XP stops completely) or
  a small value like 0.05?
  - **A:**
- **Q-AG-7.** Refill time `refillHours`. Proposal: 24 game hours for all categories. Should some
  categories refill more slowly (e.g. pickpocket per NPC: 72 h)?
  - **A:**
- **Q-AG-8.** Non-combat XP budgets: which sources use them, and what is the base?
  Proposal: `pickpocket` (per NPC) and `speech` (per merchant, which also covers G-N13 if
  Q-SPE-4 goes that way). Lockpicking, crafting and enchanting use diminishing repetition instead,
  since they have no target.
  - **A:**
- **Q-AG-10.** Crafting bucket parameters: size (raw XP per active game day), refill period,
  falloff (`p`, `mMin`). Placeholder in `scripts/smithing.toml`: 4000 raw XP per active day.
  - **A:**
- **Q-AG-11.** "Active game time": exclude sleep, wait, fast travel and jail. Anything else?
  Carriage rides are fast travel. Should time spent in dialogue or in menus count? Game time
  mostly does not pass in menus anyway.
  - **A:**
- **Q-AG-12.** Diminishing repetition for crafting: key (base item FormID), window (proposal:
  24 active game hours), curve (proposal: `XP × 1/(1 + n/n0)` for the n-th repeat, `n0 = 5`).
  - **A:**
- **Q-AG-13.** Apply the same crafting bucket + DR to Enchanting (enchant/disenchant) and to
  alchemy crafting? Proposal: yes, with their own buckets.
  - **A:**
- **Q-AG-9.** Damage over time (poisons, burning, cloaks, summons): count it into the player's
  budget for the victim in the matching category (`offense-poison`, `offense-magic`,
  `offense-summon`)? Proposal: yes.
  - **A:**

## 12. User interface

### 12.1 Skill status panel — PrismaUI

- A custom panel built with **PrismaUI** (HTML/JS UI framework for SKSE) showing all sub-skills
  grouped by main skill, with **floor**, **current**, **potential**, decay status and active
  perks.
- Custom Skills Framework is **not** used for this (it models one value per skill and a perk tree;
  three values and ~150 sub-skills do not fit well).
- **Q-UI-1.** How is the panel opened: hotkey, from the vanilla skills menu, a lesser power?
  - **A:**
- **Decided:** PrismaUI is **added** to the modlist (it is not in LoreRim, R-3). MCM Helper 1.6.2
  and SKSE Menu Framework are already there.
- **Q-UI-2.** Fallback if PrismaUI turns out problematic in Lorerim (input focus, HUD, other
  DLLs): SKSE Menu Framework (ImGui, already installed) as plan B — ok?
  - **A:**
- **Q-UI-3.** Notifications: show a message when a sub-skill perk activates/deactivates, when a
  sub-skill starts decaying, when it reaches a new level?
  - **A:**

### 12.2 Configuration — MCM Helper + config file

**Analysis: is MCM Helper enough?** Yes, for global settings, with these notes:
- MCM Helper pages are defined in JSON; values are stored in an INI
  (`Data/MCM/Settings/<ModName>.ini`) that the DLL can read directly. Change notifications reach
  the DLL through a small Papyrus script (MCM Helper requires a quest + script extending
  `MCM_ConfigBase`) that forwards to native functions or a ModEvent.
- MCM buttons for debug actions call native (DLL-registered) Papyrus functions.
- **Not** suitable for per-sub-skill tables (~150 sub-skills × several parameters: conversion,
  decay rate, floor rate, curve). Those live in a **config file** (JSON or TOML) next to the DLL,
  with a "reload config" button in MCM.

**MCM pages (proposal):**
1. General — toggles for optional mechanics (summon upkeep, control loss), global multipliers.
2. Dynamics — zone multipliers, `floorRate`, `floorCap`, grace period, ramp length, hysteresis,
   window length.
3. Weapons / Armor / Magic / Other — coefficients of cross-links, slot weights, with-shield
   coefficients, trainer bonus cap, etc.
4. Debug — see §13.

- **Q-UI-4.** Config file format: JSON or TOML (TOML is easier to edit by hand, JSON is easier
  to share with PrismaUI)?
  - **A:**

## 13. Debugging

- A **Debug** page in MCM (decided). Proposal for its contents:
  - verbose XP log toggle (every XP event: source, raw XP, modifiers, result, main XP) to
    `Documents/My Games/Skyrim Special Edition/SKSE/<mod>.log`;
  - reload config file;
  - set/reset a sub-skill's floor/current/potential (selected via a list);
  - advance "virtual time" for decay testing;
  - dump all sub-skill states to the log;
  - toggle "vanilla XP passthrough" (to compare behavior).
- **Decided:** per-target budget inspector. For the crosshair target, show per category `spent`,
  `B`, the falloff multiplier `m` and the time until full refill. Also a console command to reset
  budgets (crosshair target or all). See
  [`refs/per-target-budget.md`](refs/per-target-budget.md) §9.
- Proposal: also console commands (via a console command hook) for the same operations.
- **Q-DBG-1.** Accept this list? Anything to add?
  - **A:**

## 14. Technical notes (preliminary, for the plan)

- Runtime: **Skyrim AE 1.6.1170, SKSE 2.2.6** (R-1). CommonLibSSE-NG (pinned commit) with
  Address Library; build on Windows with MSVC + CMake + vcpkg. Hook addresses must be verified
  for 1.6.1170.
- **Form references in config** always use `Plugin.esp|0xLocalID` (FormKey) and are resolved with
  `TESDataHandler::LookupForm(localID, plugin)` at `kDataLoaded`. EditorIDs are not used:
  they are not loaded at runtime by default, and LoreRim renames many of them.
- Integration points found in LoreRim (R-6, R-13): **Precision 2.0.4** has a pre-hit/post-hit
  API, the preferred hook for melee miss/glancing. **Parrying RPG 2.0.1** handles collision
  parries before the normal hit, so `parry` needs its own adapter. **True Directional Movement
  2.2.5** adjusts projectiles (aim support / homing), which may undo launch spread.
- Persistence: SKSE co-save serialization of all sub-skill states (current, own floor,
  potential and peak, each as `uint16 level + float progress`, see §5.1; idle meter `U`), per main skill rust `R`, idle meter `U_main`, `base` and initial
  levels `L_i^0` (§4.4), per-target and crafting budgets, the active-game-hours
  counter, trainer bonuses, crafted-potion registry. Format is versioned.
- Vanilla XP suppression: hook the player's skill-experience award function
  (`PlayerCharacter::AddSkillExperience` or equivalent); the hook also provides the vanilla XP
  amount, used as base XP for routing.
- Event sources (to verify in plan): hit events, spell cast / magic effect apply events, block
  events, crafting events, book read, training menu, barter transactions, lockpick/pickpocket
  events, shout use.
- Sub-skill perks, keywords and globals in an ESL-flagged ESP.
- Only the player is affected.
- Aim perturbation (accepted: archery Q-WPN-7; spell missiles (incl. lobbers) and beams Q-MAG-5; glancing
  melee hits Q-WPN-8): hook `RE::Projectile::Launch` (filtered
  to the player) for missiles, arrows and lobbers; a per-frame aim sway for beams (and the
  visible bow sway); a miss or glancing chance in the melee hit pipeline. Details:
  [`refs/aim-perturbation.md`](refs/aim-perturbation.md); spells:
  [`impl/spell-aim-inaccuracy.md`](impl/spell-aim-inaccuracy.md).
- Continuous numeric bonuses from sub-skills (if accepted, Q-DYN-9): hook entry-point evaluation
  (`RE::BGSEntryPoint::HandleEntryPoint`) or the damage calculation. Details:
  [`refs/dynamic-skill-effects.md`](refs/dynamic-skill-effects.md).
- Sub-skill requirements on existing perks (decided, Q-DYN-13): one `GlobalVariable` per perk
  requirement, conditions injected at `kDataLoaded` into perk conditions, entry points and
  ability effects from a config table: [`impl/perk-requirements.md`](impl/perk-requirements.md).

- Item classification (weapons, and possibly armor, materials, spells): our own `_KID.ini`
  distributes `SPO_*` class keywords (records in our ESL) from OCF, Requiem and vanilla
  keywords; the DLL checks only our keywords, looked up by FormKey. The cache is built after
  KID's `KID_KeywordDistributionDone` event (or lazily). KID INIs may use EditorIDs; that is
  resolved by KID, not by our DLL. Details: [`refs/frameworks.md`](refs/frameworks.md) §2, §4.
- Build: CommonLibSSE-NG from alandtse (branch `ng`) as a submodule, with an xmake (libxse) or
  CMake + vcpkg template; choose in the plan. Optional: CLibUtil (header-only). Details:
  [`refs/frameworks.md`](refs/frameworks.md) §5.

- **Q-TECH-1.** Is a mod-added ESP acceptable, or must the mod be DLL-only (perks created at
  runtime)? ESP is simpler and more conventional.
  - **A:**
- **Q-TECH-2.** License. The maintained CommonLibSSE-NG (alandtse) is now
  GPL-3.0-or-later with a modding exception (it was MIT). A plugin that links it statically must
  use a compatible license. Options: (a) GPL-3.0 for our mod; (b) another license allowed by the
  modding exception (read `EXCEPTIONS.md`); (c) pin an older MIT-licensed CommonLib (loses
  fixes). **Proposal:** (a), or (b) after reading the exception.
  - **A:**
- **Q-TECH-3.** Dependencies. KID and OCF are already in LoreRim and would become hard
  requirements for weapon classification; AVG would become one if Q-DYN-9 option (4) is
  chosen. Is that acceptable, or should the mod also work without them (fallback to the
  built-in TOML classifier, globals and DLL hooks)? **Proposal:** KID + OCF required; AVG
  optional, only if R-18 passes.
  - **A:**

## 15. Research and tooling

### 15.1 Pacing baseline (answer to "how many hours in vanilla?")

There is no reliable published data. Rough community estimates for vanilla (normal play, no
grinding): the main combat skill reaches 100 in roughly 50–100 h, secondary skills often never
reach 100 in a typical 100–150 h playthrough; Lorerim/Requiem progression is slower. These numbers
are too vague to tune by.

**Decided (Q-RES-1):** the first deliverable of the plan is a tiny **telemetry SKSE plugin** that only logs
every vanilla skill XP award (skill, amount, game time, real time, context) without changing
anything. Play Lorerim normally for several sessions → we get real XP/hour per skill and can set
target pacing as "× of Lorerim baseline".

### 15.2 Research tasks

Research tasks with more than a line of detail have their own file in [`research/`](research/)
(`R-<n>-<topic>.md`), linked from the table below and from the [`impl/`](impl/README.md) blocks
that need them.

Static research of the installed profile was done on 2026-11-11 (Mutagen dump; report
`research-report.md` + TSV tables, kept outside the repo in `~/tmp/research`). Worksheets
derived from it: perk worksheets in [`skills/`](skills/README.md),
[`materials.md`](refs/lorerim/materials.md), [`weapons.md`](refs/lorerim/weapons.md)
(generator: `scripts/gen_lorerim_refs.py`). Status per task:

| ID | Status / result |
|---|---|
| R-1 | Done: AE 1.6.1170, SKSE 2.2.6 |
| R-2 | Done (static): Experience 3.7.3, SSL Rewritten + Enhanced Script, Starting Skills, Simply Smart Training, Little Lessons, Typography Training, Tome Trials, Beneficial Speech Checks. Use XP disabled. |
| R-3 | Done: MCM Helper yes, PrismaUI no (will be added), SKSE Menu Framework yes |
| R-4 | Done: worksheet `refs/lorerim/weapons.md` |
| R-5 | Done: worksheet `refs/lorerim/materials.md` (set → material needs COBJ, R-15) |
| R-6 | Partial: sources listed; actual Block XP callbacks need a runtime test |
| R-7 | Partial: no unified archetype keyword family; `Traits_DruidicSpell`, Triumvirate `TVR_*` → explicit lists needed |
| R-8 | Done (static): school directions confirmed by Magic Redone keywords `Nox_KW_*`; perk trees in `skills/<skill>/perks.md` |
| R-9 | Done (static): no unified upkeep (mechanic suspended) |
| R-10 | Done: staff perks are in Enchanting (Arcane Artificery, Staff Channeling, Wand Mastery…) |
| R-11 | Done: a few MCP candidates exist, none validated; Mutagen dump works |
| R-12 | Done: Custom Skills Framework 3.1.0 + Custom Skills Menu installed |
| R-13 | Done (static): Precision 2.0.4, Parrying RPG, TDM 2.2.5 present; hook conflicts need runtime check |
| R-14 | Partial: no general clamp at 100 found, but safety above 100 not proven; runtime probes needed if main > 100 |
| R-15 | **New:** dump COBJ (recipes: workbench, ingredients, counts, required perks), for Smithing units, set → material, tempering |
| R-16 | Partial (static, R-22): no master switch. Stop new points with MCM points per level = 0 and level multiplier = 0, and zero the bank `SSL_PointsLeftAfterLvlUp`; SSL also zeroes `fSkillUsageLockPick*`. To verify on a test profile. Details: [`refs/vanilla-skill-xp.md`](refs/vanilla-skill-xp.md) §6 |
| R-17 | **New (runtime):** which hook gives raw use points with `UseMult = 0` (Q-XP-9); log one award per skill |
| R-18 | **New (runtime):** AVG 2.2.9.4 on AE 1.6.1170: define one functional extra value with a DLL getter, alias it in a test perk; check (1) `GetActorValue` perk condition, (2) `Multiply 1 + Actor Value Mult` entry point, (3) magic-effect condition. Also: which config path 2.2.9.4 reads ([`refs/frameworks.md`](refs/frameworks.md) §3) |
| R-19 | **New (runtime):** dump weapons with their keywords **after** KID distribution (on load game), to get real OCF coverage, including Animated Armoury; compare with the static `refs/lorerim/weapons.md` |
| R-20 | **New:** exact OCF version in LoreRim, and whether a keyword patch (e.g. "Keyword Patch Collection") covers Animated Armoury |
| R-21 | **New:** read CommonLibSSE-NG `EXCEPTIONS.md` (Q-TECH-2) |
| R-22 | Done (static): vanilla AVIF values confirmed (Smithing `UseMult`: 160 in Skyrim.esm, 1 in Update.esm), LoreRim GMST overrides, Experience / SSL / jail / Legendary settings, perk override chains: [`refs/vanilla-skill-xp.md`](refs/vanilla-skill-xp.md). Left: 6 engine-default GMSTs to read in game (§5 there); the `P_skill` / `P_sub` estimate itself (Q-ML-7, Q-DYN-15) |
| R-23 | **New (runtime):** block stamina cost: formula, whether the GMST can be swapped per hit, where the drain happens, threads, Parrying RPG / Dual Wield Parrying / MCO Block Recovery. Details: [`research/R-23-block-stamina.md`](research/R-23-block-stamina.md) |
| R-24 | **New:** does a LoreRim mod already slow time on block / parry (For Honor in Skyrim `MA_SlowTime`, Parrying RPG…), before restoring Quick Reflexes. Details: [`research/R-24-quick-reflexes-slowtime.md`](research/R-24-quick-reflexes-slowtime.md) |
| R-25 | **New:** injecting conditions / values into existing perks from the DLL: CommonLib fields, gating of entry points and abilities, perks menu display, NPC exception, old saves. Details: [`research/R-25-perk-condition-injection.md`](research/R-25-perk-condition-injection.md) |
| R-26 | **New:** spell aim: launch noise through `Projectile::Launch` (missile, lobber), aim sway for beams, TDM / SmoothCam interaction, how Requiem sways a bow without stamina. Details: [`research/R-26-spell-aim.md`](research/R-26-spell-aim.md) |

Original task list:

| ID | Task | How |
|---|---|---|
| R-1 | Exact Skyrim runtime and SKSE version used by Lorerim 5.0+ | Lorerim docs / installed instance |
| R-2 | Mods in Lorerim that change skill XP, skill books, training, leveling | modlist manifest, plugin records, SKSE plugin list |
| R-3 | Whether PrismaUI and MCM Helper are in Lorerim (or can be added) | modlist |
| R-4 | Weapon types and their keywords (incl. modded types, light/heavy bows) | dump WEAP + KYWD |
| R-5 | Armor materials and keywords (incl. AE and modded) | dump ARMO + KYWD |
| R-6 | Block XP sources in Requiem/combat mods | plugins + SKSE plugins |
| R-7 | Spell archetype keywords (druidic, shamanic, …) actually used | dump SPEL/MGEF + KYWD |
| R-8 | School sub-directions: perk trees per school and keywords they condition on | dump PERK conditions |
| R-9 | Existing summon upkeep / thrall mechanics | modlist + plugins |
| R-10 | Staff/wand perks and their tree | dump PERK |
| R-11 | Skills / MCP servers for Creation Kit, SKSE, CommonLibSSE-NG | web research |
| R-12 | Custom skill frameworks (Custom Skills Framework and alternatives), status and popularity | web research |
| R-13 | Is Precision (physical melee collision) in Lorerim? Other SKSE mods hooking `Projectile::Launch` or melee hit detection | SKSE plugin list |
| R-14 | Vanilla/Requiem/Lorerim formulas that clamp skills at 100 or misbehave above it (prices, pickpocket, sneak, armor, etc.) | game settings + perk entry points + web research |

### 15.3 What is needed to do research R-2…R-10 on Linux

Researching Lorerim content does not require the game to run, only its **data files**:

- **Minimum:** the Wabbajack modlist file (`.wabbajack` — a zip with the full manifest: every mod,
  version and file). It can be downloaded and parsed on Linux. Gives R-2, R-3 and partially R-9.
- **For record-level research:** the load order (`plugins.txt`/`loadorder.txt` from the MO2
  profile) plus all `.esm/.esp/.esl` files of the installed list (plugins only, BSAs not needed;
  a few hundred MB). Copy them from the Windows machine.
- **Tools on Linux:**
  - **Mutagen** (.NET, runs natively on Linux): a small C# program reads the whole load order and
    dumps keywords, weapons, armors, spells, magic effects and perks with their conditions to
    JSON/CSV. This is the best option for an agent: deterministic, scriptable, text output.
  - **Spriggit** (by the Mutagen author): converts plugins to YAML text — handy for reading
    individual plugins.
  - **xEdit** works under Wine and has scripting, but is less convenient for automation.
- Alternatively, run the same Mutagen dump on the Windows machine and commit the JSON output into
  this repo; then all analysis can be done here.

## 16. Sub-skill catalog (preliminary)

| Main skill | Sub-skills | Approx. count |
|---|---|---|
| One-Handed | dagger, sword, war-axe, mace, (+modded), dual-wield, with-shield, with-magic | 7+ |
| Two-Handed | greatsword, battleaxe, warhammer, (+modded) | 3+ |
| Archery | light-bow, heavy-bow, light-crossbow, heavy-crossbow | 4 |
| Block | heavy-shield, light-shield, parry, bash | 4 |
| Heavy Armor | heavy-armor, materials, heavy-other | ~10 |
| Light Armor | light-armor, clothing, materials, light-other | ~10 |
| Smithing | materials, 6–7 categories incl. jewelry | ~20 |
| Destruction, Restoration, Alteration, Illusion, Conjuration | forms (~10–15, shared), archetypes (R-7), sub-directions (~4–6 per school), conjuration specials | ~50 |
| Enchanting | enchanting-weapons, enchanting-armor, enchanting-other, wands-staffs | 4 |
| Speech | intimidation, persuasion, trading, thuum | 4 |
| Alchemy | poisons, potions | 2 |
| Lockpicking, Pickpocket, Sneak | one each | 3 |
| **Total** | | **~120–130** |

- **Q-CAT-1.** Forms are shared across schools, but each sub-skill has exactly one parent main
  skill (§3). Either a form sub-skill exists per school (`destruction/missile`,
  `restoration/missile`, … — many more sub-skills) or form sub-skills have **no fixed parent**
  and count towards the school of the spell being cast. The main level is computed from
  sub-skill levels (§4.4.1), so a shared form would need a per-school part of its level (e.g. its
  XP tracked per school). Which one? (Same question for archetypes.)
  - **A:**

## 17. Other important questions

- **Q-OTH-1.** Mid-game installation and uninstallation (deferred; to be revisited near the end).
  - **A:**
- **Q-OTH-2.** Followers/NPCs are unaffected (player only) — confirm.
  - **A:**
- **Q-OTH-3.** Lorerim updates: should the mod pin a specific Lorerim version, or should the data
  (keyword → sub-skill mapping) live in the config file so it can be updated without
  recompiling? Proposal: all mappings in config.
  - **A:**
- **Q-OTH-4.** Main-skill perks (Lorerim trees) still require main skill levels and perk points,
  unchanged. Main skills now rust when not practised (§4.4.3), so "no omnipotence" also works
  through the main skill. Our own perks go dormant below their threshold; other mods' perks are
  not changed (§4.4.4). Is anything else needed (e.g. main-skill perk conditions on sub-skill
  current)?
  - **A:**
- **Q-OTH-5.** Mod name?
  - **A:**
