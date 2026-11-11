# Conjuration

> Part of the design: [`design-v2.md`](../../design-v2.md) (was §8.3). Bare `§N` references point to design-v2.md. Index of skills: [`skills/README.md`](../README.md).

**Sub-skills (parent: Conjuration):** `conjure-reanimated`, `conjure-daedra` (atronachs
included: they are daedra), `conjure-spirit` (spectral/ghost summons, familiars), `conjure-bound`,
`soul-magic` (Soul Trap, Banish, Command Daedra, Expel), `conjure-other` (decided, Q-CON-1).

- Bound weapons: attacks with a bound weapon give XP to **both** the weapon's sub-skill and
  `conjure-bound` (decided, Q-WPN-6).

**Summon XP** (decided, Q-CON-2): by **damage dealt by the player's summons and thralls**; if that
cannot be measured reliably, by time the summon spends in combat. Grinding analysis:

- Damage is measured in the same hit/damage hook as weapon effects: the attacker is a commanded
  actor of the player. Proposal: `xp = kSummon × damage / targetMaxHealth` (the fraction of the
  target's health removed, × a target-level factor), so a stronger summon does not give more XP
  per kill, only faster kills.
- Risks (grinding-analysis G-N18, G-V15): a harmless or endless opponent (mudcrab, essential NPC,
  regenerating troll, an enemy's summon), and summon/dismiss spam. Closed by: valid-target rules
  (TW: no teammates, followers, own summons, non-hostile actors) and the per-target budget
  `offense-summon` (damage, keyed by victim, §11.2, [`refs/per-target-budget.md`](../../refs/per-target-budget.md)).
  Casting the summon itself gives only the vanilla casting XP, with diminishing repetition.
- Time in combat (fallback) is weaker: a summon can stay "in combat" with an unreachable or
  fleeing enemy forever. It would need its own cap per game hour and a "target took damage
  recently" condition, so it is used only if damage measuring fails.

**Optional mechanic A — summon upkeep** — **suspended** (decided 2026-11-11): the mod
concentrates on how XP is gained. Research R-9 found no unified upkeep in LoreRim, only a
scripted thrall cost in *Undeath* and Magic Redone summon changes. The text below is kept for
later:
- Summoned creatures drain the caster's magicka: `upkeepIdle` per second out of combat,
  `upkeepCombat` per second in combat (configurable). Does not apply to bound weapons. When
  magicka runs out, the summon is dismissed (Q-CON-2).
- Conjuration XP for summons is granted **only while summons are in combat**.

**Optional mechanic B — reanimation control loss** (MCM toggle):
- A reanimated creature may break free with probability depending on creature level vs
  `conjure-reanimated` current level (decided, Q-CON-3):
  - only reanimated creatures (`conjure-reanimated`); Requiem/LoreRim **permanent thralls are
    immune**;
  - the roll is made every `controlRollSec` seconds (config) while the creature is active;
  - a creature that breaks free **turns hostile**.

- **Q-CON-5.** Perk changes (Conjuration): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`perks.md`](perks.md)** (all tree perks of
  this skill, with handles; also `perks-removed.md`, `perks-ordinator.md` there); see also [`refs/perk-requirements.md`](../../refs/perk-requirements.md) §5.
  - **A:**
