# Spell classifier: form, school sub-direction, archetype

- **Status:** done (prototype implementation; LoreRim spell dump pending).
- **Design:** [`skills/magic/design.md`](../skills/magic/design.md) (forms, Q-MAG-1; Q-MAG-2
  proposal: costliest effect only), [destruction](../skills/destruction/design.md),
  [restoration](../skills/restoration/design.md); Q-CAT-1 is open (see *Ids*).
- **Depends on:** [`plugin-foundation.md`](plugin-foundation.md) (config).
- **Research:** none; the dump button below checks the rules on the real spell list.

## What it does

For a `SpellItem` of a managed school, returns three sub-skill ids: form, sub-direction,
archetype. Cached per spell.

## Ids (prototype assumption for Q-CAT-1)

One set of forms and one archetype per school: `<school>/<form>`, `<school>/<direction>`,
`<school>/standard`, `<school>` = `destruction` / `restoration` (the skill of the award). This
keeps "one parent per sub-skill" and is easy to change later; it is **not** a design decision.

## Form (costliest effect `spell->GetCostliestEffectItem()->baseEffect`)

Check in this order; the first rule that applies wins:

| # | Condition (MGEF `data`, projectile `BGSProjectile::data.types`) | Form |
|---|---|---|
| 1 | archetype `kCloak` | `cloak` |
| 2 | delivery `kSelf` (includes wards and self heals) | `self` |
| 3 | delivery `kTouch` | `touch` |
| 4 | delivery `kTargetActor` | `direct` |
| 5 | delivery `kTargetLocation`, casting `kConcentration` | `wall` |
| 6 | delivery `kTargetLocation`, otherwise | `rune` |
| 7 | aimed, projectile type `kBeam` | `beam` |
| 8 | aimed, projectile type `kFlamethrower` or `kCone` | `flame` |
| 9 | aimed, projectile type `kBarrier` | `wall` |
| 10 | aimed, `kMissile`, `kGrenade` (lobber) or `kArrow`, with an explosion (MGEF `data.explosion` or the projectile's explosion) | `explosion` |
| 11 | aimed, `kMissile`, `kGrenade`, `kArrow` | `missile` |
| 12 | anything else | none (log `[spell] no form`) |

- Rules 5 / 6 are a heuristic (vanilla runes are fire-and-forget at a location, walls are
  concentration); the dump will show if LoreRim breaks it.
- Summon archetype (`kSummonCreature`) in these schools: no form (Q-MAG-1; the Conjuration part is
  not in prototype 1).

## Sub-direction

From [`config/magic-directions.toml`](../config/magic-directions.toml): the keywords of the
costliest effect (MGEF `HasKeyword`), rules in file order, first match wins; no match →
`<school>/other`. Keywords resolved by FormKey at `kDataLoaded`; a keyword that does not resolve
is logged and skipped.

## Archetype

Always `<school>/standard` in prototype 1 (R-7: archetype lists not made yet).

## Debug

- The debug panel shows, per hand, the equipped spell with its form, sub-direction and the
  projectile type in words.
- Button *Dump player spells*: logs every spell the player knows, for all schools, with casting,
  delivery, projectile type, explosion yes/no, form and sub-direction.

## Test

## Prototype notes

- Pure rule precedence is tested in SpellRules.h; Classification.cpp supplies
  the engine's costliest effect, delivery, casting type and projectile flags.
- Keyword FormKeys resolve at DataLoaded and after config reload. Missing
  keywords are logged and skipped. Spell classification is read live rather
  than cached, so another plugin's post-load form edits are visible.

- Ice Spike (`02B96C:Skyrim.esm`) and Firebolt (`012FD0`) → `missile`; Lightning Bolt (`02DD29`)
  and Sparks (`02DD2A`) → `beam`; Fireball (`01C789`) → `explosion`; Flames → `flame`; Healing →
  `self`; Heal Other → `missile` (check what LoreRim makes of it); a ward → `self` +
  `restoration/wards`.
- Frost spell → `destruction/frost`; Healing → `restoration/help`.
