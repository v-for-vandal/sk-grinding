# R-26. Spell aim: launch noise and aim sway

- **Status:** new (headers, LoreRim plugins, test DLL in game).
- **For:** [`impl/spell-aim-inaccuracy.md`](../impl/spell-aim-inaccuracy.md), variants A (beam) and
  B (missile form: missile and lobber projectiles); the same checks cover archery (Q-WPN-7).

## Checks

| # | Question | How to check | Result decides |
|---|---|---|---|
| 1 | Do player spell missiles and lobbers go through `RE::Projectile::Launch`, and does changing the launch angles in `LaunchData` before the original change the flight? Also for dual casting and spells with several projectiles. | headers (signature, fields `angleX` / `angleZ`, `desiredTarget`, auto-aim); test DLL with a fixed 10° yaw offset | **B works** for missile / lobber |
| 2 | Does True Directional Movement 2.2.5 (projectile aim support / homing) undo the launch noise? | read its INI (projectile / target lock options); test check 1 with target lock on and off | apply noise after TDM, or turn its option off for player spells |
| 3 | How does Requiem / LoreRim make the bow sway when stamina is out? (script `Game.ShakeCamera`, image space, engine bow sway, a mod) | search Requiem scripts / magic effects / GMSTs; watch in game with 0 stamina and a drawn bow | whether A can reuse it (same feel for bows and beams) |
| 4 | Which per-frame change moves the aim: player rotation (`SetRotationX/Z`, `data.angle`), camera state, or both, in first and third person? Does a held beam follow it? | test DLL: slow constant yaw drift while a beam is held | **A works**, and how |
| 5 | How to detect "player is casting a beam now" and since when: magic caster state per hand, spell's projectile type, start / stop of the cast | headers (`ActorMagicCaster`, `MagicCaster::State`), test log | trigger and `t` for the cumulative sway |
| 6 | Do SmoothCam and TDM (headtracking, directional camera) overwrite our rotation each frame? | test check 4 with them enabled (LoreRim default) | A as is, or apply the offset after them / to another value |
| 7 | Does any other LoreRim SKSE plugin hook `Projectile::Launch` (projectile tweaks)? | search skse-plugins.tsv list and DLL strings | hook order / conflicts |

## How to decide

- Missile / lobber: **B** if check 1 passes; check 2 tells how to deal with TDM.
- Beam: **A** if checks 4, 5, 6 pass; reuse the bow mechanism if check 3 finds one.
  If 4 or 6 fails: B per frame on `BeamProjectile`, with the same cumulative amplitude.
