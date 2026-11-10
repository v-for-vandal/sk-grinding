# Skill Practice Overhaul — Design Document (draft v2)

> Status: **draft v2**. Translated from the original draft (`design.md`, kept unchanged),
> with consistent terminology and decisions from the first review round applied.
> Companion document: [`grinding-analysis.md`](grinding-analysis.md).
> Technical reference notes: [`refs/`](refs/): [`aim-perturbation.md`](refs/aim-perturbation.md),
> [`per-target-budget.md`](refs/per-target-budget.md),
> [`perk-requirements.md`](refs/perk-requirements.md),
> [`dynamic-skill-effects.md`](refs/dynamic-skill-effects.md).
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
  - Character level is **decoupled from skills** in Lorerim 5.0+; it grows from quests only.
    So slowing down main-skill growth does not slow down character leveling.
  - Clothing experience goes into **Light Armor** in Lorerim.
  - The "max 5 trainings per level" limit is **removed** in Lorerim.
  - Thu'um-improving perks live in the **Speech** tree in Lorerim.
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
| **Current** | Present level of a sub-skill, range **0–200**. Goes up with use, decays without use. Drives sub-skill perks. |
| **Floor** | "Muscle memory". Current never decays below floor. Has two parts: **own floor** (earned by the sub-skill itself, capped at `floorCap` = 75) and **effective floor** (own floor raised by cross-links, see §6). |
| **Potential** | Best level ever reached, additionally raised by books, trainers and other external sources. Below potential, current grows faster ("recalling"). Invariant: `potential ≥ current`. |
| **Sub-skill XP** | Experience earned by a sub-skill from a gameplay event. |
| **Main XP** | Experience added to a main skill. **The only source of main XP is sub-skill activity** (vanilla XP is suppressed). |
| **Conversion** | The function mapping sub-skill XP → main XP, defined per sub-skill (may be non-linear). |
| **Cross-link** | A rule that one sub-skill's own floor raises another sub-skill's effective floor. |
| **Use event** | Any event that grants XP to a sub-skill; resets its decay timer. |
| **Window rule** | A rule that grants XP depending on whether a complementary event happened within the last N seconds (dual-wield, with-shield, with-magic). |
| **Zone multiplier** | XP multiplier that depends on where current is relative to floor and potential (§5.3). |

## 4. Core model: experience flow

### 4.1 Decisions

- **Flow model.** Sub-skills and main skills are connected by **experience flow**, not by a
  level formula: every XP gain of a sub-skill produces a (converted) XP gain of its parent
  main skill. Main skill levels are therefore **monotonic** (never decrease).
- **Vanilla XP is fully suppressed.** The engine's own skill XP awards are intercepted and
  zeroed. Main skills grow only through §4.2.
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
  → if current XP > peak XP: main XP += Δpeak × g(peak level)   (§4.4; Q-XP-1, Q-XP-2)
                               peak XP = current XP
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
  - **A:**
- **Q-XP-2.** With the peak rule (§4.4) the rotation exploit G-N2 is gone. One case remains:
  when potential is above peak (raised by books or trainers), XP that raises peak gets the ×1.5
  recall multiplier. Should Δpeak be converted as is, or divided by the zone multiplier (raw)?
  Recommendation: raw, so that books/trainers make the sub-skill faster but do not boost the
  main skill.
  - **A:**
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
  - **A:**
- **Q-XP-7.** Target pacing (how many hours of normal play from 15 to 100 in a main skill,
  how many hours from 0 to 100/200 in an actively used sub-skill). See §15.1 for what we know
  about vanilla and the proposal to measure it.
  - **A:**

### 4.4 Conversion model (decided)

- **Peak rule.** Each sub-skill stores `peakXP`, the highest current XP it has ever had. Main
  XP is produced **only by XP that raises the peak**. Training back after decay (below peak)
  produces no main XP. This closes the rotation exploits G-N2, G-N26 and G-S2.
  - Peak is **not** potential. Potential is also raised by books, trainers and external
    increments (§4.1, §10). If conversion were tied to potential, a trainer raising potential
    would make the following learning produce *no* main XP, so training would hurt the main
    skill. Peak only follows the player's own practice.
