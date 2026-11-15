# R-33: Requiem's armor weight penalty ("Mass Effect") and how we can change it

- **For:** [armor](../skills/armor/design.md) *Continuous effects* (decided: movement speed is our own `SpeedMult`
  change, lever 2 below); the casting penalty in armor (*Casting in armor*).
- **Status:** web research done 2026-11-15 from Requiem's source
  (<https://github.com/ProbablyManuel/requiem>, script read at tag `v5.4.5` and on `main` 6.x)
  and its changelog. Nexus returned 403; the LoreRim wiki did not resolve. Record checks in
  xEdit on the real load order are open (below).

## Findings

**Which Requiem.** LoreRim 5.0.2 (Load Order Library, 2026-06-02) loads `Requiem.esp`,
`Requiem - Weapons and Armor Redone.esp` (WAR, NoxCrab), `Requiem Lite.esp`, `Big Tweaks.esp`,
"Requiem - Exhaustion and Stamina Fix", "Requiem - Resist and Regen Tweak" and Fozar's
Dragonborn patch. Requiem 6.0.0 absorbed Fozar's patch, so LoreRim is most likely on **5.4.5**
*(inferred)*. The facts below are for 5.4.5 unless marked.

**Implementation (confirmed from source).** One Papyrus script, `REQ_MassEffectArmor`
(`components/papyrus-scripts/src/gamemechanics/coreScripts/`), an active effect of the ability
`REQ_Ability_MassEffect`. No DLL, no game setting.

```
pieces  = worn armor in slots 32 body, 31 head, 33 hands, 37 feet, 39 shield (duplicates removed)
factor  = 1 − clamp(HeavyArmorMod, 0, 100)/100   for heavy pieces
          1 − clamp(LightArmorMod, 0, 100)/100   for light pieces and clothing
penalty = Σ factor(piece) · piece.GetWeight()/100
mass    = Σ piece.GetWeight()/100
applied as deltas against the last applied values (ActiveMass, ActivePenalty):
  Mass += Δmass; Infamy += Δpenalty; MovementNoiseMult += Δpenalty; SpeedMult += −50·Δpenalty
on effect finish: everything reverted to 0
```

- **Movement speed:** −0.5 SpeedMult per effective weight unit. For example, 60 weight of
  heavy armor with no perks gives −30. There is no cap.
- **Weapons** stopped counting in 5.2.0. Power attack stamina cost became +25 % of the weapon's
  weight (5.4.2).
- **`Infamy` is the "effective mass" store** that other records read; its base is 1.0 in 5.x
  (0.0 from 6.0.0). Changelog 5.2.0: running reduces magicka and stamina regeneration up to
  100 %, scaling with this penalty. Changelog 6.0.0: jump stamina cost scales with effective
  mass. The exact formulas are in Requiem.esp records *(not read)*.
- **Weight:** base `GetWeight()`. The *Mod Armor Weight* perk entry point (perks "armor weight
  set to 0 / −50 % / −15 %" in our worksheets) most likely changes carry weight only, not this
  penalty *(inferred)*.
- **Reduction AVs** `HeavyArmorMod` / `LightArmorMod` are vanilla AVs repurposed as a percentage
  reduction (xEdit: `HeavyArmorModifier` / `LightArmorModifier`). Perk abilities add to them:
  Heavy Armor Conditioning +35, then four perks with +10 each, 75 in total; Light Armor Agility
  +50 (worksheets).
- **Recalculation:** only on `OnEffectStart` / `OnEffectFinish`, armor equip / unequip, and the
  player closing `StatsMenu`. There is no timer. A change of the reduction AVs has no effect until
  one of these happens.
- Changelog 5.2.2: "Armor weight penalty reduction from perks does not reduce damage taken". The
  engine may read these AVs in its damage calculation *(inferred; check)*.

**Spell cost in armor (confirmed from changelog 5.2.0).** It no longer depends on weight.
Untrained casting costs, by spell tier Novice → Master:
- light armor: +10/20/30/40/50 %;
- heavy armor: +20/40/60/80/100 %.

The Combat Casting / Agile Spellcasting perks remove it tier by tier, most likely as a spell
cost entry point in the esp *(unconfirmed)*. **WAR may change this** *(secondary, search
snippet of Nexus article 5697)*: Battle Mage "halves the spell cost penalty from worn armor
weight", armor paddings raise the weight penalty.

## Levers for the plugin

1. **Add our share to `HeavyArmorMod` / `LightArmorMod`.** This is the cleanest lever, because
   Requiem already reads these values. Use a permanent modifier, or a hidden ability whose
   magnitude we update.
   - **It can only reduce the penalty.** The value is clamped to 0–100, and perks already give
     up to 75, so the headroom is 25 unless we reduce the perks' values.
   - There is one value for heavy armor and one for light armor plus clothing, so it cannot
     differ by material.
   - **A recalculation must be forced** after a change, and throttled. Two untested options:
     call `UpdateMassEffect` on the actor's `REQ_MassEffectArmor` instance through the Papyrus
     VM, or remove and re-add the ability (its finish event reverts the deltas cleanly).
   - Check the 5.2.2 damage side effect first.
2. **Our own delta on `SpeedMult` / `Infamy` / `MovementNoiseMult`.** Requiem works with
   deltas against its own last values, so an extra delta does not conflict with it.
   - **This can also raise the penalty**, e.g. a low-level penalty below the checkpoint (§5.7).
   - We have to compute the worn weight ourselves, with the same five slots and weight classes.
   - A SpeedMult change needs a movement speed refresh. Requiem relies on Bug Fixes SSE; the
     common trick is to nudge CarryWeight.
3. **Not levers:** the *Mod Armor Weight* entry point and game settings (5.0.2 disabled the
   vanilla running penalty in favor of the Mass Effect).

## Open (needs xEdit on the LoreRim load order)

- The exact Requiem version, and what Big Tweaks.esp, WAR and Requiem Lite change in the Mass
  Effect, in the reduction perks and in the spell cost penalty.
- Which records read `Infamy`, and their formulas: running regen, stamina drain, attack cost,
  jump cost.
- What the engine does with `HeavyArmorMod` / `LightArmorMod`, given the 5.2.2 fix.
- Whether NPCs get `REQ_Ability_MassEffect`.

## Sources

- `REQ_MassEffectArmor.psc` at
  <https://raw.githubusercontent.com/ProbablyManuel/requiem/v5.4.5/components/papyrus-scripts/src/gamemechanics/coreScripts/REQ_MassEffectArmor.psc>
  and on `main`
- Changelog: <https://github.com/ProbablyManuel/requiem/wiki/Changelogs>,
  <https://github.com/ProbablyManuel/requiem/blob/main/components/documentation/src/Changelog.md>
- AV indices: <https://en.uesp.net/wiki/Skyrim_Mod:Actor_Value_Indices>
- LoreRim plugins: <https://api.loadorderlibrary.com/v1/lists/lorerim>
- Secondary: <https://www.nexusmods.com/skyrimspecialedition/articles/5697> (WAR, snippet only)
