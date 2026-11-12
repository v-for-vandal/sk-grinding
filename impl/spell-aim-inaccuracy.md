# Spell inaccuracy: aim sway and projectile direction noise

- **Status:** done (prototype implementation; R-26 runtime validation pending): **beam → variant A** (aim sway), **missile → variant B**
  (direction noise at launch; `missile` includes lobber projectiles, Q-MAG-1); details to confirm in R-26.
- **Design:** [`skills/magic/design.md`](../skills/magic/design.md), *Accuracy* (Q-MAG-5);
  background: [`refs/aim-perturbation.md`](../refs/aim-perturbation.md) (options A and B there).
- **Depends on:** *subskill-state*, *spell-classifier*, *projectile-launch-hook* (planned,
  see [README](README.md)).
- **Research:** [R-26](../research/R-26-spell-aim.md).
- **Shared with archery:** the same two mechanisms are planned for arrows (launch noise + visible
  sway while drawing, [`skills/weapons/design.md`](../skills/weapons/design.md), Q-WPN-7). Write
  them so that archery only adds its own trigger and sub-skill.

## What it does

Player only. A spell is less accurate at a low level of its **form** sub-skill (`missile`,
`beam`; `explosion` is open, Q-MAG-7). `missile` covers both straight and arcing (lobber)
projectiles (Q-MAG-1). Not affected: `flame`, `direct`, touch, runes and the other forms; NPCs.

`L` is the form's `current` level after context penalties (§5.7), read when the spell is
launched (missile) or every frame while it is held (beam).

## Variants

### A. Noise in the aim itself (aim sway)

The aim point wanders, as with a drawn bow when stamina is out: the crosshair visibly moves,
and the spell goes where the crosshair is. The player sees the problem and can partly correct it.

- A per-frame hook adds a smooth offset (random walk or Perlin noise, not per-frame jitter) to
  the player's aim yaw / pitch while the spell is being cast. Whether it rotates the player, the
  camera or both is decided in R-26 (check 4); the beam must follow the moved aim by itself (the
  engine re-aims a beam every frame).
- **Cumulative for beams** (design): amplitude `A(L, t) = A_max(L) × min(1, t / t_full)`, `t` =
  time the beam has been held; back to 0 when the cast stops.
- `A_max(L)` falls with the level and is 0 from a level `L_0` up.
- Reuse what Requiem / LoreRim does for the low-stamina bow sway if R-26 check 3 finds a
  reusable mechanism (same feel for bows and beams).
- Pros: fair and visible; one mechanism for spells and bows. Cons: moves the view, and camera
  mods (SmoothCam, True Directional Movement) may fight it; needs "player is casting a beam now"
  detection.

### B. Change where the projectile flies

At launch, rotate the projectile's direction by a random angle `δ ~ N(0, σ(L))` (independent in
yaw and pitch, or a random direction with angle `|δ|`). The crosshair does not move.

- Missile: in the *projectile-launch-hook* (`RE::Projectile::Launch`, shooter = player,
  projectile type missile / lobber), change the launch angles before calling the original.
- `σ(L) = σ_max × max(0, 1 − L / L_0)`.
- For a beam the direction is recomputed every frame, so B would need a per-frame
  `BeamProjectile` hook and is not chosen there (invisible to the player, feels like random
  misses).
- Pros: small code, no camera; one hook for spells and arrows. Cons: invisible, the player sees only
  where the spell lands (the design accepts this for one-shot spells); aim support / homing of True Directional Movement
  may undo it (R-26 check 2).

### Choice

| Form | Variant | Why |
|---|---|---|
| `beam` | **A** | held for seconds, cumulative sway reads naturally; B needs a per-frame beam hook anyway |
| `missile` (incl. lobbers) | **B** | one-shot projectiles; noise at launch is enough and simplest |

Fallbacks: if A is not possible for beams (R-26 check 4 or 6 fails), use B per frame on the beam
with the same cumulative amplitude. If B is undone by TDM homing, apply the noise after TDM or
disable its aim support for player spells (R-26 check 2).

**Several projectiles per cast** (decided): each projectile gets its own random `δ`.

## Prototype 1

- `L` = current level of `<school>/missile` or `<school>/beam` of the spell's school
  ([`spell-classifier.md`](spell-classifier.md); per-school forms are a prototype assumption for
  Q-CAT-1). Only managed schools (Destruction, Restoration) are perturbed.
- **Missile (B).** First choice: a hook on `RE::Projectile::Launch` (R-26 check 1). If a function
  hook on it is hard to install, use the fallback: a vtable hook on `MissileProjectile` (and the
  lobber class, if it is separate) that, on the **first** update of a projectile whose shooter is
  the player, rotates its `linearVelocity` and its angles by `δ`. Log which variant is used.
- **Beam (A).** Trigger: a player `MagicCaster` (left, right or dual) is charging or casting a
  spell whose form is `beam` (`MagicCaster::state`, R-26 check 5). `t` counts from the start of
  charging, so a fire-and-forget beam (Lightning Bolt) sways while it is charged and goes where
  the crosshair is at release; a concentration beam (Sparks) sways while held. Apply the offset
  as a per-frame **delta** to the player's yaw / pitch (so the player can still turn), and remove
  nothing at the end (the offset is a disturbance, not a lock). Smooth noise: sum of two sines
  per axis with random phases, frequency around `beam_noise_hz`.
- Config keys: `[aim]` in [`config/SkillPractice.toml`](../config/SkillPractice.toml); MCM toggles
  for both ([`mcm-config.md`](mcm-config.md)). Values: σ at level 0 = 6°, sway 4° after 3 s, both
  0 from level 75 (placeholders).
- The debug panel shows `σ(L)` and the current sway amplitude ([`status-ui.md`](status-ui.md)).

## Prototype notes

- Launch uses Address Library 42928/44108 and a MinHook trampoline. The original
  is always called. Only player SpellItem missiles in initialized managed schools
  receive noise; explosion, flame, NPC, scroll and weapon projectiles do not.
- Beam sway applies per-frame delta rotation through `TESObjectREFR::SetAngle`
  (the pinned API has no SetRotationX/Z). It stops without undoing the final
  disturbance. SmoothCam/TDM behaviour must be checked in first and third person.
- Sine weights/frequency ratios are configurable. `aim.debug_missile_yaw_deg=10`
  enables the fixed yaw probe for R-26; leave it at zero for normal gameplay.
- The panel shows actual current beam amplitude and missile sigma. No aim
  compatibility result is inferred from successful compilation.

## Config

- per form (`missile`, `beam`): `L_0`;
- B: `σ_max` (degrees);
- A: `A_max` (degrees), `t_full` (seconds), noise speed;
- values are placeholders, not decided.

## Test

- Fire 20 Firebolts (missile) at a wall at form level 0 and at `L_0`; the spread of impact points
  follows `σ`; none at `L_0`. Same with an arcing (lobber) missile spell.
- Hold a Lightning Bolt (beam) at level 0: the crosshair wanders more the longer it is held and
  stops when the cast ends; at `L_0` it does not move.
- An NPC casting the same spells is not affected; `flame` spells are not affected.
- With SmoothCam and TDM on (LoreRim defaults), in first and third person.
