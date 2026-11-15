# R-35: Export armor rating and weight of armor records

- **For:** the material tiers of the armor rating effect ([armor](../skills/armor/design.md)
  *Continuous effects*, material sub-skills; worksheet
  [`refs/lorerim/armor-material-tiers.md`](../refs/lorerim/armor-material-tiers.md)). Weight also
  feeds the movement speed balance next to Requiem's Mass Effect ([R-33](R-33-requiem-mass-effect.md)).
- **Status:** new. The research dump of 2026-11-11 (`research/tables/armor-materials.tsv`) has no
  armor rating and no weight: columns are `formKey, editorID, name, winner, type, slots,
  template, keywords` only.

## Task

Re-export ARMO records from the installed LoreRim profile (the `LoreResearch` Mutagen tool,
`run-research.ps1`; not in this repository) with two more columns in `armor-materials.tsv`:

| Column | Mutagen (`IArmorGetter`) | Note |
|---|---|---|
| `armorRating` | `ArmorRating` (float) | base value of the winning record, before perks and skill |
| `weight` | `Weight` (float) | base weight (what Requiem's Mass Effect reads, R-33) |

- Winning record (as the other columns). Records with a template (`TemplateArmor`, enchanted
  variants): export their own values too; the worksheet uses base items (no template).
- No aggregation in the tool: the worksheet script groups cuirasses per keyword itself.

## After the export

- `scripts/gen_armor_material_tiers.py` adds the median (and range) of the cuirass armor rating
  per keyword to the worksheet's *Cuirass armor rating* column; the user assigns tiers.
- Do clothing records (`type = Clothing`) have a non-zero armor rating in LoreRim? Vanilla
  clothing has 0. For information only: the `clothing` bonus is in flat points and does not
  depend on it.
- Check that Requiem's armor ratings are in the base records (not rescaled at runtime by a
  script or the Reqtificator patch, R-33 notes) by comparing one cuirass with its in-game value
  at a known skill level.
