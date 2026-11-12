# Implementation blocks

This folder lists **what the mod must implement**, as separate building blocks. The design
(`design-v2.md`, `skills/*/design.md`) says *what the player gets*; a block here says *what code
does it*, which variants exist, what it depends on and what must be checked first.

The question "what to implement, and in which order" is answered from this folder alone: read
the index below, then the files it needs. The design is read only through the links in each
file.

## Format of a block file

Each file `impl/<block>.md` has these parts:

- **Status:** `proposed` (variants not chosen), `chosen` (variant fixed, not started), `in
  progress`, `done`.
- **Design:** links to the design text this block implements.
- **Depends on:** other blocks (files here), or *planned* blocks not written yet (listed below).
- **Research:** links to research entries in [`research/`](../research/) that must be done
  before a variant is chosen.
- **What it does:** inputs, outputs, scope (e.g. player only).
- **Variants:** how it can be done, pros and cons, the preferred one and why.
- **Config:** settings the block reads.
- **Test:** how to see that it works in game.

A block is small enough to be one task for the executor agent (stage 2 plan). Plans that combine
blocks into a deliverable are in [`plan/`](../plan/) (first: [`prototype-1.md`](../plan/prototype-1.md)).

## Index

| Block | Status | Depends on | Research |
|---|---|---|---|
| [`block-stamina-cost.md`](block-stamina-cost.md): block stamina cost scaled by the shield / parry sub-skill | proposed | *player-hit-hook*, *subskill-state*, *effect-curve* | [R-23](../research/R-23-block-stamina.md) |
| [`perk-requirements.md`](perk-requirements.md): sub-skill requirements on existing perks (purchase and effects), value edits | chosen | *subskill-state* | [R-25](../research/R-25-perk-condition-injection.md) |
| [`spell-aim-inaccuracy.md`](spell-aim-inaccuracy.md): inaccuracy of `missile` (launch noise; lobbers included) and `beam` (aim sway) spells | chosen | *subskill-state*, *spell-classifier*, *projectile-launch-hook* | [R-26](../research/R-26-spell-aim.md) |
| [`heal-xp.md`](heal-xp.md): healing XP (damage-source filter for healing others, combat multipliers, per-target budget, XP without our bonus) | chosen | *subskill-state*, *player-hit-hook*, *effect-curve*, per-target budget | — |
| [`plugin-foundation.md`](plugin-foundation.md): project, build, ESL records, logging, package | chosen | — | R-21 |
| [`subskill-state.md`](subskill-state.md): sub-skill registry, values, XP award, co-save | chosen | *plugin-foundation* | — |
| [`main-skill-level.md`](main-skill-level.md): main level from sub-skills, rust, writes, start conversion | chosen | *subskill-state*, *decay* | [R-27](../research/R-27-new-game-init.md) |
| [`decay.md`](decay.md): active game time, idle meter, sub-skill decay | chosen | *subskill-state* | — |
| [`xp-capture.md`](xp-capture.md): vanilla use points → sub-skill XP, context, routing (prototype skills) | chosen | *subskill-state*, *weapon-class*, *spell-classifier*, *main-skill-level* | [R-17](../research/R-17-raw-use-points.md) |
| [`weapon-class.md`](weapon-class.md): `SPO_WeapClass_*` keywords (KID) → weapon sub-skill | chosen | *plugin-foundation* | R-19, R-20 |
| [`spell-classifier.md`](spell-classifier.md): spell form, school sub-direction, archetype | chosen | *plugin-foundation* | — |
| [`status-ui.md`](status-ui.md): status and debug panel (PrismaUI) | chosen | *subskill-state*, *main-skill-level*, *decay* | [frameworks §6](../refs/frameworks.md) |
| [`mcm-config.md`](mcm-config.md): config files, MCM Debug page | chosen | *plugin-foundation* | — |

## Planned blocks (not written yet)

Names used in *Depends on* before their files exist. Each will get a file when its design is
settled.

- *player-hit-hook* — one DLL hook on hits where the player is the attacker or the target; the
  base for weapon damage effects (design-v2 Q-DYN-9), glancing hits, block effects, XP events.
  Must coexist with Precision and Parrying RPG (design-v2 §14).
- *hit-damage-multiplier* — one damage multiplier per player hit, after vanilla and Requiem
  perks (design-v2 Q-DYN-9, option 3): weapon sub-skill and `dual-wield` effects
  ([`skills/weapons/design.md`](../skills/weapons/design.md)), `bash` damage on bash hits instead
  of them ([`skills/block/design.md`](../skills/block/design.md)). Built on *player-hit-hook*.
- *effect-curve* — the shared curve of design-v2 §5.7, evaluated from config parameters.
- *projectile-launch-hook* — one hook on `RE::Projectile::Launch` filtered to the player; used by
  spell launch noise and archery accuracy (Q-WPN-7). Must coexist with True Directional Movement.
