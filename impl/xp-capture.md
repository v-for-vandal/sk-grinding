# XP capture: vanilla use points → sub-skill XP

- **Status:** done (prototype implementation; R-17 runtime results pending).
- **Design:** design-v2 §4.1 (vanilla XP suppressed), §4.2 (pipeline), Q-XP-9 (raw use points),
  Q-XP-1 (a) (several sub-skills per event); per-skill rules:
  [weapons](../skills/weapons/design.md), [block](../skills/block/design.md),
  [smithing](../skills/smithing/design.md), [magic](../skills/magic/design.md),
  [destruction](../skills/destruction/design.md), [restoration](../skills/restoration/design.md).
- **Depends on:** [`subskill-state.md`](subskill-state.md), [`weapon-class.md`](weapon-class.md),
  [`spell-classifier.md`](spell-classifier.md), [`main-skill-level.md`](main-skill-level.md)
  (pending init).
- **Research:** [R-17](../research/R-17-raw-use-points.md).

## What it does

Takes the engine's own skill-use award (skill, raw points) for the player, decides which
sub-skills it belongs to, and gives them XP. For managed skills the vanilla award is suppressed.

## Hook

- `PlayerCharacter` vtable, `Actor::UseSkill(RE::ActorValue a_skill, float a_points,
  RE::TESForm* a_arg)` (virtual; find the index in CommonLibSSE-NG `Actor.h`; install with
  `REL::Relocation<std::uintptr_t>{ RE::VTABLE_PlayerCharacter[0] }.write_vfunc(idx, thunk)`).
  `a_points` are the raw use points before the AVIF `UseMult` (to confirm, R-17 check 1).
- At install, log the module that owns the original function pointer (another plugin may have
  hooked it first).
- In the thunk:
  1. log `[use] skill points arg` at debug level (always at debug, this is the R-17 telemetry);
  2. skill not managed (no skill file) → call the original, return;
  3. skill `pendingInit` → if the RaceSex menu has been closed at least once, run the start
     conversion now ([`main-skill-level.md`](main-skill-level.md)) and continue; else call the
     original, return;
  4. route (below); call the original only if `debug.vanilla_passthrough`.
- Raw XP for every sub-skill of the award: `raw = a_points × base_mult × debug.xp_mult`
  (`base_mult` from the skill file of `a_skill`). Each sub-skill gets the full `raw` (Q-XP-1 (a));
  factors below (bash) are applied on top.
- `a_points ≤ 0` → ignore (log at trace).

## Context: which item or spell the award is for

The award does not say *which* weapon, shield or spell. Resolution order:

1. `a_arg`, if it is a usable form (`TESObjectWEAP`, `TESObjectARMO`, `SpellItem`,
   `EffectSetting`, `TESAmmo`, a crafted item). What it actually contains per skill is R-17
   check 1; log it in any case.
2. **Context events** recorded by our sinks, newer than `context_window_ms`:

   | Kind | Source | Recorded |
   |---|---|---|
   | swing | `BSAnimationGraphEvent` on the player: `weaponSwing` (right), `weaponLeftSwing` (left) | hand, time |
   | bash | animation events `bashRelease` / `bashPowerStart` (names: check in R-17) | time |
   | hit | `TESHitEvent` with cause = player | weapon form, flags (power, bash, blocked) |
   | blocked | `TESHitEvent` with target = player and `kHitBlocked` | time |
   | cast | `TESSpellCastEvent` with object = player | spell, time |
   | craft | `ItemCrafted::Event` (and the crafting menu open/close) | item, time |

3. **Player state** at the moment of the award: equipped object per hand
   (`GetEquippedObject(left/right)`), the spell in each `MagicCaster` (`currentSpell`), the open
   crafting menu.

If the award comes **before** its context event (order is R-17 check 3), keep it in a small
pending queue and resolve it when the event arrives; after `context_window_ms` without one, use
the fallback of the skill below and log `[ctx] unresolved` at warn level.

## Routing per skill (prototype 1)

**One-Handed, Two-Handed.**

- Weapon: `a_arg` if a weapon; else the weapon in the hand of the last swing; else the only
  equipped weapon of that skill. No weapon (fists) → `1h-unarmed`.
- Sub-skill = the weapon's class ([`weapon-class.md`](weapon-class.md)). The class decides the
  sub-skill even if its parent differs from `a_skill` (design: the parent comes from the class).
- Player in a beast form (werewolf / vampire lord race) → no XP (Q-WPN-3).
- Fallback: `1h-other` / `2h-other` by `a_skill`.

**Block.**

- Bash (a bash context in the window, or the hit context has the bash flag): `bash` gets `raw`;
  if a shield is equipped in the left hand, its shield sub-skill also gets
  `raw × bash_shield_factor` (0.5, `[routing]` in the Block file). Torch or weapon bash: `bash`
  only.
- Else a block: left hand holds a shield (`TESObjectARMO` that is a shield) → `heavy-shield` or
  `light-shield` by `GetArmorType()`; otherwise → `parry`.
- Fallback: `parry` if no shield, else the shield sub-skill.

**Smithing.**

- Item: `a_arg`, else the last craft context, else the item selected in the open crafting menu
  (tempering).
- **Material sub-skill:** the first item keyword found in
  [`config/smithing-materials.toml`](../config/smithing-materials.toml) (FormKey → sub-skill);
  none → `other-smithing`.