- **Conversion function.** For a sub-skill with peak level `L`:
  `g(L) = k × (1 + e^(−T/w)) / (1 + e^((L − T)/w))`, a normalized logistic: `g(0) = k`,
  halved around `L ≈ T`, width `w`. `mainXP += Δpeak × g(L)`.
  - `k`, `T`, `w` are per sub-skill (config). Usually `k` is shared by a group of sub-skills
    (e.g. all Smithing materials), and `T` varies with how hard the sub-skill is.
  - Each sub-skill can contribute only a **finite** amount of main XP over its lifetime, so a
    high main skill requires breadth (several sub-skills). How much breadth is set per skill
    with `T`: high for skills where a specialist should go far (weapons), lower for skills where
    breadth is the theme (crafting, magic).
  - The sub-skill itself keeps growing for its own perks; only its contribution to the main
    skill fades.
- **Normalization.** Per main skill, a **reference path** is defined: a set of sub-skills each
  trained from 0 to a given level (e.g. 75). This path must take the main skill from 15 to 100.
  Each group of sub-skills gets a main-XP budget (absolute, e.g. "jewelry = Smithing 15→35", or
  a share of the rest). The `k` values are solved from the budgets, not tuned by hand.
- **Tool:** [`scripts/normalize_conversion.py`](scripts/normalize_conversion.py) with a per-skill
  TOML config (Smithing: [`scripts/smithing.toml`](scripts/smithing.toml)). It computes `k` per
  group, the main level each sub-skill gives alone, the lifetime contribution, `g(L)` tables,
  a path check, and (optionally) crafting pace in material units and active days. The XP curves
  are vanilla-style; the real Lorerim values for the main skill can be read from its AVIF
  record (or by the DLL at runtime).

## 5. Sub-skill value dynamics

### 5.1 Range and XP curve

- All sub-skills use the range **0–200**.
- XP needed to go from level `L` to `L+1` uses the vanilla-style curve:
  `cost(L) = improveMult × L^1.95 + improveOffset`, with `improveMult`, `improveOffset`
  configurable per sub-skill (exponent configurable globally). Levels for current, own floor
  and potential are all derived from their XP totals using the same curve.
- **Q-DYN-8.** Is a hard cap (200) needed at all? The engine does not require one: sub-skills are
  DLL data, and main-skill AVs can go above 100. An alternative is no cap, with the growing XP cost
  as a soft cap (polynomial: slow but endless growth; exponential: practically asymptotic).
  Constraints: some vanilla/Requiem formulas may clamp at 100 or misbehave above it (R-14), and
  the vanilla skill menu is designed for 0–100. A "design max" for balancing perks, `floorCap`
  and conversion is still needed, and all effect formulas must stay sane at any level (see
  [`refs/dynamic-skill-effects.md`](refs/dynamic-skill-effects.md) §3). Also: what should the
  *main* skill range be (0–100 as in vanilla, 0–200, uncapped)?
  - **A:**

### 5.2 Initial values

- New game is required for now (mid-game installation is deferred; see Q-OTH-1).
- **Q-DYN-1.** Starting values at new game: all zero, or derived from the race's starting
  main-skill values (e.g. Nord One-Handed 20 → floor/current of 1H sub-skills = some function)?
  - **A:**

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
  - **A:** I agree with the proposal

### 5.5 Decay

// TODO: Artem: Надо обсудить decay

- Each sub-skill remembers the game time of its last **use event**.
- For `t` = game hours since last use:
  - `t < 24`: no decay (grace period);
  - `24 ≤ t < 72`: decay rate ramps linearly from 0 to `decayRate_skill`;
  - `t ≥ 72`: decay rate = `decayRate_skill`.
- Decay lowers **current** only, never below effective floor. Floor and potential are
  unaffected. Main skills never decay.
- `decayRate_skill` is per sub-skill, from config/MCM. Skills that are hard to practise often
  decay slower (e.g. smithing materials), weapons faster.
- Calibration target: an unused weapon sub-skill fully returns to its floor in about
  **14 game days**.
- Decay is applied lazily (on each use event, on UI open, and on a periodic tick), computed
  from elapsed game time — not by a per-frame timer.
- **Q-DYN-4.** Units of `decayRate`: (a) fixed levels per game day — then a skill at 200 takes
  much longer to fall to floor than a skill at 60; or (b) relative to the gap at decay start —
  `rate = (current₀ − floor) / 12 days`, so "back to floor in 14 days" holds for any level.
  Recommendation: (b), or decay in XP rather than levels.
  - **A:**
- **Q-DYN-5.** Game time includes sleeping, waiting, fast travel and jail. Sleeping 7 days thus
  decays skills. Is that intended?
  - **A:**
