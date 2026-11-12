# Block (shields, parry, bash)

> Part of the design: [`design-v2.md`](../../design-v2.md) (was §7.3). Bare `§N` references point to design-v2.md. Index of skills: [`skills/README.md`](../README.md).

**Sub-skills (parent: Block):** `heavy-shield`, `light-shield`, `parry` (blocking with a weapon),
`bash` (bashing with a shield or a weapon).

- XP rules follow vanilla (with Requiem modifications), subject to anti-grind rules (§11).
- Shield type is determined by armor type of the shield (heavy/light).
- **Bash (decided, Q-BLK-1):** a shield bash gives full XP to `bash` and
  `XP × bashShieldFactor` (0.5, config) to the shield's sub-skill (`heavy-shield` /
  `light-shield`). A weapon bash gives XP to `bash` only.
- **Torch bash** (decided): a bash or power bash with a torch (Torch Combat, BLK-12) is treated
  as a weapon bash: XP to `bash` only, damage scaled by the `bash` effect below. Whether the
  engine reports a torch bash as a bash hit (and gives Block XP) is part of R-6.

**Continuous effects** (curve of §5.7):

- **Block stamina cost** (`heavy-shield` / `light-shield` for a shield block, `parry` for a
  weapon block): the stamina the engine takes for a blocked hit is multiplied by
  `1 + effect(L)` (decided). Here the "penalty" part of the curve makes the block **more**
  expensive and the "bonus" part **cheaper**: `pen = 50`, `P = 45`, `M = 30`, `h = 27.5`, `p = 1`
  (+50% at 0, 0 at the usual checkpoint 45, about −20% at 100, limit −30%):

  | L | 0 | 20 | 45 | 60 | 100 | 150 | 200 | 500 |
  |---|---|---|---|---|---|---|---|---|
  | stamina cost, % | +50 | +27.8 | 0 | −10.6 | **−20.0** | −23.8 | −25.5 | −28.3 |

  With Requiem an empty stamina bar breaks the block, so a beginner's block breaks sooner and a
  master's later. Implementation: [`impl/block-stamina-cost.md`](../../impl/block-stamina-cost.md).
- **The share of damage blocked is not changed** (decided). The engine already scales it with
  the Block level (which our sub-skills feed), Requiem allows up to 100% (`fBlockMax = 1`) and
  perks multiply it, and Block XP is paid per point of damage blocked: a bonus or penalty there
  would count the level twice, push towards full immunity and change XP.
- **`bash` damage** (decided, Q-BLK-4): the damage of the player's bashes (shield or weapon,
  normal or power bash) is multiplied by `1 + effect(L)`, `L` = level of `bash`, with the same
  curve as the weapon sub-skills ([weapons](../weapons/design.md), continuous effects):
  `pen = 20`, `P = 50`, `M = 45`, `h = 40`, `p = 1`: −20% at 0, 0 at 50, +25% at 100, +35.5% at
  200, limit +45%. Applied once per bash hit after vanilla / Requiem bash perks
  (`ModBashingDamage`). On a bash only this multiplier applies: the weapon sub-skill and
  `dual-wield` damage effects do not (a bash is not an attack with the weapon's blade).
  Implementation: the same hit-damage multiplier as weapons, *hit-damage-multiplier* in
  [`impl/`](../../impl/README.md).
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