- **Category sub-skill:** weapon: `one-handed-smithing` / `two-handed-smithing` by the weapon's
  class parent, bow → `bow-smithing`, crossbow → `crossbow-smithing`; ammo: arrow →
  `bow-smithing`, bolt → `crossbow-smithing`; armor: `heavy-armor-smithing` /
  `light-armor-smithing` by armor type, clothing → `clothing-smithing`; anything else: no
  category. Material keywords are read from weapons, armor and ammo (ammo since 0.1.4).
- **Jewelry:** an armor item in the ring, amulet or circlet slot (biped slots 36, 35, 42) that is
  clothing (not light / heavy armor) → only `jewelry`, no material, no category (Q-SMI-7).
- Prototype simplification: both sub-skills get `raw` (the vanilla points, i.e. LoreRim's
  `0.5 × item value`), not the `units × tier` formula of the design (R-15 not done). Smelting
  and tanning give no vanilla points anyway; if they do (R-17), drop them.

**Destruction, Restoration.**

- Spell: `a_arg` if a `SpellItem` (or the spell owning the `EffectSetting`); else the last cast
  context of a spell whose school is `a_skill`; else the spell in a caster whose school is
  `a_skill`.
- Source is a scroll or a staff (enchantment) → no XP (Q-MAG-1: scrolls ignored; staves are
  Enchanting).
- Three sub-skills get `raw` each ([`spell-classifier.md`](spell-classifier.md)):
  `<school>/<form>`, `<school>/<direction>`, `<school>/standard`, where `<school>` is `a_skill`.
- The healing rules of [`heal-xp.md`](heal-xp.md) (combat multipliers, damage pool, budget) are
  **not** in prototype 1: Restoration gets the vanilla points.
- Fallback: `<school>/other` and `<school>/standard` (no form).

**Concept sub-skills** (`dual-wield`, `with-shield`, `with-magic`): window rules of
[weapons](../skills/weapons/design.md) (*Window rules*), milestone 7 of the plan (optional). Until
then they stay at their start value.

## Debug: fixed XP per action

MCM toggle *Make all XP equal to 100* (`[debug] fixed_xp`, value `fixed_xp_value` = 100). For
fast tests, and as a fallback if the `UseSkill` hook turns out to get no points (R-17): the rest
of the prototype can then be tested in the same game session.

- While on, awards come from **our own events**, not from `UseSkill`. Every qualifying action is
  one award with `raw = fixed_xp_value × debug.xp_mult` for each sub-skill it routes to (bash:
  the shield's share is still `× bash_shield_factor`). `UseSkill` calls of managed skills are
  suppressed and only logged, so nothing is counted twice.
- Actions (each routed exactly as in *Routing per skill*, with the event as its context):

  | Action | Event |
  |---|---|
  | weapon or unarmed hit | `TESHitEvent`, cause = player, not a bash |
  | bash | `TESHitEvent`, cause = player, bash flag |
  | block | `TESHitEvent`, target = player, `kHitBlocked` |
  | Destruction / Restoration spell | `TESSpellCastEvent`, caster = player (a concentration spell: once per cast start) |
  | Smithing craft | `ItemCrafted::Event` |

  Tempering has no event: not covered in this mode (logged once).
- Log source `src=fixed` in the `[xp]` lines.
- The pending-init rule applies as for normal awards (the first such action after the race menu
  initializes the skill).

## Config

- `[xp] context_window_ms`, `[debug] xp_mult, fixed_xp, fixed_xp_value, vanilla_passthrough` in
  [`config/SkillPractice.toml`](../config/SkillPractice.toml).
- `[dynamics] base_mult`, `[routing]` in [`config/skills/*.toml`](../config/skills/).
- [`config/smithing-materials.toml`](../config/smithing-materials.toml).

## Prototype notes

- `PlayerCharacter::UseSkill` slot 0xF7 and player `Update` slot 0xAD chain
  existing vfunc owners. Context logs include steady-clock microseconds and the
  update counter. A managed-skill UseSkill call on a worker thread is suppressed
  and replayed on the main thread by the next player update (same FIFO as the
  deferred context events); unmanaged skills still pass through at once. Before
  0.1.4 such calls passed through, which leaked vanilla XP (seen as an external
  change of the main skill).
- The pinned CommonLib has no `PlayerSkills::AdvanceSkill` declaration or ID.
  The log-only second hook is `PlayerCharacter::AddSkillExperience`, Address
  Library 39413/40488, using MinHook. It always calls its original. This is a
  deliberate telemetry substitution, not proof of complete engine-path coverage.
- The pending queue waits for context up to the configured window. Selected
  SmithingMenu / ConstructibleObjectMenu recipes supply the tempering fallback.
- The six-skill implementation, UI and serialization share one runtime lifecycle;
  these integration sources are introduced with M1 and validated further in M2–M6.

## Test

- With `[log] level = "debug"`: every hit, block, bash, craft, temper, destruction hit, heal and
  ward absorb writes one `[use]` line and one or more `[xp]` lines naming the right sub-skills.
- An unarmed punch → `1h-unarmed`; a greatsword → `2h-sword`; a shield block → `heavy-shield` or
  `light-shield`; a dagger parry → `parry`; a shield bash → `bash` + half to the shield.
- Ice Spike hit → `destruction/missile`, `destruction/frost`, `destruction/standard`; healing
  self → `restoration/self`, `restoration/help`, `restoration/standard`.
- An Alteration spell (not managed) → original called, no `[xp]` line.
- No `[ctx] unresolved` warnings in a normal session; if there are, they show what to fix.
