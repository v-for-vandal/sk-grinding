# Armor (Heavy Armor, Light Armor)

> Part of the design: [`design-v2.md`](../../design-v2.md) (was §7.2). Bare `§N` references point to design-v2.md. Index of skills: [`skills/README.md`](../README.md).

**Sub-skills:**

| Parent | Sub-skills |
|---|---|
| Heavy Armor | `heavy-armor`, one per heavy material (`iron-armor`, `steel-armor`, `dwarven-armor`, `orcish-armor`, `ebony-armor`, `daedric-armor`, `dragonplate-armor`, + Anniversary Edition and modded materials, + `heavy-other`) |
| Light Armor | `light-armor`, `clothing`, one per light material (`leather-armor`, `hide-armor`, `elven-armor`, `glass-armor`, `scaled-armor`, `dragonscale-armor`, + AE and modded, + `light-other`) |
| Heavy Armor (concept) | `heavy-armor-casting` |
| Light Armor (concept) | `light-armor-casting` |
| Heavy Armor (movement) | `heavy-armor-movement` |
| Light Armor (movement) | `light-armor-movement` |

- `clothing` is a sub-skill of Light Armor, so it counts towards the Light Armor level (as LoreRim
  gives clothing XP to Light Armor).
- Material is determined by `ArmorMaterial*` keywords; for modded armor, research which keywords
  are used (R-5); fallback to `heavy-other`/`light-other`.
- Base XP per hit taken is the vanilla (Lorerim) amount, subject to anti-grind rules (§11).

**Slot weights** (configurable):

| Slot | Weight |
|---|---|
| Body | 0.70 |
| Head | 0.20 |
| Feet | 0.05 |
| Hands | 0.05 |

Rings, amulets and shields are not counted (shields belong to Block).

**Mixed sets (explicit rule):** for each hit taken, for every counted slot:
1. Determine the item's armor type (heavy / light / clothing) and material. An **empty slot
   counts as clothing** (decided, Q-ARM-1), without a material.
2. Grant `baseXP × slotWeight` to the type sub-skill (`heavy-armor`, `light-armor` or `clothing`).
3. Grant `baseXP × slotWeight` to the material sub-skill (if any).

Example: heavy ebony cuirass + light leather helmet + nothing else, base XP 10 →
`heavy-armor` +7, `ebony-armor` +7, `light-armor` +2, `leather-armor` +2, `clothing` +1 (empty feet
and hands).

- **Main skill:** both the type and the material sub-skill count in the main skill formula, each
  with its own curve `M_i` (§4.4.1; Q-XP-1, option (a), decided), normalized per main skill
  (§4.4.5). This answers Q-ARM-2.

**Casting in armor** (decided 2026-11-14): concept sub-skills `heavy-armor-casting` (parent
Heavy Armor) and `light-armor-casting` (parent Light Armor), `k = 0` (nothing to the main skill,
§4.4.1), no cross-links (§6.2), like `with-magic` ([weapons](../weapons/design.md)).

- **Which one:** the type of the **cuirass** (body slot) worn at the moment of the event: heavy →
  `heavy-armor-casting`, light → `light-armor-casting`. No cuirass, or clothing / a robe in the
  body slot → neither.