- **Q-DYN-6.** Does any tiny use reset the timer (one hit on a chicken per day prevents all decay
  — grinding-analysis G-N6)? Proposal: the timer resets only when a sub-skill gained at least
  `minDailyUseXP` (configurable) within the current game day.
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
  - **A:**
- **Q-DYN-9.** How should continuous effects (Q-DYN-7) and other numeric bonuses be implemented?
  Sub-skills are not actor values, so ESP perks cannot read them directly. Options (details in
  [`refs/dynamic-skill-effects.md`](refs/dynamic-skill-effects.md)): (1) mirror sub-skills into
  spare AVs, which does not scale; (2) step entries conditioned on globals; (3) **Proposal:**
  compute the bonuses in the DLL via a hook on entry-point evaluation or damage calculation, with
  formulas from config. ESP perks stay for discrete abilities.
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
  updates). Details in [`refs/perk-requirements.md`](refs/perk-requirements.md).
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
  - **A:**
- **Q-CL-4.** Armor material sub-skills: cross-links between materials (e.g. all heavy materials
  ×1/2 to each other) or none?
  - **A:**

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
  (own sub-skill or merged into the closest vanilla one)? Depends on research R-4.
  - **A:**
- **Q-WPN-2.** How is light vs heavy bow/crossbow determined (keyword, weight, Requiem
  classification)? Depends on research R-4.
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
  by the DLL at a threshold, §5.6). See [`refs/perk-requirements.md`](refs/perk-requirements.md) §5; depends on research R-8. Answer separately for each skill: One-Handed, Two-Handed, Archery.
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
  - **A:**
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
  by the DLL at a threshold, §5.6). See [`refs/perk-requirements.md`](refs/perk-requirements.md) §5; depends on research R-8. Answer separately for each skill: Heavy Armor, Light Armor.
  - **A:**

### 7.3 Block (shields, parry, bash)

**Sub-skills (parent: Block):** `heavy-shield`, `light-shield`, `parry` (blocking with a weapon),
`bash` (bashing with a shield or a weapon).

- XP rules follow vanilla (with Requiem modifications), subject to anti-grind rules (§11).
- Shield type is determined by armor type of the shield (heavy/light).
- **Q-BLK-1.** A shield bash: XP to `bash` only, or to `bash` and the shield sub-skill?
  - **A:**
- **Q-BLK-2.** Is there any vanilla/Requiem Block XP source other than blocking hits and bashing
  (e.g. blocking spells/arrows, timed blocks from combat mods in Lorerim)? Research R-6.
  - **A:**
- **Q-BLK-3.** Perk changes (Block): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). See [`refs/perk-requirements.md`](refs/perk-requirements.md) §5; depends on research R-8.
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

**Material table** (Q-SMI-5). Keywords are vanilla/DLC and still need verifying. `tier` and `T`
are **placeholders** (from `scripts/smithing.toml`) for the user to fill in. Modded materials
come from research R-5.

