# Weapon effects: attack stamina cost and attack speed

- **Status:** proposed (design decided 2026-11-15; mechanism waits for R-37 checks 1, 2, 4).
- **Design:** [`skills/weapons/design.md`](../skills/weapons/design.md) *Continuous effects*
  (non-damage effects); curve design-v2 §5.7. Weapon damage and glancing hits are a separate
  block (the hit hook of Q-DYN-9, [`weapon-class.md`](weapon-class.md)).
- **Depends on:** [*weapon-class*](weapon-class.md) (`SPO_WeapClass_*` of the weapon in each
  hand → sub-skill); [*subskill-state*](subskill-state.md) (effective level incl. the
  `dual-wield` / `with-shield` / `with-magic` factors).
- **Research:** [R-37](../research/R-37-melee-non-damage-levers.md),
  [R-34](../research/R-34-requiem-armor-movement-and-hits.md) (power attack hook),
  [R-36](../research/R-36-mod-armor-rating.md) check 1 (live "Multiply 1 + Actor Value Mult").

## What it does

For the player only, with a melee weapon (or fists, `1h-unarmed`) in the attacking hand:

1. **Light attack stamina cost** × `(1 + e_stam(L))`, `L` = effective level of the sub-skill of
   the attacking weapon; `e_stam` = −(curve pen 50, P 50, M 40, h 150): +50 % at 0, −10 % at
   100, −20 % at 200.
2. **Power attack stamina cost** × `(1 + e_stam(L))`, the same curve.
3. **Attack speed** (only if R-37 check 4 passes): `WeaponSpeedMult` (right) /
   `LeftWeaponSpeedMult` (left) get `+e_speed(L)` of the weapon in that hand (placeholder curve
   pen 15, P 50, M 20, h 150).

## Mechanism

- **Light attacks (R-37 check 1 decides):** Requiem already charges normal attacks.
  - If its cost goes through the attack-cost call hooked in (2), multiply there too.
  - If it is a Requiem spell or ability that damages stamina while attacking: scale its
    magnitude for the player with a perk entry on that spell and a factor from our AV, as armor
    *stamina on hit* does ([`armor-effects.md`](armor-effects.md) section 3); which entry point
    applies to that record is part of check 1.
  - Fallback: on `weaponSwing` / `weaponLeftSwing` damage stamina by `e_stam × Requiem's cost`
    (needs Requiem's formula, check 1).
- **Power attacks:** preferred the hook at `RELOCATION_ID(37650, 38603)` +0x16E / +0x171
  (`float(ActorValueOwner*, BGSAttackData*)`, the call Valhalla Combat replaces): call the
  original, multiply for the player. The hook knows the attack data (hand), so dual wielding two
  classes is handled. Alternative: our perk on entry point 27 *Mod Power Attack Stamina* with
  "Multiply 1 + Actor Value Mult", one entry per weapon class keyword, AV written by the DLL.
  Conflict: Valhalla Combat uses the same call (not in LoreRim per R-37; chain the original).
- **Attack speed:** DLL-tracked AV deltas (as SpeedMult in armor effects): apply
  `Δ = new − last applied` to the two AVs on equip change, on effective-level change (≥ 0.5) and
  on load; revert on unequip. Note the AV default 0 = 1.0 quirk (R-37 §4): read the value
  before the first change.

## Config

`[weapon_effects]` in `SkillPractice.toml`: `stamina_enabled`, `speed_enabled`,
`speed_refresh_delta = 0.5`; curves in the skill files' `[[effect]]` rows (already there for
the panel: *Light attack stamina cost*, *Power attack stamina cost*, *Attack speed*).

## Test

- Sword at 0 and at 100 (debug *Set current*): stamina loss of one normal attack and of one power
  attack differs by the factor 1.5 / 0.9 against the unmodified cost (log both).
- Dual wield sword + mace with different levels: each hand's power attack uses its own level.
- Attack speed: swing duration changes after *Set current* without re-equipping (R-37 check 4);
  unequip restores the AV exactly.
- NPCs with the same weapons: unchanged.
