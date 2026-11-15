# Armor movement XP: our version of Requiem's passive armor training

- **Status:** chosen (variant 2), not started.
- **Design:** [`skills/armor/design.md`](../skills/armor/design.md), *Movement in armor*.
- **Depends on:** [`plugin-foundation.md`](plugin-foundation.md) (Papyrus build and package),
  [`subskill-state.md`](subskill-state.md) (award), [`xp-capture.md`](xp-capture.md) (per-skill
  base multipliers).
- **Research:** [R-34](../research/R-34-requiem-armor-movement-and-hits.md) (script logic done;
  numbers and conflicts open).

## What it does

Requiem's `REQ_PassiveArmorTraining` (a player alias script) gives Heavy / Light Armor XP every
`UpdateInterval` seconds while the player runs (× 2 sprinting), summed per worn slot from the
esp property `Skillgain[]` (clothing counts half towards light). Through `Game.AdvanceSkill` this
XP would reach us as an external source (potential of all sub-skills, design-v2 §4.4.4). We
send it to the movement sub-skills instead: `heavy-armor-movement`, `light-armor-movement`.
Player only (the script only runs for the player).

## Variants

1. **ESL override of the quest alias's script properties** (`Skillgain` = 0, or our script on
   the alias). Rejected: a running quest keeps its script instance and property values in the
   save, so this does not reach existing saves (we support installing into a running game), and
   overriding the quest copies the whole record (LoreRim patches must be carried forward).
2. **Replace the script file** (chosen). We ship our own `REQ_PassiveArmorTraining.pex` (loose,
   or in our BSA); it wins over `Requiem.bsa`. Code comes from the file, so it works on existing
   saves; the script's variables and states are restored by name.
3. **Hook `Game.AdvanceSkill`** and tell this script's calls from quest rewards by the caller on
   the Papyrus stack. Rejected: relies on VM internals.

## Our script (variant 2)

- Same `Scriptname REQ_PassiveArmorTraining extends REQ_CoreScript`, **the same property names
  and types** (`contributions`, `Skillgain`, `UpdateInterval`, `ArmorDebug`, `worn`), the same
  state names (`StartEvaluation`, `EvaluateArmor`) and the same events, so the esp's VMAD binds
  and saved instances load without errors.
- Written by us from the logic in R-34, not copied (Requiem's license: check before shipping).
- Keeps the slot loop and weight classes; computes `gain_heavy`, `gain_light` separately and drops clothing (Requiem adds clothing / 2 into light; we
  ignore it).
- Checks **sprinting first**, then running (R-34 check 3: Requiem's order may never reach the
  sprint branch).
- Instead of `Game.AdvanceSkill` it calls our native function
  `SkillPractice.ArmorMovement(float heavy, float light, bool sprinting)`.
- Without our DLL the native call fails and gives nothing (logged by Papyrus); uninstalling our
  mod brings Requiem's file back.

## DLL side

- Native `ArmorMovement` (registered with `SKSE::GetPapyrusInterface()->Register`), queued to the
  main thread.
- `raw = gain × (sprinting ? sprint_mult : 1) × xp_mult × base_mult(skill)`; `heavy` →
  `heavy-armor-movement`, `light` → `light-armor-movement`. One
  award per call; the sub-skills have a small fixed `k` (0 → 100 alone: main 15 → 35), so the
  award also lowers the main skill's rust.
- Not counted as a "hit taken" for the window rules (*Casting in armor*).

## Config

`[armor_movement]` in `SkillPractice.toml`: `enabled = true`, `xp_mult` (placeholder 1.0 until
R-34 check 1 gives Requiem's rate), `sprint_mult = 2.0`.

## Package

`Scripts/REQ_PassiveArmorTraining.pex` and `Scripts/Source/REQ_PassiveArmorTraining.psc` in the
package ([`plugin-foundation.md`](plugin-foundation.md) *Package and install*). The install note
says: our mod must win the file conflict (below Requiem and any LoreRim patch that ships this
script, R-34 check 2). The script is compiled against Requiem's script sources
(`REQ_CoreScript`) as well.

## Test

- Equip a full heavy set, run for 60 s: the log shows `ArmorMovement` calls, `heavy-armor-movement`
  grows, Heavy Armor gets no external XP (the potential of its sub-skills does not move).
- Sprint: the call has `sprinting = true` and twice the XP.
- Walk or stand: no calls. Mixed set: heavy and light parts go to their sub-skills. Clothing only: no calls with a gain.
- Load a save made before installing: no Papyrus errors about the script's properties or states.
