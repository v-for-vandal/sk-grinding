# Spell classifier: form, school sub-direction, archetype

- **Status:** done (prototype implementation; LoreRim spell dump pending).
- **Design:** [`skills/magic/design.md`](../skills/magic/design.md) (forms, Q-MAG-1; Q-MAG-2
  proposal: costliest effect only), [destruction](../skills/destruction/design.md),
  [restoration](../skills/restoration/design.md); shared forms and archetypes (decided 2026-11-14,
  see *Ids*).
- **Depends on:** [`plugin-foundation.md`](plugin-foundation.md) (config).
- **Research:** none; the dump button below checks the rules on the real spell list.

## What it does

For a `SpellItem` of a managed school, returns three sub-skill ids: form, sub-direction,
archetype. Cached per spell.

## Ids

Forms and archetypes are shared by all schools ([magic](../skills/magic/design.md); implemented
2026-11-14): `form/<form>`, `<school>/<direction>`, `archetype/<archetype>`
(`archetype/standard`), `<school>` = `destruction` / `restoration` (the skill of the award). The
classifier itself returns the form name; the router builds the ids.

- Shared sub-skills are in [`config/skills/shared.toml`](../config/skills/shared.toml) (curve,
  decay, no `[main]`, no budget, `k = 0`), loaded as the pseudo-skill `magic` (`Model::shared`,
  `pseudo = true`): no main level, no rust, not written to any actor value; the panel shows it
  as *Magic* ([`status-ui.md`](status-ui.md)). The school files keep only the `direction` budget
  and reference paths of sub-directions. Decided (2026-11-15): **three** sub-directions per
  school path (Destruction `fire`, `frost`, `shock`; Restoration `help`, `wards`, `sun`, each to
  75). With only two, one sub-direction alone took the school to ≈ 79 at 75 and the main level
  rose faster than the sub-direction (`g` up to 1.33); with three: ≈ 69 at 75, `g` ≤ 0.89.
- Co-save: old per-school records (`destruction/missile`, `restoration/standard`, …) are dropped
  at load (prototype saves only; log `[state] dropped old per-school record`).

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

- **Sunbeam classified as `flame` (first playtest, 2026-11-14).** The rules look only at the
  projectile **type** of the costliest effect. Sunbeam (`REQ_Restoration1_Sun_ConcAimed`,
  Requiem - Magic Redone; Concentration, Aimed) has as its main effect
  `REQ_Effect_Restoration1_Sun_ConcAimed` with projectile `REQ_Projectile_Restoration1_Sunbeam`
  (`Requiem - Magic Redone.esp|0x005C6B`); its other effects are cheap scripts / tapers without
  a projectile, so they cannot be the costliest one. Rule 7 (`beam`) is checked before rule 8,
  so `flame` means the PROJ record's type is **Flamethrower** (or Cone), not Beam: the mod
  draws a beam with a flame-type projectile (a stream of short-lived projectiles with beam
  art). The Mutagen dump has no PROJ table, so this is still to confirm: the *Debug* tab shows
  `projectile=` for the equipped spell (expected `flamethrower`;
  [`playtesting/spell-forms.md`](../playtesting/spell-forms.md)), and R-32 dumps all PROJ.
  - Fix (proposed, skills/magic Q-MAG-9): a config override list `[[form_override]]` in
    `magic-directions.toml` (by spell, effect or projectile FormKey → form), checked before the
    rules; plus, if R-32 finds more cases, a rule "flamethrower type with range ≥ X → `beam`".
- Rules 5 / 6 are a heuristic (vanilla runes are fire-and-forget at a location, walls are
  concentration); the dump will show if LoreRim breaks it.
- Summon archetype (`kSummonCreature`) in these schools: no form (Q-MAG-1; the Conjuration part is
  not in prototype 1).

## Sub-direction

From [`config/magic-directions.toml`](../config/magic-directions.toml): the keywords of the
costliest effect (MGEF `HasKeyword`), rules in file order, first match wins; no match →
`<school>/other`. A rule may also list `effects` (MGEF FormKeys): it matches when the costliest
effect is one of them (used for Alteration's Earth Cloak, whose keywords point elsewhere). Keywords
and effects resolved by FormKey at `kDataLoaded`; one that does not resolve is logged and skipped.
Alteration is not in prototype 1; its rules are in the file already.

## Archetype

Always `archetype/standard` in prototype 1 (R-7: archetype lists not made yet).

Results are cached per spell FormID (main thread), cleared on every config reload.

## Debug

- The debug panel shows, per hand, the equipped spell with its form, sub-direction and the
  projectile type in words.
- Button *Dump player spells*: logs every spell the player knows, for all schools, with casting,
  delivery, projectile type, explosion yes/no, form and sub-direction.

## Prototype notes

- Pure rule precedence is tested in SpellRules.h; Classification.cpp supplies
  the engine's costliest effect, delivery, casting type and projectile flags.
- Keyword FormKeys resolve at DataLoaded and after config reload. Missing
  keywords are logged and skipped. Spell classification is read live rather
  than cached, so another plugin's post-load form edits are visible.

## Test

- Ice Spike (`02B96C:Skyrim.esm`) and Firebolt (`012FD0`) → `missile`; Lightning Bolt (`02DD29`)
  and Sparks (`02DD2A`) → `beam`; Fireball (`01C789`) → `explosion`; Flames → `flame`; Healing →
  `self`; Heal Other → `missile` (check what LoreRim makes of it); a ward → `self` +
  `restoration/wards`.
- Frost spell → `destruction/frost`; Healing → `restoration/help`.
