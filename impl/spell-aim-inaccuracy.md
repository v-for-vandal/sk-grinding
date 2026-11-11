# Spell inaccuracy: aim sway and projectile direction noise

- **Status:** chosen per form: **beam → variant A** (aim sway), **missile, lobber → variant B**
  (direction noise at launch); details to confirm in R-26.
- **Design:** [`skills/magic/design.md`](../skills/magic/design.md), *Accuracy* (Q-MAG-5);
  background: [`refs/aim-perturbation.md`](../refs/aim-perturbation.md) (options A and B there).
- **Depends on:** *subskill-state*, *spell-form-classifier*, *projectile-launch-hook* (planned,
  see [README](README.md)).
- **Research:** [R-26](../research/R-26-spell-aim.md).
- **Shared with archery:** the same two mechanisms are planned for arrows (launch noise + visible
  sway while drawing, [`skills/weapons/design.md`](../skills/weapons/design.md), Q-WPN-7). Write
  them so that archery only adds its own trigger and sub-skill.

## What it does

Player only. A spell is less accurate at a low level of its **form** sub-skill (`missile`,
`lobber`, `beam`). Not affected: `flame`, `direct`, touch, runes and the other forms; NPCs.

`L` is the form's `current` level after context penalties (§5.7), read when the spell is
launched (missile, lobber) or every frame while it is held (beam).

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

- Missile and lobber: in the *projectile-launch-hook* (`RE::Projectile::Launch`, shooter = player,
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
| `missile`, `lobber` | **B** | one-shot projectiles; noise at launch is enough and simplest |

Fallbacks: if A is not possible for beams (R-26 check 4 or 6 fails), use B per frame on the beam
with the same cumulative amplitude. If B is undone by TDM homing, apply the noise after TDM or
disable its aim support for player spells (R-26 check 2).

**Several projectiles per cast** (decided): each projectile gets its own random `δ`.

## Config

- per form (`missile`, `lobber`, `beam`): `L_0`;
- B: `σ_max` (degrees);
- A: `A_max` (degrees), `t_full` (seconds), noise speed;
- values are placeholders, not decided.

## Test

- Fire 20 Firebolts (missile) at a wall at form level 0 and at `L_0`; the spread of impact points
  follows `σ`; none at `L_0`. Same with a lobber spell.
- Hold a Lightning Bolt (beam) at level 0: the crosshair wanders more the longer it is held and
  stops when the cast ends; at `L_0` it does not move.
- An NPC casting the same spells is not affected; `flame` spells are not affected.
- With SmoothCam and TDM on (LoreRim defaults), in first and third person.