| Material sub-skill | Keywords (verify) | Default tier source (required perk) | tier (category XP weight) | T (fade) | Notes |
|---|---|---|---|---|---|
| `iron` | `WeapMaterialIron`, `ArmorMaterialIron`, `ArmorMaterialIronBanded` | none | 1 | 25 | |
| `steel` | `WeapMaterialSteel`, `ArmorMaterialSteel`, `ArmorMaterialSteelPlate` | Steel Smithing | 1.5 | 35 | steel plate: own sub-skill or `steel`? |
| `leather` | `ArmorMaterialLeather` | none | 1 | 25 | |
| `hide` | `ArmorMaterialHide` | none | ? | ? | merge with `leather`? |
| `imperial` | `WeapMaterialImperial`, `ArmorMaterialImperialLight/Heavy/Studded` | Steel Smithing | ? | ? | |
| `scaled` | `ArmorMaterialScaled` | Advanced Armors | ? | ? | |
| `elven` | `WeapMaterialElven`, `ArmorMaterialElven`, `ArmorMaterialElvenGilded` | Elven Smithing | 3 | 50 | |
| `orcish` | `WeapMaterialOrcish`, `ArmorMaterialOrcish` | Orcish Smithing | 3 | 50 | |
| `dwarven` | `WeapMaterialDwarven`, `ArmorMaterialDwarven` | Dwarven Smithing | 3 | 50 | |
| `glass` | `WeapMaterialGlass`, `ArmorMaterialGlass` | Glass Smithing | 5 | 65 | |
| `ebony` | `WeapMaterialEbony`, `ArmorMaterialEbony` | Ebony Smithing | 6 | 75 | |
| `daedric` | `WeapMaterialDaedric`, `ArmorMaterialDaedric` | Daedric Smithing | 8 | 90 | |
| `dragon` | `DLC1WeapMaterialDragonbone`, `ArmorMaterialDragonplate`, `ArmorMaterialDragonscale` | Dragon Armor | 10 | 90 | bone vs scale split? |
| `stalhrim` | `DLC2WeaponMaterialStalhrim`, `DLC2ArmorMaterialStalhrimLight/Heavy` | Ebony Smithing (DB) | ? | ? | |
| `nordic` | `DLC2WeaponMaterialNordic`, `DLC2ArmorMaterialNordicLight/Heavy` | Advanced Armors (DB) | ? | ? | |
| `bonemold` | `DLC2ArmorMaterialBonemoldLight/Heavy` | Advanced Armors (DB) | ? | ? | |
| `chitin` | `DLC2ArmorMaterialChitinLight/Heavy` | Elven Smithing (DB) | ? | ? | |
| `silver` | `WeapMaterialSilver` | ? | ? | ? | also jewelry |
| `gold` | jewelry only? | ? | ? | ? | also jewelry |
| `falmer` | `WeapMaterialFalmer`, `ArmorMaterialFalmer` (verify) | ? | ? | ? | |
| `draugr` / `ancient-nord` | `WeapMaterialDraugr`, `WeapMaterialDraugrHoned` | ? | ? | ? | |
| `dawnguard` | `DLC1ArmorMaterialDawnguard` | ? | ? | ? | |
| `wood` | `WeapMaterialWood` (bows) | none | ? | ? | |
| AE / Creation Club materials | various | ? | ? | ? | R-5 |
| modded materials | R-5 | ? | ? | ? | R-5 |
| `other-smithing` | fallback | none | 1 | 25 | |

- **Q-SMI-5.** Fill in the material table: tier (category XP weight), `T`, and any merges or splits
  (hide/leather, steel/steel plate, dragonbone/dragonscale, AE and modded materials).
  - **A:**
- **Q-SMI-6.** How is the budget left after `jewelry` split between categories and materials?
  The placeholder is 50/50, which means one category 0→75 gives about the same as one mid
  material.
  - **A:**
- **Q-SMI-7.** Is `jewelry` a category like the others, so a silver ring feeds `jewelry` +
  `silver`, or a standalone sub-skill with no material XP? Gems: no XP, or part of `units`?
  - **A:**
- **Q-SMI-8.** What counts as the "main material" in recipes with several ingredients (glass =
  malachite + moonstone; steel plate = steel + corundum; leather strips everywhere)? Proposal:
  sum the units of all ingredients that belong to the item's material (ingots or ore of that
  material). Ignore generic ones (leather strips, firewood). Fallback: the most numerous
  ingredient.
  - **A:**
- **Q-SMI-9.** `temperFactor` and the temper XP source. Proposal: same formulas applied to the
  temper recipe units, `temperFactor = 1`. Temper recipes are already cheap (one unit per step),
  and they give XP only when quality rises.
  - **A:**
- **Q-SMI-10.** Arrows and bolts: one recipe produces a bundle. Proposal: count `units` of the
  recipe as usual (one recipe = one craft), with an optional per-category multiplier.
  - **A:**
- **Q-SMI-4.** Perk changes (Smithing): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). See [`refs/perk-requirements.md`](refs/perk-requirements.md) §5; depends on research R-8.
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
  - **A:**
- **Q-MAG-4.** Destruction, Illusion and Restoration have no special sections. Is there nothing
  school-specific, or just not written yet?
  - **A:**
- **Q-MAG-6.** Perk changes (Destruction, Illusion, Restoration — schools without own section): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). See [`refs/perk-requirements.md`](refs/perk-requirements.md) §5; depends on research R-8. Answer separately for each skill: Destruction, Illusion, Restoration — schools without own section.
  - **A:**

### 8.2 Alteration

- The only exception to "no cross-links in magic": the **main Alteration level** raises the
  effective floor of magic sub-skills: `effectiveFloor(X) = max(…, cAlt × f(AlterationLevel))`.
- **Q-ALT-1.** All magic sub-skills of all schools, or only Alteration sub-skills? Coefficient
  and mapping (main skill scale 0–100 vs sub-skill scale 0–200; proposal:
  `cAlt × 2 × AlterationLevel`, `cAlt = 0.25`, still capped by `floorCap`)?
  - **A:**
- **Q-ALT-2.** Perk changes (Alteration): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). See [`refs/perk-requirements.md`](refs/perk-requirements.md) §5; depends on research R-8.
  - **A:**

