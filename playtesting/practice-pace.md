# Practice pace: raw XP per active hour of normal play

- **Status:** ready (prototype 1 logs raw XP and active hours).
- **Why:** `P_skill` (raw XP per active hour of normal play with a skill, **Q-ML-7**: how fast
  rust is removed) and `P_sub` (the same per sub-skill, **Q-DYN-15**: how fast the idle meter
  falls) are placeholders. LoreRim has `UseMult = 0`, so only our own log shows the pace.
- **Needs:** the LoreRim profile, prototype ≥ 0.1.4, the measured Requiem preset
  ([`requiem-xp-scale.md`](requiem-xp-scale.md)) or factors noted, `debug.xp_mult = 1`,
  verbose XP log on. Note the active-hours counter at start and end.
- **Steps:** play normally for 2–3 active hours per style: (a) a melee dungeon with sword and
  shield; (b) a mage dungeon with Destruction and Restoration; (c) a crafting session
  (smithing a set). Do not grind; play as you would.
- **Record:** active hours (panel summary line), the sum of raw XP per main skill and per
  sub-skill (from the log; a dump script may help), number of fights.
- **Expected / pass:** per skill and per sub-skill a raw XP per active hour; set `pace_skill`,
  `pace_sub` in `config/skills/*.toml` and check in the panel that normal play keeps `R` and
  `U` near 0 while a skill left alone for a day starts to rust.
- **Results:**
