# R-23. Block stamina cost: which variant works

- **Status:** new (runtime tests and a static look at three SKSE plugins).
- **For:** [`impl/block-stamina-cost.md`](../impl/block-stamina-cost.md), variants A, A', B.

## Checks

| # | Question | How to check | Result decides |
|---|---|---|---|
| 1 | What the block cost is computed from: incoming damage or blocked damage? Is `fStaminaBlockBase` really 0 in game? Any other term (power attacks, shield vs weapon)? | Block hits with known damage (a test NPC with a fixed weapon); log the player's stamina before and after each hit; compare with `damage × fStaminaBlockDmgMult` for both readings of "damage". Also `getgs fStaminaBlockBase`. | the formula variant B must copy; whether A must also swap `fStaminaBlockBase` |
| 2 | Does the engine read `fStaminaBlockDmgMult` on every hit? | Console `setgs fStaminaBlockDmgMult 1.0` in game, block a hit, compare the drain with the default `0.1`. | **A is possible** only if the drain changes at once |
| 3 | Does the drain happen **inside** the hit function wrapped by *player-hit-hook*? | In a test DLL, log the player's stamina right before and right after the original call; the difference must include the block cost. If it does not, find where the drain happens (a later frame, an animation event). | A with this hook (yes), A' or B (no) |
| 4 | Are hits processed on one thread? | Log the thread id in the hook for player hits and for NPC-vs-NPC hits during a large fight. | A is safe (all on the main thread); otherwise A' or B |
| 5 | Do LoreRim mods touch block stamina? `ParryingRPG.dll` 2.0.1, `DualWieldParryingNG.dll`, `MCOBlockRecovery.dll`. | Static: search the DLLs and their INI / JSON for `fStaminaBlock`, `Stamina`, `DamageActorValue`. Runtime: repeat check 1 for a timed parry, a dual-wield block and right after a block recovery. | whether variant B misses their costs; whether A must read the value just before the swap (it already does) |
| 6 | Variant B only: does an extra drain that empties stamina break the block on the same hit? | Variant B test build: set stamina low, block a hit so that only the extra drain empties it; see whether the block breaks now or on the next hit. | whether B needs the DLL to stagger the player itself |

## How to choose

- **A** if checks 2, 3 and 4 pass. Preferred: the engine applies the cost and the block break
  itself; least code; no copy of the formula.
- **A'** if check 2 or 3 fails, or check 4 shows other threads, **and** the drain call can be
  found and hooked for 1.6.1170. Same behaviour as A, more reverse engineering.
- **B** if neither A nor A' is possible. Needs check 1 (formula) and check 6 (block break);
  check 5 tells which mod costs it will not see.
