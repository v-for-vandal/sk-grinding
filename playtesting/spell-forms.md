# Spell forms of beam-looking spells (Sunbeam)

- **Status:** ready (the *Debug* tab shows `projectile=` for the equipped spell).
- **Why:** Requiem's Sunbeam was classified as `flame`, though it looks like a beam. The likely
  cause is a flamethrower-type projectile with beam art
  ([`impl/spell-classifier.md`](../impl/spell-classifier.md)). The fix (overrides, maybe a
  range rule) waits for this: skills/magic **Q-MAG-9**, R-32.
- **Needs:** LoreRim profile, prototype 1, debug mode on; spells Sunbeam and other concentration
  spells of Destruction / Restoration (Sparks, Lightning Bolt, Flames, Frostbite, modded
  beams).
- **Steps:** equip each spell, open the panel, *Debug* tab; also *Dump player spells*.
- **Record:** per spell: form, `projectile=` type, how it looks and behaves (continuous ray
  reaching far / spray with short range / hits several targets).
- **Expected / pass:** Sunbeam shows `projectile=flamethrower` (or cone). List every spell whose
  look and engine type disagree; that list fills the override table (Q-MAG-9), and if there are
  many, note their ranges for a range rule.
- **Results:**
