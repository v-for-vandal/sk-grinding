# Magic: general structure (all schools)

> Part of the design: [`design-v2.md`](../../design-v2.md) (was §8.1). Bare `§N` references point to design-v2.md. Index of skills: [`skills/README.md`](../README.md).

For every spell cast that grants XP, the spell feeds three kinds of sub-skills:

1. **Form** — gameplay shape of the spell (combination of casting type, delivery and projectile
   type). The form sub-skills are cross-school; their XP goes to the main skill of the spell's
   school.
2. **Archetype** — grouping by mod keywords (`druidic`, `shamanic`, …); spells without an
   archetype use `standard`. Actual list from Lorerim research (R-7).
3. **School sub-direction** — e.g. `destruction/fire`, `destruction/frost`, `destruction/shock`,
   `restoration/healing`, `restoration/sun`, `restoration/poison`, … from keywords and/or
   Lorerim perk tree structure (R-8). Fallback: `<school>/other`.

No cross-links between magic sub-skills, except the Alteration rule ([alteration](../alteration/design.md)).

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

**Accuracy** (decided, Q-MAG-5; mechanics in [`refs/aim-perturbation.md`](../../refs/aim-perturbation.md)):

- Only the **form** sub-skill drives inaccuracy (not the school sub-direction or archetype).
- Only `missile`, `lobber` and `beam` are affected. `flame`, `direct` and every short-range
  form (`touch`, `rune`, …) are not.
- Missiles and lobbers: direction noise at launch, `σ` decreasing with the form's level.
- Beams: an aim sway like archery's low-stamina sway, **cumulative**: it grows the longer the
  beam is held.

- **Q-MAG-1.** Which of the proposed forms to add, rename or merge? Is `direct` a good name?
  - **A:**
- **Q-MAG-2.** Spells with several effects from different sub-directions (e.g. fire + frost):
  proposal — classify by the costliest (primary) effect only.
  - **A:**
- **Q-MAG-3.** XP amount per cast: vanilla (base magicka cost, granted on vanilla conditions —
  e.g. Destruction on hit) for all three sub-skill kinds? Relation to Q-XP-1.
  - **A:**
- **Q-MAG-4.** Destruction, Illusion and Restoration have no special sections. Is there nothing
  school-specific, or just not written yet?
  - **A:**
- **Q-MAG-6.** Perk changes (Destruction, Illusion, Restoration — schools without own section): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheets [`destruction/perks.md`](../destruction/perks.md), [`illusion/perks.md`](../illusion/perks.md), [`restoration/perks.md`](../restoration/perks.md)** (all tree perks of
  this skill, with handles; also `perks-removed.md`, `perks-ordinator.md` there); see also [`refs/perk-requirements.md`](../../refs/perk-requirements.md) §5. Answer separately for each skill: Destruction, Illusion, Restoration — schools without own section.
  - **A:**
