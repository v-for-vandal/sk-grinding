# Continuous (dynamic) skill-based effects

Reference note for [`design-v2.md`](../design-v2.md) (questions Q-DYN-7, Q-DYN-9).
Question: can one effect scale continuously with the skill level, instead of constant perks at
level checkpoints? Example: below 30, −(30 − s)% damage; above 30, +(s − 30)% damage.

Status: based on general knowledge of the Creation Kit perk system and CommonLibSSE-NG. Not yet
verified in the Lorerim setup. Items marked *(verify)* need confirmation on the Windows machine.

## 1. Perk entry points can read an actor value

Perk entry points (for example `Mod Attack Damage`, `Mod Incoming Damage`, `Mod Spell Magnitude`,
`Mod Spell Cost`, …) have function types beyond constants:

| Function | Effect on the value `v` |
|---|---|
| Set Value / Add Value / Multiply Value | constant |
| Add Range To Value | random in range |
| Set to Actor Value Mult | `v = AV × m` |
| Multiply Actor Value Mult | `v = v × AV × m` |
| **Multiply 1 + Actor Value Mult** | `v = v × (1 + AV × m)` |
| Add Actor Value Mult | `v = v + AV × m` |
| Set to (Negative) Absolute Value | sign tricks |

- There is no arbitrary formula. With several entries in a row you can build linear functions
  `a + b·s`.
- *(verify)* The AV read is the **current** value, including fortify effects, not the base value.

### 1.1 Worked example

Target damage multiplier: `1 + (s − 30)/100 = 0.7 + s/100`. Rewrite it as `0.7 × (1 + s/70)`:

1. Entry 1: `Multiply Value` = 0.7
2. Entry 2: `Multiply 1 + Actor Value Mult`, AV = the skill, mult = 1/70 ≈ 0.0142857

Check: s=0 → 0.70 (−30%); s=30 → 1.00; s=130 → 2.00 (+100%).

This works for any `a + b·s` with `a > 0`, as `a × (1 + (b/a)·s)`.

### 1.2 Branching and filtering

- A piecewise formula ("if s < 30 then … else …") is two sets of entries with perk conditions on the
  skill value.
- Entry conditions can filter the context, for example the weapon by keyword (`WeapTypeSword`),
  or the spell by keyword or school.
- The perk does not need to be in a skill tree. The DLL can give it to the player directly
  (`AddPerk`) as a hidden perk, at level 0.

### 1.3 Ordering

- Entries of the same entry point from all perks are applied in perk-entry priority order
  *(verify how priority works across different perks)*. Multiplicative entries stack
  multiplicatively with Lorerim/Requiem perks.

## 2. The catch: sub-skills are not actor values

Entry points can read only **actor values**, and our sub-skills (≈120) live in the DLL's own data.
Options:

| Option | How | Pros | Cons |
|---|---|---|---|
| 1. Mirror into spare AVs | DLL writes the sub-skill value into an unused AV, perk reads it | pure data, CK-editable | only ~10–15 spare AVs (`Variable01..10` etc.), other Lorerim mods may already use them; does not scale to 120 sub-skills |
| 2. Global-driven steps | DLL writes the value into a `GlobalVariable`; perk has N entries, each with a `GetGlobalValue` range condition | no AV conflicts | still stepwise (just finer steps); many records; entry-condition cost per hit |
| 3. **Compute in the DLL** | hook the entry-point evaluation (`RE::BGSEntryPoint::HandleEntryPoint` for `kModAttackDamage` etc.) or the hit-data/damage calculation, and multiply by our own formula after vanilla perks | any formula (non-linear, from config), no AVs, no records, scales to all sub-skills | needs hooks; effects not visible in CK; must be documented for players |

**Recommendation: option 3** for numeric bonuses (damage, cost, magnitude, speed, prices …). Keep
ESP perks for things that fit data better: new abilities, conditional effects, anything shown in a
perk tree.

### 2.1 Notes on option 3

- Hooking `HandleEntryPoint` catches every entry point. Filter by entry point ID, and by
  `perkOwner == player`.
- To apply the bonus once per event, put it after the vanilla calculation, as one multiplier per
  entry point and per event.
- Formula parameters live in config/MCM, per sub-skill.
- Continuous effects do not need hysteresis. Hysteresis is only needed for discrete perks that are
  added or removed at thresholds (design §5.6).

## 3. Related: skill values above 100 / no upper cap

- Main skills are float actor values. Their base can be set above 100 (`SetBaseActorValue`), and the
  current value already goes above 100 with fortify. The hard 100 cap is inside the vanilla
  skill-advance code, which our pipeline replaces anyway.
- Sub-skills are DLL data, so the engine imposes no cap on them at all.
- Practical limits: some vanilla/Requiem formulas may clamp the skill at 100 or behave oddly above it
  (prices, pickpocket chance, sneak detection). The vanilla skill menu is designed for 0–100.
  See research R-14 and question Q-DYN-8.
- If option 3 is used, our own effect formulas must stay sane at any level: linear or saturating,
  never exploding.