### 8.3 Conjuration

**Sub-skills (parent: Conjuration):** `conjure-reanimated`, `conjure-daedra`, `conjure-bound`,
`conjure-other`. **Proposal** — additional: `conjure-atronach` (or fold into daedra),
`conjure-spirit` (spectral/ghost summons, familiars), `soul-magic` (Soul Trap, Banish,
Command Daedra, Expel).

**Optional mechanic A — summon upkeep** (MCM toggle):
- Summoned creatures drain the caster's magicka: `upkeepIdle` per second out of combat,
  `upkeepCombat` per second in combat (configurable). Does not apply to bound weapons.
- Conjuration XP for summons is granted **only while summons are in combat**.

**Optional mechanic B — reanimation control loss** (MCM toggle):
- A reanimated creature may break free with probability depending on creature level vs
  `conjure-reanimated` current level.

- **Q-CON-1.** Accept proposed extra sub-skills?
  - **A:**
- **Q-CON-2.** Upkeep: what happens when magicka runs out (summon dismissed? health drain?
  nothing)? What exactly grants XP "in combat": time spent in combat, damage dealt by summons,
  kills?
  - **A:**
- **Q-CON-3.** Control loss: how often is the roll made (on summon, every N seconds, on each
  hit taken?), what happens (turns hostile, flees, collapses), how does it interact with
  Requiem/Lorerim permanent thralls?
  - **A:**
- **Q-CON-4.** Does Lorerim already have a summon-upkeep mod? (Research R-9.)
  - **A:**
- **Q-CON-5.** Perk changes (Conjuration): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). See [`refs/perk-requirements.md`](refs/perk-requirements.md) §5; depends on research R-8.
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
  by the DLL at a threshold, §5.6). See [`refs/perk-requirements.md`](refs/perk-requirements.md) §5; depends on research R-8.
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
  - **A:**
- **Q-SPE-2.** Shouts: XP per shout regardless of target, or only when it affects a hostile?
  Scaled by number of words used?
  - **A:**
- **Q-SPE-3.** "Saved gold" when selling: `saved = actualSellPrice − referenceSellPrice`. Confirm;
  also what if the result is negative (worse than reference) — zero XP?
  - **A:**
- **Q-SPE-4.** Accept the log formula? Note: per transaction, log rewards splitting one big sale into many small ones (grinding-analysis G-N13); proposal — apply it to cumulative saved gold per merchant per game day.
  - **A:**
- **Q-SPE-5.** Perk changes (Speech): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). See [`refs/perk-requirements.md`](refs/perk-requirements.md) §5; depends on research R-8.
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
  by the DLL at a threshold, §5.6). See [`refs/perk-requirements.md`](refs/perk-requirements.md) §5; depends on research R-8.
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
  by the DLL at a threshold, §5.6). See [`refs/perk-requirements.md`](refs/perk-requirements.md) §5; depends on research R-8. Answer separately for each skill: Lockpicking, Pickpocket, Sneak.
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
- **Q-BT-4.** With unlimited trainings (Lorerim) and stacking potential, a rich player can buy
  potential up to 200. Cap trainer-granted potential (e.g. not above `current + X`, or not above
  the trainer's own skill level × 2)? See grinding-analysis G-N9.
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
- **Main XP only from raising the sub-skill peak** (§4.4). Closes G-N2, G-N26, G-S2.
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
- **Q-UI-2.** Fallback if PrismaUI turns out problematic in Lorerim (version/compatibility):
  SKSE Menu Framework (ImGui) as plan B — ok?
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

- Runtime: Skyrim AE version used by Lorerim 5.0+ (exact version to verify, R-1);
  CommonLibSSE-NG with Address Library; build on Windows with MSVC + CMake + vcpkg.
- Persistence: SKSE co-save serialization of all sub-skill states (XP for current, own floor,
  potential and peak; last-use time), per-target and crafting budgets, the active-game-hours
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

- **Q-TECH-1.** Is a mod-added ESP acceptable, or must the mod be DLL-only (perks created at
  runtime)? ESP is simpler and more conventional.
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
  - **A:**

### 15.2 Research tasks

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
  unchanged. Since main skills never decrease, "no omnipotence" relies only on sub-skill decay
  and sub-skill perks. Is that enough, or should something else (e.g. main-skill perk conditions
  on sub-skill current) also be considered?
  - **A:**
- **Q-OTH-5.** Mod name?
  - **A:**
