# R-25. Injecting conditions and values into existing perks from the DLL

- **Status:** new (CommonLibSSE-NG headers, then a test DLL in game).
- **For:** [`impl/perk-requirements.md`](../impl/perk-requirements.md).

## Checks

| # | Question | How to check | Result decides |
|---|---|---|---|
| 1 | Field names and layout in CommonLibSSE-NG: `BGSPerk::perkConditions`, `perkEntries`; conditions of `BGSEntryPointPerkEntry` per tab; data of entry point functions (`BGSEntryPointFunctionDataOneValue` …); `BGSAbilityPerkEntry::ability`; conditions of a spell's effects (`Effect::conditions`); how to build a `TESConditionItem` (`GetGlobalValue`, `GetIsReference`, OR flag) | read the headers; write a dump of one perk's conditions | how the code is written |
| 2 | Does an added condition in the perk owner tab of an entry point gate it at once? | test DLL: inject `GetGlobalValue == 1` into one damage entry point; toggle the global by console; compare damage | **effects gating works** for entry points |
| 3 | Is an injected condition on an ability's effect re-checked (constant effect turns off / on), and how fast? | same with an ability perk (e.g. Masterly Strike, BLK-15) | gating works for abilities; delay |
| 4 | What does the perks menu show after the DLL changes or removes a `GetBaseActorValue Block >= N` item, and with an added `GetGlobalValue` item? Does it crash? | change BLK-07 to 50, add a global to another perk, open the menu | whether the shown "Requires" level follows the condition; Q-DYN-12 |
| 5 | NPC side: with `GetIsReference Player == 0 OR …`, does a Requiem NPC with the perk keep the effect while the player's global is 0? | spawn an NPC known to have the perk; compare with the global at 0 and 1 | the condition form is right |
| 6 | Magnitude edits at `kDataLoaded`: do they reach an ability already on the player from an old save? | change an ability magnitude, load a save made before | whether the DLL must re-add abilities on load |
| 7 | Are the injected conditions kept for the whole session (no reload of records, e.g. after `coc` or a new game)? | inject, play, change cell, start a new game, check again | whether to re-inject on events |

## How to decide

- Checks 1, 2, 4 pass → Requires and entry-point gating as designed.
- Check 3 fails → abilities: the DLL removes / re-adds the ability spell on the player itself
  (driven by the same global), instead of a condition on its effects.
- Check 4 shows the old level or nothing → show requirements in our panel (Q-DYN-12 b).
- Check 6 fails → on game load the DLL removes and re-adds the affected abilities on the player.
