# Destruction

> Part of the design: [`design-v2.md`](../../design-v2.md) (§8.1 general magic rules: [`magic/design.md`](../magic/design.md)). Bare `§N` references point to design-v2.md. Index of skills: [`skills/README.md`](../README.md).

Skeleton: only the school-specific part. Forms (`missile`, `beam`, `flame`, …), archetypes, XP per
cast and accuracy are common to all schools and live in [`magic/design.md`](../magic/design.md).

**Sub-skills (parent: Destruction):** forms and archetypes as in [magic](../magic/design.md), plus
the school sub-directions below.

**School sub-directions** (proposal, from the perk tree and Magic Redone keywords, R-8):

| Sub-skill | Spells (keywords) | Perks (guessed from names, to check) |
|---|---|---|
| `destruction/fire` | fire damage (`Nox_KW_Destruction_Perk_Pyromancy`) | DES-06, DES-12, DES-13, DES-32 |
| `destruction/frost` | frost damage (`Nox_KW_Destruction_Perk_Cryomancy`) | DES-03, DES-08, DES-14, DES-33 |
| `destruction/shock` | shock damage (`Nox_KW_Destruction_Perk_Electromancy`) | DES-04, DES-09, DES-15, DES-34 |
| `destruction/arcane` | `Nox_KW_Destruction_Arcane`, `Nox_KW_Destruction_Perk_ArcaneFocus` | DES-18, DES-21, DES-25 |
| `destruction/blood` | `Nox_KW_Destruction_Perk_BloodMagic`, absorb (`Nox_KW_Destruction_Absorb_MS`) | DES-19, DES-22, DES-26, DES-29 |
| `destruction/entropic` | `Nox_KW_Destruction_Entropic`, `Nox_KW_Destruction_Perk_EntropicFocus` | DES-20, DES-24, DES-27 |
| `destruction/other` | fallback | — |

- **Q-DES-1.** Sub-directions: keep the list above, merge (e.g. `blood` into `entropic`) or
  split anything? Exact keyword → sub-skill mapping goes to config (R-8).
  - **A:**

**XP rules:** as in [magic](../magic/design.md) (Q-MAG-3), nothing Destruction-specific yet.

**Continuous effects:** form sub-skills: accuracy ([magic](../magic/design.md)). School
sub-directions: not decided.

- **Q-DES-2.** Continuous effects of the sub-directions (e.g. damage of spells of that element ×
  `1 + effect(L)`, curve of §5.7), or "no continuous effects"?
  - **A:**
- **Q-DES-3.** Perk changes (Destruction): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`perks.md`](perks.md)** (all tree perks of
  this skill, with handles; also `perks-removed.md`, `perks-ordinator.md` there); see also [`refs/perk-requirements.md`](../../refs/perk-requirements.md) §5.
  - **A:**
