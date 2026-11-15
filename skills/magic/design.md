# Magic: general structure (all schools)

> Part of the design: [`design-v2.md`](../../design-v2.md) (was §8.1). Bare `§N` references point to design-v2.md. Index of skills: [`skills/README.md`](../README.md).

For every spell cast that grants XP, the spell feeds three kinds of sub-skills:

1. **Form** — gameplay shape of the spell (combination of casting type, delivery and projectile
   type). **Shared** by all schools: `form/missile`, `form/beam`, ….
2. **Archetype** — grouping by mod keywords (`druidic`, `shamanic`, …); spells without an
   archetype use `standard`. **Shared** by all schools: `archetype/standard`, …. Actual list
   from Lorerim research (R-7).
3. **School sub-direction** — e.g. `destruction/fire`, `destruction/frost`, `destruction/shock`,
   `restoration/healing`, `restoration/sun`, `restoration/poison`, … from keywords and/or
   Lorerim perk tree structure (R-8). Fallback: `<school>/other`.

**Shared forms and archetypes** (decided 2026-11-14, was design-v2 Q-CAT-1):

- One sub-skill per form and per archetype for **all** schools, not one copy per school
  (`destruction/missile`, `restoration/missile`, … are dropped). A Destruction missile and a
  Restoration missile (Heal Other) train the same `form/missile`. This removes ~4 × 15
  sub-skills and matches the idea of a form: the casting technique, not the school.
- They are **shared sub-skills** (design-v2 §3): no parent main skill and `k = 0`, like the
  concept sub-skills. A school's main level is computed from its **sub-directions only**
  (§4.4.1); the `form` and `archetype` budgets of the skill files go to the sub-directions.
- What they are for: accuracy (below), their own perks and continuous effects (Q-DYN-7).
- Own dynamics (decay, idle meter, zones, floor) as for any sub-skill. No rust: rust belongs to a
  main skill (§4.4.3).
- The Alteration rule (raises the effective floor of all magic sub-skills) covers them too.
- Start values ([§4.4.4](../../design-v2.md); not in any skill's start split): forms
  `max(0, 2 × (best school − 15))`, archetypes 0.
- UI: a pseudo-skill *Magic* in the status panel ([`impl/status-ui.md`](../../impl/status-ui.md)):
  a block like a main skill, but without a level and without the next-level bar.
- **Q-MAG-8.** Confirm `k = 0` for shared sub-skills. Alternative: a shared sub-skill counts
  towards each school by the share of its raw XP that came from that school's casts
  (`f_s`, stored per form and school): school `s` gets `f_s × M(L)`. It keeps the form budget in
  the main level and stays level-based (no rotation exploit), but costs per-school shares in the
  save, a per-school rust term for one sub-skill, and a less obvious main level. Proposal: `k = 0`.
  - **A:** Agree with k=0

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

- **Q-MAG-9.** Some mods draw a beam with a **flamethrower**-type projectile (Requiem's
  Sunbeam), so the engine type says `flame` while the player sees a beam
  ([`impl/spell-classifier.md`](../../impl/spell-classifier.md)). The form is the *gameplay*
  shape, so such spells should count as `beam` (beam sway included). Proposal: an override list
  in config (spell / effect / projectile → form), filled from the PROJ dump (R-32), and a range
  rule if there are many. Waits for
  [`playtesting/spell-forms.md`](../../playtesting/spell-forms.md).
  - **A:**
- **Q-MAG-2.** Spells with several effects from different sub-directions (e.g. fire + frost):
  proposal — classify by the costliest (primary) effect only.
  - **A:**
- **Q-MAG-3.** XP amount per cast: vanilla (base magicka cost, granted on vanilla conditions —
  e.g. Destruction on hit) for all three sub-skill kinds? Relation to Q-XP-1.
  - **A:**
