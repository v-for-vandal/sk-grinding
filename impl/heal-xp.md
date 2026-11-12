# Healing XP: damage-source filter, multipliers, budgets

- **Status:** chosen (design decided); the damage-source filter has a fallback.
- **Design:** [`skills/restoration/design.md`](../skills/restoration/design.md), *XP* (Q-RES-2,
  Q-RES-5, Q-RES-6, Q-RES-8).
- **Depends on:** *subskill-state*, *player-hit-hook* (extended to hits on any actor, see below),
  *effect-curve*, the per-target budget ([`refs/per-target-budget.md`](../refs/per-target-budget.md)).
- **Research:** none yet; the check is listed in the test plan.

## What it does

Computes the XP of a player healing spell (sub-skill `restoration/help`, plus form and
archetype) from the health it restored:

```
raw      = restored / (1 + effect(L_help))          // amount without our bonus (Q-RES-8)
if target == player:
    xp = raw × (inCombat ? 1 : 0.1) × DR(spell)      // DR: diminishing repetition, §11.2 item 4
else:
    counted = min(raw, pool[target]); pool[target] −= counted
    xp = counted × (inCombat ? 1 : 0.1) × budget(target, support-heal)
```

`inCombat` = the player is in combat. Ward XP uses the same `raw` division with
`effect(L_wards)`.

## Damage pool per actor

- `pool[a]` = health actor `a` lost to damage **not caused by the player**. It grows on every
  health loss of `a` whose source is not the player (enemies, falling, traps, other NPCs'
  spells), and is capped at `a`'s max health.
- Source: the attacker / caster of the hit or the magic effect. Damage over time from the
  player's own effects counts as the player's.
- Entries for actors not seen for a while (unloaded, a day of game time) are dropped; not saved
  to the co-save (a pool lost on load only costs a bit of XP).

**Variants for detecting the source:**

1. A hook on health damage (`Actor::HandleHealthDamage` or the hit hook of *player-hit-hook*,
   widened to all actors near the player): attacker known. Preferred.
2. `TESHitEvent` (cause, target) plus the health difference: coarser, misses non-hit damage.

**Fallback** (decided): if neither gives a reliable source, no pool; healing others gets × 0.2
in combat and × 0.1 out of combat.

## Config

- `kHeal` (budget, × target max health), refill period: placeholders, as for the other budgets
  (Q-AG-7);
- multipliers 1 / 0.1 (self), 1 / 0.1 (others), fallback 0.2 / 0.1;
- DR window and curve for self heals: as for crafting (Q-AG-12) until tuned.

## Test

- Heal a follower hurt by a bandit: XP for the health they lost. Hit the follower yourself, then
  heal: no XP for that part.
- Healing at full health: no XP. Out of combat: × 0.1.
- Spam heal on one follower: XP falls once `kHeal × max health` is used, recovers after refill.
- Self-heal spam out of combat: × 0.1 and decreasing with repeats.
- With `restoration/help` at 100 (+25% heal): XP per cast is the same as at 50.
