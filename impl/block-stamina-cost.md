# Block stamina cost by sub-skill

- **Status:** proposed (variant not chosen).
- **Design:** [`skills/block/design.md`](../skills/block/design.md), continuous effects; curve:
  design-v2 §5.7.
- **Depends on:** *player-hit-hook*, *subskill-state*, *effect-curve* (planned, see
  [README](README.md)).
- **Research:** [R-23](../research/R-23-block-stamina.md) — decides which variant is possible
  and which is preferred.

## What it does

When a hit on the **player** is blocked, the stamina the engine takes for the block is
multiplied by `1 + effect(L)`, where `effect` is the block stamina curve (+50% at 0, 0 at 45,
−20% at 100, limit −30%; see the design; negative above 45). `L` is the level of the
sub-skill that blocks:

- shield block → `heavy-shield` or `light-shield` (shield armor type);
- weapon block (also two weapons with Dual Wield Parrying) → `parry`.

`L` is the `current` level after context penalties (§5.7). Not changed: the share of damage
blocked, Block XP, NPC blocking.

Engine background (to confirm in R-23): with Requiem the block costs
`fStaminaBlockBase + damage × fStaminaBlockDmgMult` (`0` and `0.1` in LoreRim); when stamina is
out, the block breaks and the player is staggered (`fStaminaBlockStaggerMult = 5`). So a higher
cost means the block breaks sooner, which is the intended penalty.

## Variants

### A. Swap the game setting around the player's hit (preferred if R-23 allows)

`fStaminaBlockDmgMult` is a game setting (GMST): the DLL can change it at runtime
(`RE::GameSettingCollection::GetSingleton()->GetSetting("fStaminaBlockDmgMult")`, field
`data.f`). It is shared by all actors, so it is changed only for the duration of one hit on the
player:

1. in the *player-hit-hook*, when the target is the player and the hit is blocked (or may be);
2. read the **current** value `v` (not one cached at load: another mod may change it too);
3. set `v × (1 + effect(L))` (also `fStaminaBlockBase` if R-23 shows it is non-zero);
4. call the original hit processing;
5. restore `v`.

- Pros: the engine itself takes the higher cost and breaks the block on this same hit; no need
  to find the exact engine call that drains stamina; no copy of the formula.
- Cons: works only if the engine reads the setting on every hit, and only if the drain happens
  inside the wrapped function; an NPC hit processed at the same moment on another thread would
  see the swapped value.
- Sub-variant **A'**: hook the engine call that drains stamina for a block (found with Address
  Library for 1.6.1170) and scale its amount. Same result as A, without the shared-setting risk,
  but needs reverse engineering. Use it if A fails only on the threading check.

### B. Extra drain after the hit

After the original hit processing, if the hit was blocked, take
`extra = effect(L) × (fStaminaBlockBase + damage × fStaminaBlockDmgMult)` stamina from the player
(settings read at runtime). For a bonus (negative effect), restore that amount instead.

- Pros: does not touch shared settings; works wherever the engine drains.
- Cons: copies the engine formula (must match what R-23 measures); if the extra drain empties
  stamina, the block does not break on this hit, only on the next one (unless the DLL staggers
  the player itself); a mod that changes the cost on its own (Parrying RPG…) is not seen.

**Order of preference:** A → A' → B. The reasons and the checks that pick one are in R-23.

## Config

- curve parameters per sub-skill group (shields, parry): `pen`, `P`, `M`, `h`, `p` (§5.7);
- `variant` (A / B) for debugging, if both are implemented.

## Test

- In game, block hits of known damage at sub-skill levels 0, 45, 100 (console sets the level);
  log stamina before and after each hit; the drain must follow the curve.
- Check that an NPC blocking the player's hits loses the same stamina as with the mod disabled.
- At level 0, the block must break after fewer hits than with the mod disabled.
