# R-36: Entry point *Mod Armor Rating*: live multiplier, clothing pieces

- **For:** [`impl/armor-effects.md`](../impl/armor-effects.md) section 1 (armor rating of the type
  and material sub-skills) and the `clothing` bonus ([armor](../skills/armor/design.md)
  *Continuous effects*).
- **Status:** new (runtime checks; no web research done).

## Why

- The type and material effects (`rating × (1 + effect)`) are planned as entries of our perk on
  *Mod Armor Rating* with *Multiply 1 + Actor Value Mult*, the AV written by the DLL.
- The `clothing` bonus is in flat points. It is implemented as a `DamageResist` delta (decided).
  The rejected alternative: a *Mod Armor Rating* entry with "add value" on clothing items. It is
  kept here in case the delta turns out worse (e.g. it shows wrongly in the inventory, or other
  mods reset `DamageResist`).

## Checks

| # | Question | How | Decides |
|---|---|---|---|
| 1 | Is the entry point function *Multiply 1 + Actor Value Mult* available for *Mod Armor Rating*, and is the AV read live (change it by console → the inventory and the real `DamageResist` change without re-equipping)? | test perk in an ESL; `setav` the AV; read the inventory rating and `getav DamageResist` | variant A of section 1 (otherwise a re-equip / refresh, or the hook, variant B) |
| 2 | Does the engine apply *Mod Armor Rating* per item, so that item conditions (`ArmorHeavy`, `ArmorLight`, a material keyword) select the pieces? | two entries with different keyword conditions and multipliers; mixed set | per-type and per-material entries |
| 3 | Does the armor rating sum include **clothing** pieces at all (weight class clothing, base rating 0)? With an "add value" entry conditioned on `ArmorClothing`, does a robe add the value to `DamageResist`? | test entry `+50` on clothing; wear a robe; compare `getav DamageResist` with and without | whether the rejected clothing variant could work |
| 4 | Generated AVs (*Actor Value Generator*, in LoreRim) as the function's AV argument | same as check 1 with a generated AV | the fallback when spare vanilla AVs run out |
| 5 | Interaction with the vanilla skill scaling of armor rating (does the entry point apply before or after `fArmorScalingFactor` / the skill factor?) | compare a piece's rating at two main skill levels with the entry on and off | whether our `+10 %` is of the base or of the scaled value |
