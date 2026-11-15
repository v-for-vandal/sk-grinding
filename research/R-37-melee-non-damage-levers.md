# R-37: Non-damage melee levers for weapon sub-skill effects

- **For:** continuous effects of the weapon sub-skills ([weapons](../skills/weapons/design.md)
  *Continuous effects*, *Accuracy*): what can be penalized at low levels besides weapon damage.
- **Status:** web research done (2026-11-15). Checks in xEdit and in game are open (below).
- **Related:** [R-33](R-33-requiem-mass-effect.md), [R-34](R-34-requiem-armor-movement-and-hits.md)
  (power attack cost hook), [R-36](R-36-mod-armor-rating.md) (live AV multiplier on an entry
  point), [`refs/aim-perturbation.md`](../refs/aim-perturbation.md) §4.2 (melee glancing).

Many Nexus, UESP and CK wiki pages returned 403 to the researcher. Facts marked *(snippet)* come
from search-result snippets, not full pages. GitHub sources were read in full.

## Findings

### 1. Normal (light) attack stamina

- Perk entry points have **no** light-attack stamina entry. The only stamina entry is
  `kModPowerAttackStamina = 27` (CommonLibSSE-NG `RE/B/BGSEntryPoint.h`).
- The engine has a per-attack field `BGSAttackData::AttackData::staminaMult` (offset 0x30, next to
  `recoveryTime`, `staggerOffset`, `knockDown`, `damageMult`, flags `kPowerAttack` /
  `kBashAttack`; `RE/B/BGSAttackData.h`). It belongs to the race attack data, not to the actor.
- **Requiem charges stamina for normal attacks** (old manual and changelog; Wildlander wiki for a
  Requiem-based list). As described there:
  - a constant drain until the swing ends, scaled by attack duration;
  - scaled by the weapon skill (halved at grandmaster; 1/4 at 100 vs 0 per Wildlander);
  - scaled by mass / armor weight and by having the weapon's base perk; NPCs too.
  - The 5.x formula and its implementation (ability on attack state, perk, or attack data) are
    **not confirmed**. Requiem 5.2.0 reworked stamina *(snippet)*.
- LoreRim: no separate attack-stamina mod found in the LoreRim mod page *(snippet, early 2024)*.
  It lists Maxsu's Attack MCO/DXP, Chocolate Poise, Stances, Precision, SCAR 2, Combat Pathing
  Revolution; no Valhalla Combat.
- How other mods do it:
  - **Valhalla Combat** (D7ry, source read): replaces the call at
    `RELOCATION_ID(37650, 38603) + RELOCATION_OFFSET(0x16E, 0x171)` with
    `float getAttackStaminaCost(RE::ActorValueOwner*, RE::BGSAttackData*)`. Comment: "to cancel
    out vanilla power attack stamina consumption". Also hooks `RELOCATION_ID(37510, 38452)`
    (stamina regen).
  - **Attacks Use Stamina** (Nexus 132796) *(snippet)*: drains on the `HitFrame` animation event,
    hand from `weaponSwing` / `weaponLeftSwing`.
  - **Apex Combat** (CommonLibSSE-NG, README): charges once at "attack commit" after an
    affordability check; leaves power attacks to the engine.
  - **Storm of Steel** *(snippet)*: perk entry points `Apply Weapon Swing Spell` (67) / `Apply
    Combat Hit Spell` (51). Only one spell per entry point without the Scrambled Bugs patch.

### 2. Power attack stamina

- Cost = `(fStaminaAttackWeaponBase + weight × fStaminaAttackWeaponMult) ×
  fPowerAttackStaminaPenalty`, defaults 20 / 1 / 2, then entry point 27; dual wield uses the main
  hand weight, unarmed weight 0. Source: a third-party plugin README (PowerAttackStaminaReq), GMST
  names not checked. Requiem 5.4.2 adds +25 % of the weapon weight (R-33).
- The hook point `RELOCATION_ID(37650, 38603)` +0x16E / +0x171 is confirmed (Valhalla Combat). Its
  CommonLib name and whether the hooked call also runs for normal attacks are not confirmed.

### 3. Melee hit chance, misses, glancing

