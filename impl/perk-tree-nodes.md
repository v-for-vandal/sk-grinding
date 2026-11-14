# Restored perks: new nodes in LoreRim perk trees

- **Status:** proposed (variant depends on R-29 G3).
- **Design:** design-v2 §5.6 (purchasable perks with sub-skill requirements); per-skill decisions
  in the worksheets, section *Restored to the LoreRim tree*:
  [Smithing](../skills/smithing/perks-ordinator.md), [Restoration](../skills/restoration/perks-ordinator.md),
  [Alteration](../skills/alteration/perks-ordinator.md). Background:
  [`skills/perks-guide.md`](../skills/perks-guide.md), *Bringing back Ordinator's own perks* (type A).
- **Depends on:** [`perk-requirements.md`](perk-requirements.md) (purchase conditions, gating,
  magnitudes), [*plugin-foundation*](plugin-foundation.md) (ESL, esp-gen).
- **Research:** [R-29](../research/R-29-restored-ordinator-perks.md) (records, scripts, runtime
  nodes).

## What it does

Ordinator's own perks (type A) are loaded in LoreRim but are not nodes of any tree. This block
makes them buyable: a node in the skill's tree under the chosen parent, purchase conditions
rewritten for the LoreRim tree, and (for some) changed effects. Only type A perks so far; type B
(copies from the original esp) is not needed by any decision yet.

| Skill | Perks (parent → child) |
|---|---|
| Smithing | SMI-04 → ORD-SMI-02 → ORD-SMI-05 → ORD-SMI-11, ORD-SMI-12; ORD-SMI-02 → ORD-SMI-08 |
| Restoration | RES-01 → ORD-RES-04 |
| Alteration | ALT-01 → ORD-ALT-06 → ORD-ALT-13 → ORD-ALT-20; ALT-24 → ORD-ALT-21; ALT-09 → ORD-ALT-17 |

## Tree nodes: variants

- **Preferred: runtime, in the DLL** at `kDataLoaded`: create `BGSSkillPerkTreeNode`s, link them
  to the parent node (`children` / `parents`), set the perk and the grid position, append them to
  the skill's `ActorValueInfo::perkTree`. No AVIF override, so no conflict with the LoreRim / Requiem
  AVIF overrides and it survives LoreRim updates. Unknown whether the perks menu accepts nodes
  added this way (R-29 G3).
- **Fallback: build time**, `tools/esp-gen` (Mutagen) copies the winning AVIF of the pinned
  LoreRim version into our ESL and adds the nodes. Our ESL must load after every plugin that
  overrides these AVIFs; it has to be regenerated when LoreRim changes them.
- Node data (parent handle, grid x / y) in config, one table per skill; positions from R-29 G4.

## Purchase conditions

Through [`perk-requirements.md`](perk-requirements.md) (runtime injection into
`BGSPerk::perkConditions`), per perk and per rank:

- `has perk <Ordinator parent>` → `has perk <LoreRim parent>` where the parent changes
  (ORD-SMI-01 → SMI-04, ORD-RES-01 → RES-01, ORD-ALT-01 → ALT-01, ORD-ALT-07 / -12 → ALT-24,
  ALT-17 → ALT-09);
- skill-level items changed as in the worksheet (Smithing chain: −20; Ritualist: 80 → 100);
- sub-skill requirements added as one global each (`dwarven >= 40`, `with-magic >= 75`, the
  Arcane Thesis k-of-n formula, …).

## Gating the effects (Q-DYN-11)

These perks work through abilities with Script effects and spells cast by those scripts, not
through entry points. The requirement global is added (as in perk-requirements item 3) to:

- the ability's effects (Spellblade `01B58C`, Rend Resistances `187D2A`, Arcane Thesis `02C047`,
  `09E346`, Spirit Tutors): the script effect stops while the requirement is not met;
- a perk without its own records (Energy Roil, the Smithing chain after ORD-SMI-02): the spells
  the scripts cast for it, e.g. effect 1 of `ORD_Alt_EnergyRoil_Spell_Proc` (`01BB32`). Config
  list `gated_spells` per perk (new key in perk-requirements).

## Changed effects

- **Magnitudes:** perk-requirements item 4 (`magnitudes`); Energy Roil `01BB32` effect 1:
  150 → 100.
- **Added conditions on a spell effect** (new key `effect_conditions_add`): Energy Roil `01BB32`
  effect 1 gets one OR group run on `PlayerRef`:
  `GetEquippedItemType Left == 1 OR … == 4 OR GetEquippedItemType Right == 1 OR … == 4`
  (one-handed weapon in the hand that does not cast). Works per target if R-29 A2 confirms it.
- **Descriptions** of changed perks: method from R-29 A3 (runtime or an ESL override of the PERK
  record).

## Config

Per skill: `[[node]]` with `perk`, `parent`, `grid_x`, `grid_y`. Per perk (perk-requirements
config): `requires_replace`, `requires_add`, `gated_spells`, `magnitudes`, `effect_conditions_add`,
`description`.

## Test

- The perks menu shows each new node under its parent, greyed out until the conditions are met; the
  condition text shows the new skill level.
- Buy ORD-SMI-02 with Smithing 20, `dwarven` 40 and SMI-04: the Autocannon recipe appears at the
  forge (R-29 S1), the power works.
- Energy Roil with a sword in the other hand: a nearby enemy loses 100 armor for 4 s; with fists,
  none; Spellblade's +20 % damage in both cases.
- Lower `with-magic` below 75 (debug panel *setCurrent*): Spellblade stops working within about a
  second.
