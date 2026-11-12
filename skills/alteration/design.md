# Alteration

> Part of the design: [`design-v2.md`](../../design-v2.md) (was §8.2). Bare `§N` references point to design-v2.md. Index of skills: [`skills/README.md`](../README.md).

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
