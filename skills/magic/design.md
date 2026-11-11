# Magic: general structure (all schools)

> Part of the design: [`design-v2.md`](../../design-v2.md) (was §8.1). Bare `§N` references point to design-v2.md. Index of skills: [`skills/README.md`](../README.md).

For every spell cast that grants XP, the spell feeds three kinds of sub-skills:

1. **Form** — gameplay shape of the spell (combination of casting type, delivery and projectile
   type). The form sub-skills are cross-school; which main skill they count towards
   (one form sub-skill per school, or no fixed parent) is open, design-v2 Q-CAT-1.
2. **Archetype** — grouping by mod keywords (`druidic`, `shamanic`, …); spells without an
   archetype use `standard`. Actual list from Lorerim research (R-7).
3. **School sub-direction** — e.g. `destruction/fire`, `destruction/frost`, `destruction/shock`,
   `restoration/healing`, `restoration/sun`, `restoration/poison`, … from keywords and/or
   Lorerim perk tree structure (R-8). Fallback: `<school>/other`.

No cross-links between magic sub-skills, except the Alteration rule ([alteration](../alteration/design.md)).

**Forms** (decided, Q-MAG-1). A form is the gameplay shape of the spell:

| Form | What goes there |
|---|---|
| `missile` | single projectiles, **including** arcing ones (lobbers) and Heal Other |
| `explosion` | projectiles that explode on impact (Fireball); was "area" in the proposal |
| `beam` | beams |
| `flame` | sprays, **including** cones |
| `rune` | runes |
| `wall` | walls |
| `cloak` | cloaks |
| `direct` | spells applied straight to the target actor (was "target-actor") |
| `self` | self buffs (Oakflesh, Muffle, armor / resist spells), **including** self heals |
| `touch` | touch-delivered spells |

Not forms:

- **Wards** are a Restoration sub-direction `restoration/wards`, not a form
  ([restoration](../restoration/design.md)).
- **Summons** have no form. A summon spell of another school feeds its own school's
  sub-direction and archetype as usual; the form share goes to a Conjuration sub-skill
  (`conjure-other` if unclear), see [conjuration](../conjuration/design.md).
- **Dual casting** is ignored for now: a dual-cast spell counts as an ordinary cast.
- **Scrolls** are ignored for now: no form, no sub-skill, no XP.

**School-specific design:** [destruction](../destruction/design.md),
[illusion](../illusion/design.md), [restoration](../restoration/design.md),
[alteration](../alteration/design.md), [conjuration](../conjuration/design.md).

**Accuracy** (decided, Q-MAG-5; mechanics in [`refs/aim-perturbation.md`](../../refs/aim-perturbation.md)):

- Only the **form** sub-skill drives inaccuracy (not the school sub-direction or archetype).
- Only `missile` (lobbers included) and `beam` are affected. `flame`, `direct` and every
  short-range form (`touch`, `rune`, …) are not; `explosion`: see Q-MAG-7.
- Missiles: direction noise at launch, `σ` decreasing with the form's level (the
  crosshair does not move, the projectile flies off it).
- Beams: an aim sway like archery's low-stamina sway, **cumulative**: it grows the longer the
  beam is held (the crosshair moves, the beam follows it).
- Implementation: [`impl/spell-aim-inaccuracy.md`](../../impl/spell-aim-inaccuracy.md) (both
  variants; beam = aim sway, missile = launch noise).
- **Q-MAG-7.** `explosion` (Fireball) is also a launched projectile. Proposal: the same launch
  noise as `missile`, driven by the `explosion` level.
  - **A:**

- **Q-MAG-2.** Spells with several effects from different sub-directions (e.g. fire + frost):
  proposal — classify by the costliest (primary) effect only.
  - **A:**
- **Q-MAG-3.** XP amount per cast: vanilla (base magicka cost, granted on vanilla conditions —
  e.g. Destruction on hit) for all three sub-skill kinds? Relation to Q-XP-1.
  - **A:**