- No Skyrim mod with a melee to-hit / miss / glancing roll was found ("not found", not "does not
  exist"). Classic RPG Archetypes (SSE 41178) gives the *defender* a dodge chance *(snippet)*.
- **Precision API** (header `src/PrecisionAPI.h`, main branch, interface V1–V4, read in full):
  - `AddPreHitCallback(handle, PreHitCallback)`, `PreHitCallback =
    std::function<PreHitCallbackReturn(const PrecisionHitData&)>`;
  - `PreHitCallbackReturn { bool bIgnoreHit; std::vector<PreHitModifier> modifiers; }`;
  - `PreHitModifier { ModifierType (Damage, Stagger); ModifierOperation (Additive,
    Multiplicative); float modifierValue; }`;
  - `PrecisionHitData`: attacker, target, hit / hitting rigid bodies, hit position, normal,
    velocity, shape keys;
  - also post-hit, pre-physics-step, collision filter, weapon–weapon and weapon–projectile
    callbacks, `GetAttackCollisionCapsuleLength`.
  - So a pre-hit callback **can cancel a hit and scale its damage and stagger**. One callback per
    plugin handle *(secondary)*. Which interface version Precision 2.0.4 (LoreRim) exposes is not
    confirmed (latest 2.0.6 / V4 *(snippet)*).

### 4. Other levers

| Lever | Mechanism | Per actor | Weapon keyword | Live |
|---|---|---|---|---|
| Power attack stamina | entry point 27 or the 37650/38603 hook | yes | entry point condition; hook sees the attack data | next attack *(inferred)* |
| Light attack stamina | Requiem's own system; or our drain on `weaponSwing` / `HitFrame`, or the hook if it runs for light attacks | yes | DLL | yes |
| Attack speed | AVs `WeaponSpeedMult` (85), `LeftWeaponSpeedMult` (132); also bow draw | yes | no: DLL sets it on equip / per hand | not confirmed (reports of needing a weapon switch) |
| Stagger dealt | entry point `kModTargetStagger = 34` (target side 33); Precision pre-hit `Stagger` modifier | yes | yes | next hit |
| Critical chance / damage | entry points 1 / 2 | yes | likely (weapon condition tab) | next hit |
| Reach | weapon form; Precision length / radius settings per weapon or attack (no per-actor value) | no | – | – |
| Recovery / cooldown | `AttackData::recoveryTime` (race attack data); MCO animations | no | – | – |
| Swing noise | none (only detection entry points 48, 57) | – | – | – |
| Durability | none in vanilla | – | – | – |

- Attack speed gotcha: the AV default 0 counts as 1.0, and the first change from 0 misbehaves
  (+0.25 → 1.25); set 1.0 first *(snippet, Comprehensive Attack Rate Patch)*.
- Poise in LoreRim: Chocolate Poise *(snippet, early 2024)*; whether it reads
  `ModTargetStagger` is not confirmed (MaxsuPoise does).

## Checks (xEdit / in game)

| # | Question | How | Decides |
|---|---|---|---|
| 1 | How Requiem 5.4.x charges normal attacks: formula, record type (ability on attack state, perk, attack data `staminaMult`), and whether it already scales with the skill | xEdit: Requiem.esp abilities / perks with stamina damage while attacking; race attack data | a light-attack stamina penalty can scale Requiem's own cost instead of adding a new drain |
| 2 | Does the 37650/38603 call run for normal attacks too, and with which attack data | log in the hook: power vs normal, `kPowerAttack` flag | one hook for both costs |
| 3 | Precision 2.0.4 interface version; `bIgnoreHit` and the Stagger modifier work for the player's hits | request the API at `kPostPostLoad`, log the version; test callback | glancing via Precision |
| 4 | `WeaponSpeedMult` / `LeftWeaponSpeedMult` changes apply mid-combat | console `modav`, swing timing | attack speed as a continuous effect |
| 5 | Current LoreRim 5.x list: poise mod, any attack-stamina mod, what *Requiem - Modern Combat Rework* changes | LoreRim modlist / research tables `enabled-mods.tsv` | conflicts |
| 6 | `kModTargetStagger` is read by LoreRim's poise mod | xEdit / in game | stagger as a lever |

## Sources

- CommonLibSSE-NG: `include/RE/B/BGSEntryPoint.h`, `include/RE/B/BGSAttackData.h` (main).
- Valhalla Combat: https://github.com/D7ry/valhallaCombat (`src/bin/Hooks.cpp`, `src/include/Hooks.h`).
- Precision API: https://raw.githubusercontent.com/ersh1/Precision/main/src/PrecisionAPI.h; settings `src/Settings.h`.
- PowerAttackStaminaReq README: https://github.com/zhouziweishiwo-afk/PowerAttackStaminaReq
- Apex Combat: https://github.com/opteron-x86/apex-combat
- Requiem manual (old): https://www.scribd.com/document/223255466/Requiem-Manual; changelog mirror: https://silo.tips/download/requiem-the-roleplaying-overhaul
- Wildlander wiki One-Hand: https://wiki.wildlandermod.com/08-SkillAnalysis/One-Hand/
- Nexus *(snippets)*: LoreRim 112590, Attacks Use Stamina 132796, Simple Attack Stamina Cost 92475, Storm of Steel 33697, Comprehensive Attack Rate Patch 89042, Requiem Exhaustion and Stamina Tweaks 67232.
- MaxsuPoise manual: https://github.com/max-su-2019/MaxsuPoise/blob/master/docs/en/Mechanics%20Manual.md