- **XP: window rule** (glossary; the window rules of [weapons](../weapons/design.md), `windowSec =
  10`, configurable). Complementary pair = (**hit taken** by the player, **spell cast**):
  - a spell cast qualifies if the player took a hit in the last `windowSec` seconds; the
    `*-armor-casting` sub-skill of the cuirass gets the XP the cast gives to its own magic
    sub-skill;
  - symmetric, as `with-magic`: a hit taken qualifies if the player cast a spell in the window;
    the sub-skill gets the XP the hit gives to the cuirass's type sub-skill (`baseXP × body
    weight`, *Mixed sets* above).
  - Events as in the weapons rules: a hit taken = one that grants armor XP (after the
    anti-grind rules of §11); a spell cast = any cast that grants XP to its own magic sub-skill.
    XP is taken after the per-skill base multipliers (Q-XP-9). No one-to-one pairing.
- **Effects (context penalty, §5.7), like `with-magic`; no bonus:** after the checkpoint the
  sub-skill only unlocks perks. Factor `0.75` at level 0, linearly to `1.0` at `C = 45`.
  - **Armor side:** while a spell is equipped in either hand, the effective level of the worn
    armor's type and material sub-skills = `current × factor(heavy/light-armor-casting)`: armor
    rating and stamina lost on hit (*Continuous effects* below).
  - **Magic side:** a spell cast while wearing a heavy / light cuirass: the effective level of
    the spell's magic sub-skills (form, school sub-direction, archetype) = `current ×
    factor(...)`. With a weapon in the other hand `with-magic` also applies; the factors multiply
    (§5.7).
  - Only our sub-skills' effects are affected; `current`, perk thresholds and the main skills
    are not.
- **Perks:** the casting branches of both trees require these sub-skills (worksheets:
  [`heavy-armor.perks.md`](heavy-armor.perks.md) Combat Casting HA-03 → HA-05 → HA-07 → HA-17,
  [`light-armor.perks.md`](light-armor.perks.md) Agile Spellcasting LA-08 → LA-13). Rule for every
  perk of the branch with `HeavyArmor` / `LightArmor ≥ X`: the main-skill requirement becomes
  `max(25, X − 20)` and `heavy-armor-casting` / `light-armor-casting ≥ X` is added (checked on
  `current` with the hysteresis of §5.6). Implementation:
  [`impl/perk-requirements.md`](../../impl/perk-requirements.md) (replace a `Skill ≥ N` item, add a
  global).

  | Perk | Main skill | Sub-skill |
  |---|---|---|
  | HA-03 Combat Casting | HeavyArmor 25 → 25 | `heavy-armor-casting ≥ 25` |
  | HA-05 Combat Trance | HeavyArmor 50 → 30 | `heavy-armor-casting ≥ 50` |
  | HA-07 Combat Meditation | HeavyArmor 75 → 55 | `heavy-armor-casting ≥ 75` |
  | HA-17 War Mage | HeavyArmor 100 → 80 | `heavy-armor-casting ≥ 100` |
  | LA-08 Agile Spellcasting | LightArmor 25 → 25 | `light-armor-casting ≥ 25` |
  | LA-13 Agile Spellcasting (rank 2) | LightArmor 50 → 30 | `light-armor-casting ≥ 50` |

**Movement in armor** (decided 2026-11-15): sub-skills `heavy-armor-movement` (parent Heavy
Armor) and `light-armor-movement` (parent Light Armor). They take over Requiem's passive armor
training (XP for running in armor, [R-34](../../research/R-34-requiem-armor-movement-and-hits.md)),
which otherwise reaches us through `Game.AdvanceSkill` as an external source. Not given to the
type sub-skills, so running laps does not raise the main level through `heavy-armor` /
`light-armor`.

- **XP:** while running (× 2 sprinting), every `UpdateInterval` seconds, Requiem's per-slot gain
  of the worn pieces: heavy pieces → `heavy-armor-movement`, light pieces →
  `light-armor-movement`; rate configurable. **Clothing gives nothing** (decided, former
  Q-ARM-7): running in clothing is ignored.
- **How:** we replace Requiem's script file with our own version that calls the DLL
  ([`impl/armor-movement-xp.md`](../../impl/armor-movement-xp.md)).
- **Effects:** movement speed (*Continuous effects* below).
- **Main skill: a small fixed `k`** (decided, former Q-ARM-6): a movement sub-skill trained
  0 → 100 alone takes its main skill 15 → 35. With the placeholder shape (`T = 90`, `w = 15`)
  that is `k ≈ 0.060` (`normalize_conversion.py`, path level 100), set as `k = …` in the skill
  file, outside the normalization of the other groups. Alongside other trained sub-skills it adds
  the same main XP, so fewer levels. Anti-grind for running laps / autorun (§11): open with R-34
  check 1 (the real rate).

**Continuous effects** (decided 2026-11-15, former Q-ARM-5). The shared effect curve (§5.7) on the
sub-skill scale, checkpoint `P = 50`, `p = 1`; values fitted to the requested points:

| Effect | Sub-skills | Curve | L = 0 | 25 | 50 | 75 | 100 | 150 | 200 | limit |
|---|---|---|---|---|---|---|---|---|---|---|
| Armor rating of the piece | `heavy-armor`, `light-armor` | `pen 30, P 50, M 40, h 150` | −30 % | −15 % | 0 | +5.7 % | **+10 %** | +16 % | **+20 %** | +40 % |
| Stamina lost on an unblocked hit | `heavy-armor`, `light-armor` | `pen 50, P 50, M 40, h 150`, `invert` | **+50 %** | +25 % | 0 | −5.7 % | **−10 %** | −16 % | **−20 %** | −40 % |
| Movement speed | `heavy-armor-movement`, `light-armor-movement` | `pen 50, P 50, M 37.5, h 75` | **−50 %** | −25 % | 0 | +9.4 % | **+15 %** | +21.4 % | **+25 %** | +37.5 % |

- **Driven by the type sub-skill** (decided, former Q-ARM-8): a heavy piece uses `heavy-armor`, a
  light piece `light-armor`.
- **Material sub-skills: armor rating** (decided 2026-11-15): per worn piece of that material,
  a second factor `rating × (1 + effect_mat(L_material))`, the shared curve with `−30 %` at 0,
  `0` at the checkpoint 50, `+F1(tier) %` at 100 and `+F2(tier) %` at 200 (`M`, `h` fitted per
  tier as above). `F1` / `F2` are small tables by **material tier**; materials are mapped to
  tiers in a worksheet ([`refs/lorerim/armor-material-tiers.md`](../../refs/lorerim/armor-material-tiers.md)).
  Tables and tiers: to be filled by the user. The casting penalty applies to this level too.
- **`clothing`: flat armor rating, no penalty** (decided 2026-11-15, former Q-ARM-9). Points,
  not a percentage, so it does not depend on the clothing records' own rating (0 in vanilla;
  works without LoreRim). The shared curve with `pen = 0, P = 50, M = 200, h = 150`, in armor
  rating points for a full clothing set:

  | L | 0–50 | 75 | 100 | 150 | 200 | limit |
  |---|---|---|---|---|---|---|
  | bonus, points | 0 | +28.6 | **+50** | +80 | **+100** | +200 |

  Applied as `Σ_slots slotWeight × bonus(L_clothing)` over **worn clothing pieces only**;
  empty slots do not count (a naked character gets nothing). Not stamina on hit (below).
  Alternative kept in research: a *Mod Armor Rating* "add value" entry on clothing items
  ([R-36](../../research/R-36-mod-armor-rating.md)).
- **Armor rating:** per worn heavy / light piece, `rating × (1 + effect(L_type))`, on top of
  vanilla and Requiem (the vanilla skill scaling of armor rating still reads the main skill).
- **Stamina on hit:** Requiem's own stamina damage of an unblocked hit (spells `StaminaDamage[*]`,
  [R-34](../../research/R-34-requiem-armor-movement-and-hits.md)) × `(1 + Σ_slots slotWeight ×
  effect(L_type of the piece))` (*Slot weights*). Clothing and empty slots add 0 (decided): an
  unarmored character takes Requiem's amount unchanged. Requiem's amount and its conditions are
  R-34 check 4.
- **Movement speed:** `SpeedMult` change = `Σ_slots slotWeight × effect(L_movement of the
  piece's type)` (heavy pieces → `heavy-armor-movement`, light → `light-armor-movement`, clothing
  and empty slots → 0). A full heavy set at level 0 is −50 %, a cuirass alone −35 %. It adds to
  Requiem's armor weight penalty (R-33: e.g. −30 for 60 weight without perks); the sum must be
  checked in playtesting.
- Effective level as usual: the casting penalty (*Casting in armor*) lowers the type and material
  levels while a spell is equipped (armor rating, stamina on hit), not the movement ones.
- How: [`impl/armor-effects.md`](../../impl/armor-effects.md).

**Questions:**

- **Q-ARM-3.** Anniversary Edition and modded materials: own sub-skill per material, or group
  them (e.g. "Nordic", "Stalhrim", "Chitin" own; minor modded sets → `*-other`)?
  Depends on research R-5.
  - **A:**
- **Q-ARM-4.** Perk changes (Heavy Armor, Light Armor): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheets [`heavy-armor.perks.md`](heavy-armor.perks.md), [`light-armor.perks.md`](light-armor.perks.md)** (all tree perks of
  this skill, with handles; also `*.perks-removed.md`, `*.perks-ordinator.md` here); see also [`refs/perk-requirements.md`](../../refs/perk-requirements.md) §5. Answer separately for each skill: Heavy Armor, Light Armor.
  - **A:**
