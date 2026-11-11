# Block (shields, parry, bash)

> Part of the design: [`design-v2.md`](../../design-v2.md) (was §7.3). Bare `§N` references point to design-v2.md. Index of skills: [`skills/README.md`](../README.md).

**Sub-skills (parent: Block):** `heavy-shield`, `light-shield`, `parry` (blocking with a weapon),
`bash` (bashing with a shield or a weapon).

- XP rules follow vanilla (with Requiem modifications), subject to anti-grind rules (§11).
- Shield type is determined by armor type of the shield (heavy/light).
- **Bash (decided, Q-BLK-1):** a shield bash gives full XP to `bash` and
  `XP × bashShieldFactor` (0.5, config) to the shield's sub-skill (`heavy-shield` /
  `light-shield`). A weapon bash gives XP to `bash` only.
- **Q-BLK-2.** Is there any vanilla/Requiem Block XP source other than blocking hits and bashing
  (e.g. blocking spells/arrows, timed blocks from combat mods in Lorerim)? Research R-6.
  - **A:**
- **Q-BLK-3.** Perk changes (Block): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`perks.md`](perks.md)** (all tree perks of
  this skill, with handles; also `perks-removed.md`, `perks-ordinator.md` there); see also [`refs/perk-requirements.md`](../../refs/perk-requirements.md) §5.
  - **A:**
