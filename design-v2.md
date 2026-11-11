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
> LoreRim worksheets: [`refs/lorerim/`](refs/lorerim/): [`perks.md`](refs/lorerim/perks.md),
> [`materials.md`](refs/lorerim/materials.md), [`weapons.md`](refs/lorerim/weapons.md).
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
    The full list is in [`refs/lorerim/perks.md`](refs/lorerim/perks.md).
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
- **External increments become potential.** Any non-mod increase of a main skill (quest
  rewards, `AdvanceSkill`/`IncrementSkill` from scripts, Oghma Infinium, etc.) is intercepted
  and converted into a **potential** increase for **all sub-skills of that main skill**.
  Example: "Smithing +5" → potential of every Smithing sub-skill increases. Books and trainers
  follow the same idea with their own rules (§10).

### 4.2 XP pipeline (per gameplay event)

```
gameplay event
  → determine affected sub-skills and raw XP for each           (per-skill rules, §7–§9)
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

- **Q-XP-1.** One event often feeds several sub-skills at once (sword hit with a shield →
  `sword` + `with-shield`; ebony cuirass hit → `heavy-armor` + `ebony-armor`; fireball →
  `missile` + archetype + `destruction/fire`). How much main XP does such an event produce?
  Options: (a) every sub-skill converts its own XP and they add up; (b) the event produces
  main XP once, computed from the raw event XP, and sub-skills only track proficiency;
  (c) each sub-skill has a weight and weights per event sum to 1.
  **Decided for Smithing: (a)** (§7.4). This is safe because conversion is normalized per main
  skill against a reference path (§4.4), so summing does not make the skill grow faster.
  Proposal: (a) + per-skill normalization for all skills.
  - **A:** (a) + per-skill normalization for all skills.
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
- **Q-XP-6.** Jail: vanilla removes skill progress while serving time. With vanilla XP
  suppressed this becomes a no-op. Should jail do something in our model (e.g. start decay
  immediately for all sub-skills), or nothing?
  - **A:**: Jai starts decay immediately and counts as x2 time (one day in jail
    is two days of inactivity)
  - Note (R-22): in LoreRim `Less Jail.esp` caps jail at **7 days**, about one day per 1000 gold
    of bounty (`iCrimeMaxNumberofDaysinJail = 7`, `iCrimeDaysInPrisonMod = 1000`).
- **Q-XP-7.** Target pacing (how many hours of normal play from 15 to 100 in a main skill,
  how many hours from 0 to 100/200 in an actively used sub-skill). See §15.1 for what we know
  about vanilla and the proposal to measure it.
  - **A:**

- **Q-XP-8.** **Experience mod skill cap.** Experience caps skills by character level
  (`fSkillCapBase = 50`, `fSkillCapMult = 1`, `bUseRacialCaps = false`: `cap = 50 + level`, 51 at
  level 1, 100 at level 50; confirmed in R-22). Our main skills would hit that cap. Options: (a) disable the Experience skill cap
  (INI/MCM), so main skills are limited only by our own model; (b) keep it as an additional
  "level gate". With the level model (§4.4) nothing needs to be stored: the computed value is
  clamped and rises at once when the cap rises; the UI must explain the cap; (c) replace it with our own gate (e.g. via perk requirements).
  Recommendation: (a). Character-level progression stays with Experience either way.
  - **A:** (a), disable cap
- **Q-XP-9.** **Base XP when `UseMult = 0`.** The design uses the engine's per-action XP amount as
  base XP. In LoreRim the AVIF multiplier is 0, so after the multiplier the amount is 0. Proposal:
  hook the call that receives the **raw use points** before the AVIF multiplier
  (`PlayerCharacter::UseSkill`/`PlayerSkills::UseSkill` *(verify which one applies the
  multiplier)*). Our own per-skill base multipliers (config) then replace `UseMult`. If raw
  points are not reachable, compute base XP per event ourselves (§7–§9 rules). Runtime test:
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
- The **Experience skill cap** (Q-XP-8) is applied as a clamp on the computed value. When the
  character levels up, the main skill rises at once to what has already been earned.

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
  conversion and grows only from further practice. Mapping: Q-ML-9.
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
- **Q-ML-9.** Start conversion (§4.4.4): which own floor does a main level give? Proposal:
  `floor = clamp(2 × (main − 15), 0, floorCap)` for every sub-skill of that skill (sub-skills use
  a 0–200 scale). Should all sub-skills get the same floor, or only "basic" ones (e.g. `iron`,
  `steel` for Smithing; the weapon types for One-Handed)? Related: Q-DYN-1, Q-OTH-1.
  - **A:** formula accepted: `floor = clamp(2 × (main − 15), 0, floorCap)`. A skill at the
    default 15 gives 0, so most sub-skills start from 0 and go through the rise (§4.4.1); race
    bonuses (+5 / +10) give floors 10 / 20, still below `T1 = 25`. Open: same floor for all
    sub-skills or only "basic" ones.
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
  placeholders, training all sub-skills to 75 already gives main ≈ 150 (§7.4).
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
- **Q-DYN-3.** Should floor gain use the XP before or after the zone multiplier? And should the
  floor grow while current is below floor (catching up)? Proposal: floor uses XP before zone
  multiplier and grows only while `current ≥ effectiveFloor`.
  - **A:** 1. Before zone multiplier 2. If floor only grows when current >=
    effectiveFloor, then we have a problem that having effectiveFloor from
    other skill will slow down growing own floor. Perhaps we should instead
    accelerate growing ownFloor while ownFloor <= current <= effectiveFloorL
    What are the downsides?

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
- A perk is **active** while `current ≥ threshold` and is removed when
  `current < threshold − hysteresis` (`hysteresis` configurable, default 2 levels).
- Implementation (to confirm in plan): perks are regular `BGSPerk` records in the ESP, added to /
  removed from the player by the DLL.
- **Q-DYN-7.** Besides threshold perks, should current level have a small continuous effect
  (e.g. +0.1% weapon damage per level of the matching weapon sub-skill)? Without it, the
  sub-skill system has no gameplay effect until perks are designed.
  - **A:** Yes, it should. Every sub-skill must have designated continuous
    effect, or multiple, or explicitly stated in design document that there are
    no continuous effects.
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
- **Q-DYN-10.** Sub-skill requirements on purchasable perks (e.g. perk P requires
  `smithing-dwarven ≥ 70`): which value is checked? (a) `current` — fluctuates with decay, the
  player may lose access to a perk they had "earned"; (b) own / effective floor — stable, but
  capped at `floorCap` 75, so thresholds above 75 are impossible, and effective floor can be
  raised via cross-links; (c) `potential` — never decays, reflects the peak reached, but is also
  raised by books/trainers (Q-BT-4). Proposal: (c), or allow choosing per requirement in config.
  - **A:**
- **Q-DYN-11.** Perk conditions gate only the purchase. If the sub-skill later drops below the
  requirement, should the bought perk: (a) keep working; (b) stop working until the sub-skill
  recovers (with hysteresis), which means also injecting the condition into the perk's entry
  points/effects? Per perk or global setting?
  - **A:**
- **Q-DYN-12.** How to show sub-skill requirements to the player? The vanilla skills menu
  probably does not render custom conditions. Options: (a) append the text to the perk
  description (at runtime from the DLL, or via ESP override); (b) show it in the PrismaUI panel;
  (c) both. Proposal: (c) if runtime description edit is feasible, otherwise (b).
  - **A:**
- **Q-DYN-13.** Implementation. **Proposal:** each referenced sub-skill value is mirrored into a
  `GlobalVariable` in our ESL. On `kDataLoaded` the DLL appends `GetGlobalValue … >= N`
  conditions to existing perks from a config table, so Lorerim `PERK` records are not
  overridden. Alternative: ESP overrides of Lorerim perks (conflicts, breaks on Lorerim
  updates). Details in [`refs/perk-requirements.md`](refs/perk-requirements.md). Alternative
  if R-18 succeeds: AVG extra values instead of globals, read with `GetActorValue` conditions
  ([`refs/frameworks.md`](refs/frameworks.md) §3.5).
  - **A:**

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
| Armor | `light-armor ↔ clothing`: ×1/2; `light-armor ↔ heavy-armor`: none |
| Smithing | none |
| Magic | none between sub-skills; **exception:** main Alteration level raises effective floor of all magic sub-skills (§8.2) |
| Others | none |

- **Q-CL-2.** Archery: do bow/crossbow sub-skills link to each other (×1/2 like a hand class?) and
  to melee (×1/4 or none)?
  - **A:**
- **Q-CL-3.** Do concept sub-skills (`dual-wield`, `with-shield`, `with-magic`) participate in
  cross-links? Proposal: no.
  - **A:** No, they do not
- **Q-CL-4.** Armor material sub-skills: cross-links between materials (e.g. all heavy materials
  ×1/2 to each other) or none?
  - **A:** No crosslinks between materials

## 7. Warrior skills

### 7.1 Weapons (One-Handed, Two-Handed, Archery)

**Sub-skills:**

| Parent | Sub-skills |
|---|---|
| One-Handed | `dagger`, `sword`, `war-axe`, `mace`, + modded 1H types found in Lorerim (research R-4) |
| Two-Handed | `greatsword`, `battleaxe`, `warhammer`, + modded 2H types (spears, halberds, quarterstaffs…?) |
| Archery | `light-bow`, `heavy-bow`, `light-crossbow`, `heavy-crossbow` |
| One-Handed (concept) | `dual-wield`, `with-shield`, `with-magic` |

- XP base amount per hit is the vanilla (Lorerim/Requiem) amount, captured from the suppressed
  vanilla award (see §14 technical notes), and then routed to sub-skills.
- Weapon type is determined by `WeapType*` keywords (plus mod keywords found by R-4).
- Staves and wands are cast, not used in melee, so they are not weapons here (see §8.5).

**Window rules** (window length `windowSec = 10`, configurable):

- An event qualifies if a **complementary** event happened within the last `windowSec` seconds
  (looking back only). No one-to-one pairing: every qualifying event grants XP.
  Example for `with-magic`: hit–spell–hit–hit–spell within 10 s → 4 awards (the first hit has
  nothing before it).
- `with-magic`: complementary pair = (weapon hit, spell cast). Weapon hit qualifies if a spell
  was cast in the window; spell cast qualifies if there was a weapon hit in the window.
- `dual-wield`: complementary pair = (main-hand attack, off-hand attack).
- `with-shield`: weapon hit with a shield equipped grants `with-shield` XP × **1.0** if a block
  happened in the window, otherwise × **0.5** (coefficients configurable).

**Questions:**

- **Q-WPN-1.** Modded weapon types in Lorerim: which ones exist and where do they go
  (own sub-skill or merged into the closest vanilla one)? Research R-4 found quarterstaff,
  halberd, pike, spear, javelin, katana, rapier, claw, whip, scythe, shortsword and OCF variants.
  **Fill in the worksheet [`refs/lorerim/weapons.md`](refs/lorerim/weapons.md)**, which also
  describes the classifier and config-defined (dynamic) sub-skills.
  - **A:**
- **Q-WPN-2.** How is light vs heavy bow/crossbow determined? Research R-4: by the Requiem
  keywords `REQ_WeaponType_BowLight/BowHeavy` (`9F9915`/`9F9914:Requiem.esp`) and
  `REQ_WeaponType_CrossbowLight/CrossbowHeavy` (`9F7179`/`899DBE:Requiem.esp`), never by weight.
  Proposal: use these. Fallback for unmarked bows: light.
  - **A:**
- **Q-WPN-10.** Classifier and sub-skill registry (see
  [`refs/lorerim/weapons.md`](refs/lorerim/weapons.md)): **Proposal:** sub-skills and weapon
  rules are defined in drop-in config files. Rules are ordered by FormKey and the first match wins,
  with the animation type as fallback. The co-save is keyed by sub-skill string id. Automatic
  creation of sub-skills from unknown keywords is opt-in and off by default.
  - **A:**
- **Q-WPN-3.** Unarmed combat (fists, claws for Khajiit/werewolf/vampire lord): add a sub-skill
  or ignore? Which main skill?
  - **A:**
- **Q-WPN-4.** Window events: does "weapon hit" mean a landed hit, or any swing? Does "spell
  cast" mean any cast, a successful one, or only a hostile spell hitting an enemy? Does the
  spell cast in `with-magic` grant XP equal to the spell XP or to a weapon-equivalent amount?
  - **A:**
- **Q-WPN-5.** Is `with-shield` symmetric too (a block grants `with-shield` XP if a weapon hit was
  in the window)?
  - **A:**
- **Q-WPN-7.** Should low archery sub-skills make arrows/bolts less accurate (direction noise
  at launch, `σ` decreasing with level), optionally with a visible aim sway while drawing? See
  [`refs/aim-perturbation.md`](refs/aim-perturbation.md) §3.
  - **A:**
- **Q-WPN-8.** Should low melee sub-skills cause misses? Melee hits are a cone check, not physical
  collision, so there is no aim to perturb. Options: (a) miss chance; (b) glancing hit (reduced
  damage, no stagger); (c) narrower hit cone or reach for the player (needs reverse engineering).
  Proposal: (b), maybe (a) at very low levels. See
  [`refs/aim-perturbation.md`](refs/aim-perturbation.md) §4.
  - **A:**
- **Q-WPN-6.** Bound weapons (Bound Sword etc.): give XP to both the weapon sub-skill and
  `conjure-bound`, or only `conjure-bound`?
  - **A:**
- **Q-WPN-9.** Perk changes (One-Handed, Two-Handed, Archery): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`refs/lorerim/perks.md`](refs/lorerim/perks.md)** (all tree perks of
  this skill, with handles); see also [`refs/perk-requirements.md`](refs/perk-requirements.md) §5. Answer separately for each skill: One-Handed, Two-Handed, Archery.
  - **A:**

### 7.2 Armor (Heavy Armor, Light Armor)

**Sub-skills:**

| Parent | Sub-skills |
|---|---|
| Heavy Armor | `heavy-armor`, one per heavy material (`iron-armor`, `steel-armor`, `dwarven-armor`, `orcish-armor`, `ebony-armor`, `daedric-armor`, `dragonplate-armor`, + Anniversary Edition and modded materials, + `heavy-other`) |
| Light Armor | `light-armor`, `clothing`, one per light material (`leather-armor`, `hide-armor`, `elven-armor`, `glass-armor`, `scaled-armor`, `dragonscale-armor`, + AE and modded, + `light-other`) |

- Clothing XP goes into the Light Armor main skill (as Lorerim does).
- Material is determined by `ArmorMaterial*` keywords; for modded armor, research which keywords
  are used (R-5); fallback to `heavy-other`/`light-other`.
- Base XP per hit taken is the vanilla (Lorerim) amount, subject to anti-grind rules (§11).

**Slot weights** (configurable):

| Slot | Weight |
|---|---|
| Body | 0.70 |
| Head | 0.20 |
| Feet | 0.05 |
| Hands | 0.05 |

Rings, amulets and shields are not counted (shields belong to Block).

**Mixed sets (explicit rule):** for each hit taken, for every slot that has an item equipped:
1. Determine the item's armor type (heavy / light / clothing) and material.
2. Grant `baseXP × slotWeight` to the type sub-skill (`heavy-armor`, `light-armor` or `clothing`).
3. Grant `baseXP × slotWeight` to the material sub-skill.

Example: heavy ebony cuirass + light leather helmet + nothing else, base XP 10 →
`heavy-armor` +7, `ebony-armor` +7, `light-armor` +2, `leather-armor` +2. Empty slots grant nothing.

**Questions:**

- **Q-ARM-1.** Empty slots: is their share simply lost (current rule), or redistributed among
  occupied slots?
  - **A:** empty slot is treated as clothing
- **Q-ARM-2.** Main XP for armor depends on Q-XP-1: with the rule above, a single hit gives XP to
  two sub-skills per slot. Which of them feeds the main skill?
  - **A:**
- **Q-ARM-3.** Anniversary Edition and modded materials: own sub-skill per material, or group
  them (e.g. "Nordic", "Stalhrim", "Chitin" own; minor modded sets → `*-other`)?
  Depends on research R-5.
  - **A:**
- **Q-ARM-4.** Perk changes (Heavy Armor, Light Armor): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`refs/lorerim/perks.md`](refs/lorerim/perks.md)** (all tree perks of
  this skill, with handles); see also [`refs/perk-requirements.md`](refs/perk-requirements.md) §5. Answer separately for each skill: Heavy Armor, Light Armor.
  - **A:**

### 7.3 Block (shields, parry, bash)

**Sub-skills (parent: Block):** `heavy-shield`, `light-shield`, `parry` (blocking with a weapon),
`bash` (bashing with a shield or a weapon).

- XP rules follow vanilla (with Requiem modifications), subject to anti-grind rules (§11).
- Shield type is determined by armor type of the shield (heavy/light).
- **Q-BLK-1.** A shield bash: XP to `bash` only, or to `bash` and the shield sub-skill?
  - **A:** both. to 'bash' subskill fully, to 'heavy-shield'/'light-shield' with
    coefficient, say 0.5
- **Q-BLK-2.** Is there any vanilla/Requiem Block XP source other than blocking hits and bashing
  (e.g. blocking spells/arrows, timed blocks from combat mods in Lorerim)? Research R-6.
  - **A:**
- **Q-BLK-3.** Perk changes (Block): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`refs/lorerim/perks.md`](refs/lorerim/perks.md)** (all tree perks of
  this skill, with handles); see also [`refs/perk-requirements.md`](refs/perk-requirements.md) §5.
  - **A:**

### 7.4 Smithing

**Sub-skills (parent: Smithing):**

- **material** sub-skills: one per material (table below);
- **category** sub-skills: `one-handed-smithing`, `two-handed-smithing`, `light-armor-smithing`,
  `heavy-armor-smithing`, `clothing-smithing`, `bow-smithing`, `crossbow-smithing`. Arrows go to
  `bow-smithing`, bolts to `crossbow-smithing`;
- `jewelry` (rings, necklaces, circlets).

**XP sources:** only **crafting** and **improving (tempering)**. Smelting and tanning give nothing.

- Material of an item: item keywords (`WeapMaterial*` / `ArmorMaterial*`), falling back to the
  main ingredient of the recipe, with `other-smithing` as the last resort.
- **Crafting** an item grants raw XP:
  - material sub-skill: `XP_mat = xpPerUnitMat × units`;
  - category sub-skill: `XP_cat = xpPerUnitCat × tier(material) × units`;
  - `units` = number of units of the main material in the recipe (COBJ). Large items cost more
    material and give more XP; expensive materials teach the category more. XP per unit is the
    same for every item, so there is no "best item to spam". An optional per-item override
    table is allowed.
- **Improving** grants XP only when the item's quality level actually increases (G-S5). It uses
  the same formulas, applied to the units of the temper recipe, × `temperFactor` (config).
- **Tier is our concept, not a game property.** The engine has no material tier. Tiers live in the
  config table below. The default tier can be derived from the perk the recipe requires (Steel,
  Elven, Advanced, Glass, Ebony, Daedric, Dragon Smithing…).
- No cross-links within Smithing.

**Conversion into main Smithing** (model §4.4):

- Each sub-skill converts its own XP (contributions add up; Q-XP-1 decided as (a) for Smithing).
- `k`: one value for all materials, another for all categories, a separate one for `jewelry`.
- `T` (where conversion fades): the same for all categories. For materials it grows with
  difficulty: iron is learned quickly and stops teaching Smithing early, glass teaches longer.
  `jewelry` fades very early.
- **Reference path:** `jewelry` + 3 categories (equal to each other) + `steel` + `orcish` +
  `glass`, each 0→75 ⇒ Smithing 15→100. `jewelry` 0→75 alone ⇒ Smithing 15→35.
- With the placeholder values in [`scripts/smithing.toml`](scripts/smithing.toml) (vanilla-like
  curve): N(15→100) ≈ 91.6k main XP; `jewelry` ≈ 9.5% of N; the rest is split 50/50 between
  categories and materials (Q-SMI-6). Reaching 200 in every path sub-skill gives Smithing ≈ 117.
  Training all sub-skills to 75 gives ≈ 150 (the main-skill cap is Q-DYN-8).

**Anti-grind (decided, details in §11):** daily Smithing XP bucket that refills only in active
game time (G-S1); diminishing repetition by item type (G-S3); improving gives XP only when
quality rises (G-S5).

**Material table** (Q-SMI-5): see the worksheet
[`refs/lorerim/materials.md`](refs/lorerim/materials.md), with 111 material / armor-set keywords
actually used in LoreRim, record counts, examples and proposals for sub-skill and tier.
Notes:
- Requiem armor keywords are **set families** (`REQ_ArmorSet_Guard`, `…_Vigilant`, …), not
  physical materials. For Smithing, the material of a set comes from its recipe ingredients.
- **Recipes (COBJ) are not in the research dump yet** (R-15). They are needed for: the set →
  material mapping, `units` per recipe (§7.4 XP formulas) and temper recipes.
- Placeholder tiers and `T` values are in `scripts/smithing.toml` (iron 1 … dragon 10).

- **Q-SMI-5.** Fill in the materials worksheet
  ([`refs/lorerim/materials.md`](refs/lorerim/materials.md)): Smithing sub-skill and tier
  (category XP weight) per keyword, `T` per sub-skill, and merges or splits (hide/leather,
  steel/steel plate, dragonbone/dragonscale, CC and modded materials).
  - **A:**
- **Q-SMI-6.** How is the budget left after `jewelry` split between categories and materials?
  The placeholder is 50/50, which means one category 0→75 gives about the same as one mid
  material.
  - **A:**
- **Q-SMI-7.** Is `jewelry` a category like the others, so a silver ring feeds `jewelry` +
  `silver`, or a standalone sub-skill with no material XP? Gems: no XP, or part of `units`?
  - **A:** jewelry is a special category and creating jewelry does not feed
    materials. E.g. creating silver amulet does not feed smithing-silver at all.
    Gems only feed jewelry itself, no material xp
- **Q-SMI-8.** What counts as the "main material" in recipes with several ingredients (glass =
  malachite + moonstone; steel plate = steel + corundum; leather strips everywhere)? Proposal:
  sum the units of all ingredients that belong to the item's material (ingots or ore of that
  material). Ignore generic ones (leather strips, firewood). Fallback: the most numerous
  ingredient.
  - **A:** Clarification required. Creating glass from malachite and moonstone
    is smellting and it does not give xp. Recipe for armor should only have
    'glass ingot' as ingridient, should it not ?
- **Q-SMI-9.** `temperFactor` and the temper XP source. Proposal: same formulas applied to the
  temper recipe units, `temperFactor = 1`. Temper recipes are already cheap (one unit per step),
  and they give XP only when quality rises.
  - **A:**
- **Q-SMI-10.** Arrows and bolts: one recipe produces a bundle. Proposal: count `units` of the
  recipe as usual (one recipe = one craft), with an optional per-category multiplier.
  - **A:** Agree -count 'units' of the recipe. Batch of arrows as one craft is considered one
    item
- **Q-SMI-4.** Perk changes (Smithing): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`refs/lorerim/perks.md`](refs/lorerim/perks.md)** (all tree perks of
  this skill, with handles); see also [`refs/perk-requirements.md`](refs/perk-requirements.md) §5.
  - **A:**

## 8. Magic skills

### 8.1 General structure

For every spell cast that grants XP, the spell feeds three kinds of sub-skills:

1. **Form** — gameplay shape of the spell (combination of casting type, delivery and projectile
   type). The form sub-skills are cross-school; their XP goes to the main skill of the spell's
   school.
2. **Archetype** — grouping by mod keywords (`druidic`, `shamanic`, …); spells without an
   archetype use `standard`. Actual list from Lorerim research (R-7).
3. **School sub-direction** — e.g. `destruction/fire`, `destruction/frost`, `destruction/shock`,
   `restoration/healing`, `restoration/sun`, `restoration/poison`, … from keywords and/or
   Lorerim perk tree structure (R-8). Fallback: `<school>/other`.

No cross-links between magic sub-skills, except the Alteration rule (§8.2).

**Forms.** From the original draft: `missile`, `beam`, `flame`, `rune`, `wall`, `cloak`,
`direct` (was "target-actor"; name is a proposal). **Proposal** — forms I think are missing:

| Proposed form | Examples |
|---|---|
| `self` (self buffs) | Oakflesh, Muffle, armor/resist spells |
| `touch` (contact) | touch-delivered spells |
| `ward` | Restoration wards (concentration on self) |
| `heal-self` vs `heal-other` | Healing vs Heal Other (or merge into `self`/`missile`) |
| `area` (explosion) | Fireball (missile with explosion) — separate form or still `missile`? |
| `cone` | cone projectiles |
| `summon` / `target-location` | summons, spells placed at a point |
| `lobber` | arcing projectiles |
| `dual-cast` (concept, like `dual-wield`) | any dual-cast spell |
| `scroll` | scrolls |

- **Q-MAG-1.** Which of the proposed forms to add, rename or merge? Is `direct` a good name?
  - **A:**
- **Q-MAG-2.** Spells with several effects from different sub-directions (e.g. fire + frost):
  proposal — classify by the costliest (primary) effect only.
  - **A:**
- **Q-MAG-3.** XP amount per cast: vanilla (base magicka cost, granted on vanilla conditions —
  e.g. Destruction on hit) for all three sub-skill kinds? Relation to Q-XP-1.
  - **A:**
- **Q-MAG-5.** Should low magic sub-skills make aimed spells inaccurate? Missiles and lobbers
  are easy (noise at launch). Beams and flames re-aim every frame and need per-frame hooks
  (harder). Which sub-skill drives the accuracy: the form (`missile`, `beam`, `flame`…), the
  school sub-direction, or both? Proposal: the form, missiles first, beams and flames later. See
  [`refs/aim-perturbation.md`](refs/aim-perturbation.md) §3.
  - **A:** Only masterf of form drives inaccuracy. `Flames` are not subject to
    inaccuracy, as are 'direct' and any short-distance (e.g. touch or rune).
    Essentially, only beam, missiles and lobbers are subjects to inaccuracy.
    For beam I suggest same mechanism that is used for archery when stamina is
    low and effect should be cummulative and increasing with time of holding
    the ray.
- **Q-MAG-4.** Destruction, Illusion and Restoration have no special sections. Is there nothing
  school-specific, or just not written yet?
  - **A:**
- **Q-MAG-6.** Perk changes (Destruction, Illusion, Restoration — schools without own section): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`refs/lorerim/perks.md`](refs/lorerim/perks.md)** (all tree perks of
  this skill, with handles); see also [`refs/perk-requirements.md`](refs/perk-requirements.md) §5. Answer separately for each skill: Destruction, Illusion, Restoration — schools without own section.
  - **A:**

### 8.2 Alteration

- The only exception to "no cross-links in magic": the **main Alteration level** raises the
  effective floor of magic sub-skills: `effectiveFloor(X) = max(…, cAlt × f(AlterationLevel))`.
- **Q-ALT-1.** All magic sub-skills of all schools, or only Alteration sub-skills? Coefficient
  and mapping (main skill scale 0–100 vs sub-skill scale 0–200; proposal:
  `cAlt × 2 × AlterationLevel`, `cAlt = 0.25`, still capped by `floorCap`)?
  - **A:** All sub-skills of all schools
- **Q-ALT-2.** Perk changes (Alteration): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`refs/lorerim/perks.md`](refs/lorerim/perks.md)** (all tree perks of
  this skill, with handles); see also [`refs/perk-requirements.md`](refs/perk-requirements.md) §5.
  - **A:**

### 8.3 Conjuration

**Sub-skills (parent: Conjuration):** `conjure-reanimated`, `conjure-daedra`, `conjure-bound`,
`conjure-other`. **Proposal** — additional: `conjure-atronach` (or fold into daedra),
`conjure-spirit` (spectral/ghost summons, familiars), `soul-magic` (Soul Trap, Banish,
Command Daedra, Expel).

**Optional mechanic A — summon upkeep** — **suspended** (decided 2026-11-11): the mod
concentrates on how XP is gained. Research R-9 found no unified upkeep in LoreRim, only a
scripted thrall cost in *Undeath* and Magic Redone summon changes. The text below is kept for
later:
- Summoned creatures drain the caster's magicka: `upkeepIdle` per second out of combat,
  `upkeepCombat` per second in combat (configurable). Does not apply to bound weapons.
- Conjuration XP for summons is granted **only while summons are in combat**.

**Optional mechanic B — reanimation control loss** (MCM toggle):
- A reanimated creature may break free with probability depending on creature level vs
  `conjure-reanimated` current level.

- **Q-CON-1.** Accept proposed extra sub-skills?
  - **A:** fold aetronach into daedr a(they are daedra), conjure-sprit and
    soul-magic is good.
- **Q-CON-2.** Upkeep: what happens when magicka runs out (summon dismissed? health drain?
  nothing)? What exactly grants XP "in combat": time spent in combat, damage dealt by summons,
  kills?
  - **A:** When magicka runs out, summon is dismissed. I propose that XP is
    granted by damage dealt by summons, if it is possible to calculate. If not -
    then by time in combat. Analysis for grinding is requested.
- **Q-CON-3.** Control loss: how often is the roll made (on summon, every N seconds, on each
  hit taken?), what happens (turns hostile, flees, collapses), how does it interact with
  Requiem/Lorerim permanent thralls?
  - **A:** Permanent thralls are immune. Roll is made every N seconds. Summon
    turns hostile. This only works for `reanimated`.
- **Q-CON-4.** ~~Does Lorerim already have a summon-upkeep mod?~~ Research R-9: no unified
  upkeep. Mechanic A is suspended. While it is suspended, how is summon XP granted? Proposal:
  vanilla casting XP, plus XP for damage dealt by summons to valid targets
  (`offense-summon` budget, §11.2).
  - **A:**
- **Q-CON-5.** Perk changes (Conjuration): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`refs/lorerim/perks.md`](refs/lorerim/perks.md)** (all tree perks of
  this skill, with handles); see also [`refs/perk-requirements.md`](refs/perk-requirements.md) §5.
  - **A:**

### 8.4 Enchanting

**Sub-skills (parent: Enchanting):** `enchanting-weapons`, `enchanting-armor` (light and heavy),
`enchanting-other` (clothing, jewelry). Their role is only to carry the general mechanic
(XP → sub-skill → main).

- **Q-ENC-1.** Which actions grant XP: enchanting, disenchanting, recharging? Vanilla grants for
  enchanting and disenchanting. Disenchanting → which sub-skill (by item type)?
  - **A:**

### 8.5 Wands and staves

**Sub-skill `wands-staffs`** (parent: Enchanting): XP when using wands and staves.

- **Q-ENC-2.** Vanilla staff use gives XP to the spell's school. Suppress that and send XP only to
  `wands-staffs`, or both? Where in Lorerim are the staff-related perks (research R-10)?
  - **A:**
- **Q-ENC-3.** Perk changes (Enchanting, incl. `wands-staffs`): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`refs/lorerim/perks.md`](refs/lorerim/perks.md)** (all tree perks of
  this skill, with handles); see also [`refs/perk-requirements.md`](refs/perk-requirements.md) §5.
  - **A:**

## 9. Thief skills

### 9.1 Speech

**Sub-skills (parent: Speech):** `speech-intimidation`, `speech-persuasion`, `speech-trading`,
`speech-thuum`.

- `speech-intimidation` / `speech-persuasion`: XP for successful intimidate / persuade checks
  in dialogue.
- `speech-thuum`: XP for using shouts. This is a **new link** (vanilla shouts give no Speech XP);
  justified because Lorerim's Speech tree contains the Thu'um perks.
- `speech-trading`: XP for buying and selling, proportional to the gold saved compared to a
  **reference price** = price at Speech 15 with neutral modifiers (no perks, no disposition/
  faction modifiers). Saving 2 gold → very little XP, saving 2000 → a lot.
  - **Proposal:** `xp = kTrade × log(1 + saved)`, per transaction, after netting buy/sell loops
    (see grinding-analysis G-V18, G-N12, G-N13).
  - Floor of `speech-trading` grows faster (`floorRate` override, e.g. 0.5).

- **Q-SPE-1.** Bribery: part of `speech-persuasion`, its own sub-skill, or no XP?
  - **A:** Part of persuasion
- **Q-SPE-2.** Shouts: XP per shout regardless of target, or only when it affects a hostile?
  Scaled by number of words used?
  - **A:** Scaled by number of words used. If we can distinguate between
    offensive and non-offensive shouts, then offensive give XP only when it
    affects hostile, and non-offensive give upon use.
    Note: There are not that many shouts, perhaps DR for each shout is a good
    idea
- **Q-SPE-3.** "Saved gold" when selling: `saved = actualSellPrice − referenceSellPrice`. Confirm;
  also what if the result is negative (worse than reference) — zero XP?
  - **A:** Confirm saved formula. ZeroXP
    when result is negative
- **Q-SPE-4.** Accept the log formula? Note: per transaction, log rewards splitting one big sale into many small ones (grinding-analysis G-N13); proposal — apply it to cumulative saved gold per merchant per game day.
  - **A:** Confirm proposal for cummulative saved gold
- **Q-SPE-5.** Perk changes (Speech): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`refs/lorerim/perks.md`](refs/lorerim/perks.md)** (all tree perks of
  this skill, with handles); see also [`refs/perk-requirements.md`](refs/perk-requirements.md) §5.
  - **A:**

### 9.2 Alchemy

**Sub-skills (parent: Alchemy):** `alchemy-poisons` (poisons for weapons), `alchemy-potions`
(all other potions).

**Crafting XP:** large when the sub-skill is below its floor, small otherwise.

**Use XP (main source):**

| Action | XP | Scaling |
|---|---|---|
| Drinking an **own** potion | normal | × strength (magnitude × duration) |
| **Own** poison hitting an enemy | ×2 | × strength |
| **Foreign** poison hitting an enemy | normal | no scaling |
| Drinking a foreign potion | none (implied by the draft) | — |

- Poison XP is granted on **hit**, not on application.
- Food is excluded (proposal; food is technically a potion record).
- "Own" = created by the player at an alchemy station; tracked by the DLL on the crafting event
  (crafted potions are dynamic forms).

- **Q-ALC-1.** Logic problem: alchemy sub-skills have no cross-links, and decay stops at floor, so
  current is **never below floor** — the "large crafting XP below floor" rule never triggers.
  Did you mean "below potential"? Or should crafting XP be large when the sub-skill is low in
  absolute terms (e.g. below 50)?
  - **A:**
- **Q-ALC-2.** Drinking a potion when it has no use (Restore Health at full health) — still XP?
  Proposal: for restore-type effects, scale by the amount actually restored.
  - **A:**
- **Q-ALC-3.** "Strength" for multi-effect potions: sum over effects, or primary effect only? And
  for instant effects (duration 0), use magnitude only?
  - **A:**
- **Q-ALC-4.** Confirm: drinking foreign (bought/looted) potions gives no XP?
  - **A:**
- **Q-ALC-5.** Perk changes (Alchemy): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`refs/lorerim/perks.md`](refs/lorerim/perks.md)** (all tree perks of
  this skill, with handles); see also [`refs/perk-requirements.md`](refs/perk-requirements.md) §5.
  - **A:**

### 9.3 Lockpicking, Pickpocket, Sneak

One sub-skill each (`lockpicking`, `pickpocket`, `sneak`), XP by vanilla (Requiem) rules; their
job is only to carry the general mechanic.

- **Q-THF-1.** Do these need anti-grind rules (see grinding-analysis G-V22…G-V25), or keep them
  vanilla for now?
  - **A:**
- **Q-THF-2.** Perk changes (Lockpicking, Pickpocket, Sneak): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`refs/lorerim/perks.md`](refs/lorerim/perks.md)** (all tree perks of
  this skill, with handles); see also [`refs/perk-requirements.md`](refs/perk-requirements.md) §5. Answer separately for each skill: Lockpicking, Pickpocket, Sneak.
  - **A:**

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
  `defense-magic` (wards, resistances), `pickpocket`, `speech`. Merge or split any? For example,
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
- Aim perturbation (if accepted, Q-WPN-7/8, Q-MAG-5): hook `RE::Projectile::Launch` (filtered
  to the player) for missiles, arrows and lobbers; per-frame update hooks for beam and flame
  projectiles; a miss or glancing chance in the melee hit pipeline. Details:
  [`refs/aim-perturbation.md`](refs/aim-perturbation.md).
- Continuous numeric bonuses from sub-skills (if accepted, Q-DYN-9): hook entry-point evaluation
  (`RE::BGSEntryPoint::HandleEntryPoint`) or the damage calculation. Details:
  [`refs/dynamic-skill-effects.md`](refs/dynamic-skill-effects.md).
- Sub-skill requirements on existing perks (if accepted, Q-DYN-13): one `GlobalVariable` per
  referenced sub-skill value, plus conditions injected into `BGSPerk::perkConditions` at
  `kDataLoaded` from a config table. Details:
  [`refs/perk-requirements.md`](refs/perk-requirements.md).

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

**Proposal:** the first deliverable of the plan is a tiny **telemetry SKSE plugin** that only logs
every vanilla skill XP award (skill, amount, game time, real time, context) without changing
anything. Play Lorerim normally for several sessions → we get real XP/hour per skill and can set
target pacing as "× of Lorerim baseline".

- **Q-RES-1.** Accept the telemetry-first approach?
  - **A:** Yes, accept

### 15.2 Research tasks

Static research of the installed profile was done on 2026-11-11 (Mutagen dump; report
`research-report.md` + TSV tables, kept outside the repo in `~/tmp/research`). Worksheets
derived from it: [`refs/lorerim/perks.md`](refs/lorerim/perks.md),
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
| R-8 | Done (static): school directions confirmed by Magic Redone keywords `Nox_KW_*`; perk trees in `refs/lorerim/perks.md` |
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
  and route main XP to the school of the spell being cast. Which one? (Same question for
  archetypes.)
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
