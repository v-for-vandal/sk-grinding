# Skill books: points with and without the Skilled trait

- **Status:** waiting (needs the `TESObjectBOOK::Read` hook, design-v2 §10.1).
- **Why:** books give `bookPotential × n`, where `n` is the engine's points from the perk entry
  point *Adjust Book Skill Points* (vanilla 1). The LoreRim trait *Skilled* (Biggie Traits)
  should give `n = 2`; script bonuses (Wintersun Xarxes) should arrive once as external
  increments, not twice ([R-30](../research/R-30-book-trait.md)).
- **Needs:** LoreRim profile; three characters or saves: no book trait, *Skilled*, Xarxes
  follower; unread skill books of a managed skill (e.g. Block, One-Handed).
- **Steps:** read one unread book with each character; read the same book again; read two books
  of the same skill in quick succession.
- **Record:** `n` logged by the hook, potential before / after for every sub-skill of that
  skill, external increments logged, any vanilla "skill increased" popup.
- **Expected / pass:** no trait `+2`, Skilled `+4` per sub-skill; Xarxes adds its points once
  (as external increment → potential); the second read gives nothing; the main skill does not
  change; no vanilla popup.
- **Results:**
