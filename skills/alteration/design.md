# Alteration

> Part of the design: [`design-v2.md`](../../design-v2.md) (was §8.2). Bare `§N` references point to design-v2.md. Index of skills: [`skills/README.md`](../README.md).

Skeleton: only the school-specific part. Forms, archetypes, XP per cast and accuracy are common to
all schools and live in [`magic/design.md`](../magic/design.md).

**Sub-skills (parent: Alteration):** forms and archetypes as in [magic](../magic/design.md), plus
the school sub-directions below.

**School sub-directions** (decided: one per LoreRim perk branch). A spell's sub-direction comes from
the keywords of its costliest effect (Magic Redone `Nox_KW_*`, from the LoreRim dump 2026-11-11);
rules are checked **in this order**, first match wins. Config:
[`config/magic-directions.toml`](../../config/magic-directions.toml) `[[alteration]]`.

| # | Sub-skill | Rule (keywords of the costliest effect) | Spells (Magic Redone; LoreRim mods in brackets) | Perks |
|---|---|---|---|---|
| 1 | `alteration/weather` | `Nox_KW_Alteration_Wind`, `Nox_KW_Alteration_Perk_WeatherMagic` | Whirlwind Sphere / Strike / Blast / Nova / Cloak / Storm, Tornado, Wind Weapon, Control Weather [Wind Running; Sonic Mage spells] | ALT-19, ALT-20 |
| 2 | `alteration/geomancy` | the Earth Cloak effect `005FC4` (by effect, its keywords say Kinetomancy); `Nox_KW_Physical_005…025` (stone damage; wind spells also carry them, rule 1 takes those first) | Knock, Stone Shot, Cluster Shot, Bash, Boulder Strike, Tremor, Earthquake, Earth Cloak [Perilous Path, Raise Wall] | ALT-22 (telekinetic + physical); ALT-15 also boosts them |
| 3 | `alteration/kinetic` | `Nox_KW_Alteration_Telekinetic`, `Nox_KW_Alteration_Perk_Kinetomancy`, `MagicTelekinesis` | Telekinetic Strike / Blast / Nova / Disarm / Execution / Storm / Weapon, Telekinetic Hand, Telekinetic Grab, Jump, Jump Mountains, Mark and Recall [Telepathic Instinct, Battletide, Milestones] | ALT-15, ALT-22 |
| 4 | `alteration/improve` | `MagicArmorSpell`, `Nox_KW_Alteration_Transmutation`, `Nox_KW_Alteration_Size`, `Nox_KW_Alteration_TransmuteDisintegration` | mage armor: Oak / Stone / Iron / Ebonyflesh, Dragonhide, Fire / Frost / Shock Shell and Shield; transmutation: Grow / Enlarge / Titanize, Reduce / Shrink, Transmute Night Eye / Muscles / Corrosion / Disintegration / Corpus / Ore, Empowered Weapon, Ash Grasp / Shell / Cloud / Rune / Storm, Petrify, Polymorph, Waterbreathing, Open … Lock, Excavate, Arcane Mending [Apocalypse *Transmutate …*, Triumvirate Wildshape] | ALT-04, ALT-18 (mage armor); ALT-16, ALT-23 (transmutation) |
| — | `alteration/other` | no match | Weakness to Fire / Frost / Shock, Paralyzing Touch, Paralyze, Paralyzing Storm, Burden, Encumber, Feather, Pack Mule, Accelerate, Haste, Bounce, Corrode, Disintegrate, Detect Life / Dead / Aura, Candlelight, Magelight, Waterwalking, Featherfall, Equilibrium, Etherealize, Bend Time, Dimension Door, most Apocalypse spells (`WB_Alteration` only) | — |

- Principle: a spell goes where the **perk that boosts it** is (the keywords above are exactly the
  ones the LoreRim perks check). Exceptions: Earth Cloak (Kinetomancy keyword, but earth: rule 2);
  Ash spells and Petrify carry `MagicParalysis` but are transmutation (ALT-16 boosts them).
- Not decided, check in the first in-game test: the small `Nox_KW_Physical_005` water spell of
  another mod (Psijic *Water Globe*) lands in `geomancy`; Apocalypse spells without Nox keywords
  land in `other`.

- The only exception to "no cross-links in magic": the **main Alteration level** raises the
  effective floor of **all magic sub-skills of all schools** (decided, Q-ALT-1):
  `effectiveFloor(X) = max(…, cAlt × 2 × AlterationLevel)`, `cAlt = 0.2` (config), still capped
  by `floorCap`: Alteration 100 → floor 40 for every magic sub-skill (decided, Q-ALT-1). The
  `× 2` maps the main-skill scale (0–100) to the sub-skill scale (0–200).
- **Q-ALT-2.** Perk changes (Alteration): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`perks.md`](perks.md)** (all tree perks of
  this skill, with handles; also `perks-removed.md`, `perks-ordinator.md` there); see also [`refs/perk-requirements.md`](../../refs/perk-requirements.md) §5.
  - **A:**
