# R-34: Requiem's passive armor training and stamina damage on hit

- **For:** [armor](../skills/armor/design.md) *Movement in armor* and *Continuous effects*;
  [`impl/armor-movement-xp.md`](../impl/armor-movement-xp.md).
- **Status:** web research done 2026-11-15 from Requiem's source
  (<https://github.com/ProbablyManuel/requiem>, tag `v5.4.5`; `main` 6.x the same in these parts).
  The numbers are in `Requiem.esp` (script properties, spell magnitudes) and are open (checks
  below). LoreRim is most likely on 5.4.5 ([R-33](R-33-requiem-mass-effect.md)).

## Passive armor training (confirmed from source)

`REQ_PassiveArmorTraining.psc` (`components/papyrus-scripts/src/gamemechanics/coreScripts/`).
- `extends REQ_CoreScript`, which `extends ReferenceAlias`: a script on a player alias of a
  Requiem quest (which quest: in the esp).
- `REQ_CoreScript` sets `Player = Game.GetPlayer()` in `OnInit` and listens for the mod events
  `Requiem_ScriptInit` / `Requiem_ScriptShutDown` (→ `initScript` / `shutdownScript`).
- Properties, all `Auto` without defaults (values only in the esp): `Int[] contributions` (the
  biped slots that count), `Float[] Skillgain` (gain per interval per slot), `Float
  UpdateInterval`, `GlobalVariable ArmorDebug`, `Formlist worn`.
- **Loop:** `initScript` → `Adjust_Values()`, `RegisterForSingleUpdate(UpdateInterval)`;
  `OnUpdate` re-registers itself. Equip / unequip re-runs `Adjust_Values` through the states
  `StartEvaluation` / `EvaluateArmor`.
- **Amount** (`Adjust_Values`): for biped slots 30–61, `pos = contributions.find(slot)`;
  weight class heavy → `gain_heavy += Skillgain[pos]`, light → `gain_light += Skillgain[pos]`,
  clothing → `gain_light += Skillgain[pos] / 2`. Item weight and armor rating do not matter.
- **Grant:** `If Player.IsRunning()` → `Game.AdvanceSkill("Lightarmor", gain_light)`,
  `Game.AdvanceSkill("Heavyarmor", gain_heavy)`; `ElseIf Player.IsSprinting()` → the same × 2.
  Each only if the gain is > 0. No combat check; walking gives nothing; player only.
- **Rate:** `XP/min = 60 / UpdateInterval × Σ Skillgain` (× 2 sprinting) in vanilla use points.
  Unknown without the esp. Indirect sign that it is significant: the third-party "Requiem - Skill
  Rates Rebalance" (Nexus 102204) makes armor XP "2.5 times faster out of combat (walking and
  running exp)".
- **Possible bug:** if `IsRunning()` is also true while sprinting, the sprint branch never runs
  *(unconfirmed)*.
- MCM: only "Armor Training Debug Mode"; no toggle to disable it.

## Stamina damage on hit (confirmed from source)

`impactEffects/REQ_PlayerImpactEffects.psc`, a player alias script, `OnHit`:
- **Hit types:** weapon by skill (OneHanded 0, TwoHanded 1, Marksman 4; power attack + 2),
  hostile spells (5, concentration 6). Sources with keyword `IgnoredAttack`, non-hostile spells
  and spells while the `Blocker` effect is active are skipped.
- Every counted hit casts `MagickaDamage[type]` (Focused Mind perk check in its effects).
- **Not blocked and no ward active → `StaminaDamage[type]` is cast on the player**; blocked →
  `StaminaGain[type]` (with the right perk). Arrows and spells included (5.4.5 changelog: "Arrows
  damage stamina and magicka on hit"; "Wards block stamina damage on hit").
- Stamina < 3 → disarm (no random roll in code; MCM option "Exhaustion does not disarm").
- **Armor:** the script reads armor only for the stagger chance,
  `threshold² / (DamageResist + 100)²` (× `50 / (Block + 50)` while blocking). The stamina amounts
  and any conditions on them are in the esp spells.
- Vanilla: no stamina cost for an unblocked hit, only for blocking (`fStaminaBlock*`) *(inferred,
  no primary source)*.

## Levers

- **Passive training → our sub-skills:** replace the script file `REQ_PassiveArmorTraining.pex`
  with our own version that keeps the logic and calls our DLL instead of `Game.AdvanceSkill`
  (decided; [`impl/armor-movement-xp.md`](../impl/armor-movement-xp.md)). Rejected: an ESL override
  of the quest's script properties (does not reach running saves, copies the whole quest
  record); hooking `Game.AdvanceSkill` and reading the caller from the Papyrus stack (relies on
  VM internals).
- **Stamina damage on hit → an armor effect** (decided, armor *Continuous effects*): a perk entry point *Mod
  Incoming Spell Magnitude* conditioned on the `StaminaDamage[*]` spells, its value multiplied
  by an actor value the DLL writes from the type sub-skill. Check: does it apply to a spell the
  script casts with `Cast` on the player?

## Open (xEdit on the LoreRim load order, in game)

| # | Question | How | Decides |
|---|---|---|---|
| 1 | `UpdateInterval`, `Skillgain[]`, `contributions[]`, and the quest / alias of the script | xEdit: VMAD of the alias | the XP rate of `*-armor-movement` (config defaults) |
| 2 | Does any LoreRim mod ship its own `REQ_PassiveArmorTraining.pex`? | MO2 conflicts tab, BSA contents | the install order of our mod |
| 3 | `IsRunning()` while sprinting | in game, log both | whether our version checks sprint first |
| 4 | `StaminaDamage[0..6]` magnitudes and conditions (armor, perks) | xEdit | the size of the hit-stamina effect |
| 5 | *Mod Incoming Spell Magnitude* on these spells | test perk, log stamina per hit | whether the hit-stamina effect is possible |

## Sources

- <https://raw.githubusercontent.com/ProbablyManuel/requiem/v5.4.5/components/papyrus-scripts/src/gamemechanics/coreScripts/REQ_PassiveArmorTraining.psc>
- <https://raw.githubusercontent.com/ProbablyManuel/requiem/v5.4.5/components/papyrus-scripts/src/gamemechanics/coreScripts/REQ_CoreScript.psc>
- <https://raw.githubusercontent.com/ProbablyManuel/requiem/v5.4.5/components/papyrus-scripts/src/gamemechanics/coreScripts/impactEffects/REQ_PlayerImpactEffects.psc>
- Changelog: <https://raw.githubusercontent.com/ProbablyManuel/requiem/v5.4.5/components/documentation/src/Changelog.md>
- MCM texts: <https://raw.githubusercontent.com/ProbablyManuel/requiem/v5.4.5/components/interface/mcm-translations/Requiem_TEMPLATE.txt>
- Secondary: <https://www.nexusmods.com/skyrimspecialedition/mods/102204> (snippet only)
