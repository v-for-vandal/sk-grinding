# Dragon fight: budgets and XP in a big LoreRim fight

- **Status:** waiting (needs the per-target budgets and target worth, design-v2 §11.2).
- **Why:** LoreRim dragons have 8000+ health and are level 85+: dangerous, but a normal
  character beats them. The anti-grind rules must not cut XP in such a real fight, and the
  huge numbers must not turn one fight into many levels. Checks the offense budget
  (`kOff = 1.75 × max health`), the defense budget (`S` = attacker max health × target-worth
  level ratio) and the upper clamp of the target-worth multiplier.
- **Needs:** LoreRim profile, measured Requiem preset
  ([`requiem-xp-scale.md`](requiem-xp-scale.md)), budget inspector (§13), verbose XP log; a
  low-level (≈ 15–20) and a high-level (≈ 50) character.
- **Steps:** fight a dragon in melee (sword + shield, block its attacks) and once as a mage;
  repeat with both characters.
- **Record:** XP per sub-skill and main level before / after; budget `spent / B` and falloff `m`
  during the fight (inspector); target-worth multiplier; fight length; how hard it felt.
- **Expected / pass:** `m = 1` for the whole fight (budgets never reached); one fight gives a
  noticeable but not absurd gain (rule of thumb: ≤ 1–2 main levels for the low character, less
  for the high one); the target-worth multiplier stays at its clamp. Otherwise: change `kOff`,
  `kDef`, the target-worth curve or its clamp.
- **Results:**
