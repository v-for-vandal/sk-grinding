# Illusion

> Part of the design: [`design-v2.md`](../../design-v2.md) (§8.1 general magic rules: [`magic/design.md`](../magic/design.md)). Bare `§N` references point to design-v2.md. Index of skills: [`skills/README.md`](../README.md).

Skeleton: only the school-specific part. Forms (`missile`, `beam`, `flame`, …), archetypes, XP per
cast and accuracy are common to all schools and live in [`magic/design.md`](../magic/design.md).

**Sub-skills (parent: Illusion):** forms and archetypes as in [magic](../magic/design.md), plus
the school sub-directions below.

**School sub-directions** (proposal: one per LoreRim perk branch; keywords from the perk
conditions, R-8):

| Sub-skill | Spells (keywords) | Perks (guessed from names, to check) |
|---|---|---|
| `illusion/emotional` | `MagicInfluenceCharm`, `…Fear`, `…Frenzy`, `Nox_KW_Illusion_Command`, `…_Rally` | ILL-03 |
| `illusion/perceptual` | `MagicInvisibility`, `MagicNightEye`, `Nox_KW_Illusion_Chameleon`, `…_Muffle`, `…_Sound` | ILL-05 |
| `illusion/delusive` | `Nox_KW_Illusion_Blind`, `…_Death`, `…_Nightmare`, `…_Noise`, `…_Pain`, `…_Silence`, `…_Sleep` | ILL-07 |
| `illusion/shadow` | `Nox_KW_Illusion_Sanctuary`, `…_SanctuaryEtherealize`, `…_ShadowDrain`, `…_ShadowStride`, `…_ShadowSummon` | ILL-10 |
| `illusion/other` | fallback | — |

- **Q-ILL-1.** Sub-directions: keep the list above (it follows the perk branches), merge or
  split anything? Exact keyword → sub-skill mapping goes to config (R-8).
  - **A:**
- **Q-ILL-2.** XP: vanilla Illusion XP is given only when the spell affects a target (no XP on a
  resisted or too-high-level target). Keep that, or change anything for the sub-skills?
  - **A:**

**Continuous effects:** form sub-skills: accuracy ([magic](../magic/design.md)). School
sub-directions: not decided.

- **Q-ILL-3.** Continuous effects of the sub-directions (e.g. the maximum target level of
  emotional / delusive spells, duration of perceptual spells, curve of §5.7), or "no continuous
  effects"?
  - **A:**
- **Q-ILL-4.** Perk changes (Illusion): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`perks.md`](perks.md)** (all tree perks of
  this skill, with handles; also `perks-removed.md`, `perks-ordinator.md` there); see also [`refs/perk-requirements.md`](../../refs/perk-requirements.md) §5.
  - **A:**
