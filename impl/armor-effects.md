# Armor effects: armor rating, stamina lost on hit, movement speed

- **Status:** proposed (variants per effect below; checks first).
- **Design:** [`skills/armor/design.md`](../skills/armor/design.md), *Continuous effects* (curves,
  which sub-skills); the shared curve design-v2 §5.7.
- **Depends on:** [`subskill-state.md`](subskill-state.md), *effect-curve* (planned), the casting
  penalty (*Casting in armor*, effective level), [`plugin-foundation.md`](plugin-foundation.md)
  (ESL records: a perk, actor values).
- **Research:** [R-36](../research/R-36-mod-armor-rating.md) (*Mod Armor Rating*), [R-34](../research/R-34-requiem-armor-movement-and-hits.md) checks 4, 5;
  [R-33](../research/R-33-requiem-mass-effect.md) (speed refresh, Requiem's own speed penalty).
  Levels: the type sub-skill of each piece (`heavy-armor` / `light-armor`) and, for armor
  rating, its material sub-skill; clothing and empty slots add nothing to stamina on hit.

## What it does

Player only. Recomputed on equip / unequip, on a change of a used sub-skill's effective level
(XP, decay, a spell equipped or unequipped for the casting penalty), on load. Throttled (at most
once a second; the effects are slow).

## 1. Armor rating

- **Variant A (preferred): our perk with entry point *Mod Armor Rating*.** Two entries, one with
  the item condition `ArmorHeavy`, one with `ArmorLight` (not shields), function *Multiply 1 +
  Actor Value Mult* on a spare actor value the DLL writes (`effect` as a fraction). One AV per
  type. **Material factor** (design: material armor rating by tier): one more entry per material
  keyword (condition: the item has the keyword), each with its own AV, because each material
  has its own level (the tier only picks the curve). Generated in the ESL from the tier
  worksheet; the material keywords are the armor-set keywords (Q-ARM-3).
- **If spare vanilla AVs run out:** *Actor Value Generator* (Nexus 84743) adds new actor values;
  it is in LoreRim (`ActorValueGenerator.dll` 2.2.9.4, research dump 2026-11-11), so it costs no
  new dependency. Check before relying on it: a generated AV works as the argument of an entry
  point function (*Multiply 1 + Actor Value Mult*) and in conditions, and how our DLL reads and
  writes it. Variant B (a hook) only if that fails.
- **Clothing** (flat points, `pen 0, P 50, M 200, h 150`; worn clothing pieces only, empty slots
  0): **a `DamageResist` delta the DLL computes**, `Σ slotWeight × bonus(L_clothing)`, applied
  and tracked like the speed delta (section 3). Independent of the items' own rating. Rejected:
  a *Mod Armor Rating* "add value" entry on clothing items (unverified that the engine counts
  clothing pieces in the armor rating at all; [R-36](../research/R-36-mod-armor-rating.md) check 3).
- **Variant B: hook the armor rating calculation** (the function that sums worn armor ratings
  with the entry point) and scale per piece in code. Fallback only; needs reverse engineering.
- Check: the CK function name and that the multiplier is read live (change the AV by console,
  the inventory rating changes): [R-36](../research/R-36-mod-armor-rating.md) checks 1, 2, 5.

## 2. Stamina lost on an unblocked hit

- Requiem casts `StaminaDamage[type]` on the player in `REQ_PlayerImpactEffects` (R-34).
- **Variant A (preferred): our perk with entry point *Mod Incoming Spell Magnitude*,** condition
  "spell is one of `StaminaDamage[*]`" (a form list in our ESL), *Multiply 1 + Actor Value Mult*
  on a spare AV = `Σ_slots slotWeight × effect(L_type)` over heavy / light pieces (clothing and
  empty slots 0). R-34 check 5: does it apply to a spell the
  script casts with `Cast` on the player?
- **Variant B:** hook the stamina damage of those magic effects (the active effect's magnitude
  when it is applied). Fallback if A does not work.

## 3. Movement speed

- Our own `SpeedMult` change (R-33 lever 2): the DLL keeps the value it applied and applies only
  the difference, `Σ_slots slotWeight × effect(L_movement)` × 100 points (clothing and empty
  slots 0). Through a hidden ability with a *ValueModifier SpeedMult* effect whose magnitude is
  updated, or a temporary AV modifier; both work as a delta, so Requiem's Mass Effect (which also
  writes deltas) is not disturbed.
- **Speed refresh:** a `SpeedMult` change takes effect only after the engine refreshes movement
  speed. Requiem relies on Bug Fixes SSE for this; otherwise nudge `CarryWeight` (+0.01, then
  back). Check which one LoreRim needs.
- On uninstall / revert: apply the negative of the stored delta.

## Config

Curves in the armor skill file (`[[effect]]` rows, as in [`status-ui.md`](status-ui.md)):
armor rating `pen 30, P 50, M 40, h 150`; stamina on hit the same with `M 40`, `pen 50`,
`invert`; speed `pen 50, P 50, M 37.5, h 75`. MCM toggles per effect (debug).

## Test

- Equip one heavy piece at `heavy-armor` 0 / 50 / 100 (set by the debug panel): the piece's rating
  is −30 % / 0 / +10 % of the value without our perk.
- Take unblocked hits at `heavy-armor` 0 and 100 in a full heavy set: stamina lost per hit is
  +50 % / −10 % of the loss with the effect off (MCM toggle).
- Full heavy set, `heavy-armor-movement` 0: `SpeedMult` lower by 50 points than with the effect
  off; level 100: +15. A cuirass alone at 0: −35. Unequip all: back to the value without us.
- Full clothing set (robe, hood, shoes, gloves), `clothing` 50 / 100 / 200: armor rating +0 / +50 /
  +100 over the value with the effect off; robe alone at 100: +35; nothing worn: +0.
