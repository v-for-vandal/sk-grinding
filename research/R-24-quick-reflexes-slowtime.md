# R-24. Quick Reflexes: is there already a slow-time mechanic on block?

- **Status:** new (static look at plugins, then a runtime test).
- **For:** restoring *Quick Reflexes* (Ordinator version) in the Block tree:
  [`skills/block/perks-removed.md`](../skills/block/perks-removed.md), QuickReflexes.
- **Why:** Requiem nulled the perk (`REQ_NULL_QuickReflexes`, effect
  `REQ_NULL_PerkQuickReflexesEffect`). If a LoreRim combat mod already slows time on a block or a
  parry, restoring it gives a double slow-motion. The perk is restored **only if** there is no such
  mechanic (or it does not overlap).

## Leads (from `~/tmp/research/tables/`)

- `For Honor in Skyrim.esp` 1.8.0 (+ LoreRim balance patch) has `MA_SlowTime` (`000857`) and
  `MA_SlowTimeF` (`000858`), magic effects of type SlowTime.
- `PowerShot.esp` (Bow PowerShot) has `MAG_Slow Time`: archery, probably not related.
- LoreRim's rebuilt `Ordinator - Perks of Skyrim.esp` still has `ORD_Bck_QuickReflexes_Effect_Self`
  (`07B100`) and `ORD_Bck_QuickReflexes_Spell_Self` (`07B101`): the restored perk can use them
  instead of copies.
- SKSE plugins without records: `ParryingRPG.dll` 2.0.1, `DualWieldParryingNG.dll`, Precision,
  MCO / block recovery: slow-time would be in their INI / JSON or code.

## Checks

| # | Question | How to check | Result decides |
|---|---|---|---|
| 1 | When are For Honor's `MA_SlowTime` / `MA_SlowTimeF` applied? | Find the spells and perks using these effects (spell-effects.tsv, perk entry points, scripts in the mod); read their conditions (block, parry, guard, power attack of an enemy?) | **overlap** if it fires on block / parry against attacks |
| 2 | Do the SKSE plugins slow time? | Search INI / JSON / DLL strings of Parrying RPG, Dual Wield Parrying, Precision, MCO for `slow`, `timescale`, `SlowTime`, `GlobalTimeMultiplier`, `hitstop` | overlap, or only hit-stop (short freeze on hit, does not count) |
| 3 | Runtime: block an enemy power attack (shield and weapon, normal and timed block) with the mod disabled | watch / log the time multiplier | final answer |
| 4 | Are the Ordinator records `07B100` / `07B101` unchanged in LoreRim (magnitude 0.05, no extra conditions)? | xEdit or the dump | whether the restored perk references them or ships its own copy |
| 5 | Does `PerkQuickReflexesKeyword` (`046BB3:Skyrim.esm`) still exclude spells from slowdown ("magic slowdown set to 1")? | 7 magic effects use it (keyword-usage.tsv); check that they are still the slow-time effects | whether the second entry point of the perk is needed as is |

## How to decide

- **Restore** if checks 1–3 find no slow-time on block / parry (hit-stop does not count).
- **Do not restore** if some mod already slows time on block against power attacks. Optionally
  restore with a different effect (to be designed).
