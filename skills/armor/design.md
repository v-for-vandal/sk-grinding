# Armor (Heavy Armor, Light Armor)

> Part of the design: [`design-v2.md`](../../design-v2.md) (was §7.2). Bare `§N` references point to design-v2.md. Index of skills: [`skills/README.md`](../README.md).

**Sub-skills:**

| Parent | Sub-skills |
|---|---|
| Heavy Armor | `heavy-armor`, one per heavy material (`iron-armor`, `steel-armor`, `dwarven-armor`, `orcish-armor`, `ebony-armor`, `daedric-armor`, `dragonplate-armor`, + Anniversary Edition and modded materials, + `heavy-other`) |
| Light Armor | `light-armor`, `clothing`, one per light material (`leather-armor`, `hide-armor`, `elven-armor`, `glass-armor`, `scaled-armor`, `dragonscale-armor`, + AE and modded, + `light-other`) |

- `clothing` is a sub-skill of Light Armor, so it counts towards the Light Armor level (as LoreRim
  gives clothing XP to Light Armor).
- Material is determined by `ArmorMaterial*` keywords; for modded armor, research which keywords
  are used (R-5); fallback to `heavy-other`/`light-other`.
- Base XP per hit taken is the vanilla (Lorerim) amount, subject to anti-grind rules (§11).

**Slot weights** (configurable):

| Slot | Weight |
|---|---|
| Body | 0.70 |
| Head | 0.20 |
| Feet | 0.05 |
| Hands | 0.05 |

Rings, amulets and shields are not counted (shields belong to Block).

**Mixed sets (explicit rule):** for each hit taken, for every counted slot:
1. Determine the item's armor type (heavy / light / clothing) and material. An **empty slot
   counts as clothing** (decided, Q-ARM-1), without a material.
2. Grant `baseXP × slotWeight` to the type sub-skill (`heavy-armor`, `light-armor` or `clothing`).
3. Grant `baseXP × slotWeight` to the material sub-skill (if any).

Example: heavy ebony cuirass + light leather helmet + nothing else, base XP 10 →
`heavy-armor` +7, `ebony-armor` +7, `light-armor` +2, `leather-armor` +2, `clothing` +1 (empty feet
and hands).

- **Main skill:** both the type and the material sub-skill count in the main skill formula, each
  with its own curve `M_i` (§4.4.1; Q-XP-1, option (a), decided), normalized per main skill
  (§4.4.5). This answers Q-ARM-2.

**Questions:**

- **Q-ARM-3.** Anniversary Edition and modded materials: own sub-skill per material, or group
  them (e.g. "Nordic", "Stalhrim", "Chitin" own; minor modded sets → `*-other`)?
  Depends on research R-5.
  - **A:**
- **Q-ARM-4.** Perk changes (Heavy Armor, Light Armor): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheets [`heavy-armor.perks.md`](heavy-armor.perks.md), [`light-armor.perks.md`](light-armor.perks.md)** (all tree perks of
  this skill, with handles; also `*.perks-removed.md`, `*.perks-ordinator.md` here); see also [`refs/perk-requirements.md`](../../refs/perk-requirements.md) §5. Answer separately for each skill: Heavy Armor, Light Armor.
  - **A:**
