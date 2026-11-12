# Restoration

> Part of the design: [`design-v2.md`](../../design-v2.md) (§8.1 general magic rules: [`magic/design.md`](../magic/design.md)). Bare `§N` references point to design-v2.md. Index of skills: [`skills/README.md`](../README.md).

Skeleton: only the school-specific part. Forms (`missile`, `beam`, `flame`, …), archetypes, XP per
cast and accuracy are common to all schools and live in [`magic/design.md`](../magic/design.md).

**Sub-skills (parent: Restoration):** forms and archetypes as in [magic](../magic/design.md), plus
the school sub-directions below.

**School sub-directions** (decided, Q-RES-1; from the perk tree and Magic Redone keywords, R-8):

| Sub-skill | Spells (keywords) | Perks (guessed from names, to check) |
|---|---|---|
| `restoration/help` | healing and buffs: `MagicRestoreHealth`, `Nox_KW_Restoration_Cure`, `Nox_KW_Restoration_ResistMagic`, `Nox_KW_Restoration_FortifyAttributes` | RES-04, RES-07, RES-15, RES-19 |
| `restoration/wards` | `MagicWard` (decided in Q-MAG-1: wards are a sub-skill, not a form) | RES-09, RES-18, RES-24 |
| `restoration/sun` | `Nox_KW_Restoration_Perk_Heliomancy` (sun / anti-undead) | RES-14, RES-21, RES-23 |
| `restoration/poison` | `Nox_KW_Restoration_Poison`, `Nox_KW_Restoration_Perk_Venomancy` | RES-17, RES-20, RES-25 |
| `restoration/other` | turn undead, fallback | — |

- `restoration/help` merges the proposed `healing` and `protection` (decided, Q-RES-1). Turn undead
  goes to `other` (decided). Exact keyword → sub-skill mapping goes to config
  (R-8).

**XP** (decided, Q-RES-2, Q-RES-5, Q-RES-6, Q-RES-8). Implementation:
[`impl/heal-xp.md`](../../impl/heal-xp.md).

- Vanilla sources are kept: healing gives XP only for health actually restored (none at full
  health), wards per damage absorbed.
- **XP from the amount without our multiplier** (Q-RES-8): health restored and damage absorbed
  are divided by our continuous-effect multiplier before XP is computed, so the `help` / `wards`
  bonus does not raise XP (and the penalty does not lower it).
- **Healing self:** × 1 in combat, × 0.1 out of combat ("in combat" = the player is in combat),
  plus diminishing repetition (§11.2, item 4).
- **Healing others:**
  - only health the target lost to damage **not caused by the player** counts (the player's
    own hits on a follower do not; "hit your follower, then heal them" gives nothing);
  - × 1 in combat, × 0.1 out of combat;
  - **fallback** if tracking the damage source turns out hard: no filter, × 0.2 in combat,
    × 0.1 out of combat;
  - per-target budget (§11.2, item 2; [`refs/per-target-budget.md`](../../refs/per-target-budget.md)):
    key (healed actor, `support-heal`), budget `kHeal × target max health` per refill period.
- Wards and offensive spells (`sun`, `poison`) go through the general budgets (`defense-magic`,
  `offense-magic`, Q-AG-3): a weak hostile mage hitting your ward gives little.

**Continuous effects** (decided, Q-RES-3: the standard effect with the shared curve of §5.7).
Form sub-skills: accuracy ([magic](../magic/design.md)). School sub-directions, the player's
spells of that sub-direction:

| Sub-skill | Effect × `1 + effect(L)` (decided, Q-RES-7) |
|---|---|
| `restoration/help` | magnitude (health restored, buff strength) |
| `restoration/wards` | ward strength |
| `restoration/sun` | damage |
| `restoration/poison` | damage |
| `restoration/other` | none |

- Curve parameters (decided, Q-RES-7): the same as weapon damage and `bash`: `pen = 20`,
  `P = 50`, `M = 45`, `h = 40`, `p = 1` (−20% at 0, 0 at 50, +25% at 100, limit +45%).
- XP is computed from the amount without this multiplier (Q-RES-8, see *XP*).
- **Q-RES-4.** Perk changes (Restoration): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`perks.md`](perks.md)** (all tree perks of
  this skill, with handles; also `perks-removed.md`, `perks-ordinator.md` there); see also [`refs/perk-requirements.md`](../../refs/perk-requirements.md) §5.
  - **A:**
