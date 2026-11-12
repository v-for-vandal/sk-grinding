# Alchemy

> Part of the design: [`design-v2.md`](../../design-v2.md) (was §9.2). Bare `§N` references point to design-v2.md. Index of skills: [`skills/README.md`](../README.md).

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
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`perks.md`](perks.md)** (all tree perks of
  this skill, with handles; also `perks-removed.md`, `perks-ordinator.md` there); see also [`refs/perk-requirements.md`](../../refs/perk-requirements.md) §5.
  - **A:**
