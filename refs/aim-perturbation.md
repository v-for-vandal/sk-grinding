# Aim perturbation: spells, projectiles, melee

Reference note for [`design-v2.md`](../design-v2.md) (questions Q-WPN-7, Q-WPN-8, Q-MAG-5).
Summary: whether and how an SKSE (CommonLibSSE-NG) plugin can make the player's aim less accurate,
for example depending on sub-skill level.

Status: based on general knowledge of the engine and of CommonLibSSE-NG. It has **not** been checked
against the Lorerim runtime. Function names and the class hierarchy must be confirmed on the
Windows machine (CommonLibSSE-NG headers + Address Library IDs). Items marked *(verify)* are the
least certain.

Chosen for spells (beam: A, missile incl. lobbers: B): [`impl/spell-aim-inaccuracy.md`](../impl/spell-aim-inaccuracy.md).

## 1. Options at a glance

| Option | What changes | Difficulty | Recommendation |
|---|---|---|---|
| A. Camera / view sway | player yaw/pitch is changed every frame | medium | only as a visual cue, and only while aiming |
| B. Projectile direction noise at launch | `LaunchData` direction before the projectile is spawned | low (missiles, arrows); high (beams, flames) | **main mechanism** for spells and archery |
| C. Melee miss / glancing chance | hit is cancelled or its damage reduced | low | **main mechanism** for melee |
| D. Melee hit cone / reach for the player | target search geometry | high | only if C is not good enough |
| E. Physical weapon sway (with Precision) | weapon collision / animation | very high, fragile | not recommended |

## 2. A. Camera / view sway

- Apply a small periodic or noise offset to the player's rotation (`PlayerCharacter` angles) or to
  the camera state, from a per-frame hook such as player update or camera update.
- In third person the aim point comes from a ray cast from the camera through the crosshair. So the
  camera has to move, not only the character.
- Downsides: the whole screen moves (motion sickness if it never stops), and it affects all actions,
  not only shooting and casting.
- Milder variant: sway only while a bow is drawn or a spell is charging, like aiming sway in
  shooters. Amplitude depends on the sub-skill.
- Main value: **visible feedback**, which makes option B feel fair.

## 3. B. Projectile direction noise

### 3.1 Hook point

- Every projectile (arrows, bolts, spell missiles, beams, flames, cones, lobbers) is spawned through
  `RE::Projectile::Launch(...)`, which takes `RE::Projectile::LaunchData` (origin, angles,
  shooter, spell/ammo, weapon, …). *(verify exact signature and field names in CommonLibSSE-NG)*
- Hook `Launch`. When `shooter` is the player, rotate the direction (`angleX` pitch / `angleZ` yaw)
  by a random angle before calling the original.
- The hook sees **all** actors' projectiles, so filter by `shooter == player` unless NPCs should be
  affected too.

### 3.2 Per projectile type

| Projectile type (`BGSProjectile` type) | Examples | How to add noise |
|---|---|---|
| Missile | Firebolt, Ice Spike, arrows, bolts | noise at launch is enough |
| Lobber | arcing projectiles (e.g. some runes/explosives) | noise at launch is enough |
| Arrow | bows/crossbows | noise at launch; can also scale with draw time |
| Cone | cone spells | noise at launch *(verify)* |
| Beam | Lightning Bolt, Chain Lightning-like | direction is **re-evaluated every frame** from the caster → launch noise alone is not enough; hook the beam's per-frame update (`RE::BeamProjectile`) *(verify)* |
| Flame | Flames, Frostbite, Sparks (concentration "spray") | same as beam: per-frame update hook (`RE::FlameProjectile`) *(verify)* |

- For beam and flame, a smooth, slowly changing noise (for example Perlin noise or a random walk)
  looks better than independent per-frame jitter.
- For missiles, a simple model: deviation angle ~ `N(0, σ)`, with
  `σ = σ_max × (1 − level / levelRef)` clamped at 0. `σ_max` and `levelRef` come from config.

### 3.3 Which sub-skill drives the noise

- Archery: the weapon's `archery-*` sub-skill (accepted, Q-WPN-7, with a visible aim sway).
- Spells: only the form sub-skill, and only `missile` (lobber projectiles included, Q-MAG-1) and `beam` (decided, Q-MAG-5; see
  [`skills/magic/design.md`](../skills/magic/design.md)). `flame`, `direct`, touch and runes are
  not perturbed. Beams get a cumulative sway that grows while the beam is held.
- Melee: glancing hits, no misses (decided, Q-WPN-8).

### 3.4 Pitfalls

- Aim-assist / homing (`desiredTarget` in launch data, auto-aim) can cancel the noise. Check it.
- Other SKSE mods in Lorerim may hook `Projectile::Launch` too (arrow tweaks, projectile overhauls).
  Check for conflicts during research.
- Noise without visible feedback reads as "random unfair misses". Pair it with option A or a UI cue.

## 4. Melee

### 4.1 How vanilla melee hits work

- In vanilla, melee hits are **not** physical collisions. At the hit frame of the attack animation
  the engine searches for targets inside a cone in front of the attacker, limited by distance.
  - Cone angle: a game setting (`fCombatHitConeAngle`) *(verify name/value)*.
  - Distance: weapon reach × game settings (`fCombatDistance`, weapon `reach` field).
- Changing these game settings is global: it affects every actor, not only the player.

### 4.2 C. Miss / glancing chance (recommended)

- Hook the point where the player's melee hit is applied to a target: the hit event / hit data
  population, or the damage application *(verify the best hook; `TESHitEvent` comes too late to
  cancel)*.
- With probability `pMiss(level)`, cancel the hit (no damage, play a deflect/miss effect).
  Alternatively, make it a "glancing" hit with reduced damage and no stagger.
- Easiest to implement and fully controlled from config. Glancing hits feel better than full misses.

### 4.3 D. Player-only hit cone / reach

- Hook the target search for the player only and narrow the cone or reach depending on the
  sub-skill.
- Requires reverse engineering of the target-search function. Do it only if C is not satisfying.

### 4.4 E. Physical sway with Precision

- If Lorerim uses **Precision** (physical weapon collision), melee hits follow the weapon's actual
  trajectory. In theory the trajectory could be perturbed.
- That means touching animation or collision code, which is very fragile. Not recommended.
- Research: check whether Precision is in Lorerim (R-13).

## 5. Recommendation

- Spells and archery: **B** (launch noise for missiles, arrows and lobbers first; beams and flames
  later via per-frame hooks), plus optional **A** while aiming, as feedback.
- Melee: **C** (miss or glancing chance).
