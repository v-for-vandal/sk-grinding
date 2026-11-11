# Old model: main skill fed by XP transfer (superseded)

Status: **superseded on 2026-11-11** by the level-derived model with main-skill rust
([`design-v2.md`](../design-v2.md) §4.1, §4.4). This note keeps the old model and explains how
the new one differs.

## 1. The old model

- **Flow model.** Sub-skills and main skills were connected by **experience flow**: every XP
  gain of a sub-skill produced a converted XP gain of its parent main skill. Main skill levels
  were **monotonic**: they never went down.
- **Pipeline step:** `if current XP > peak XP: main XP += Δpeak × g(peak level); peak XP =
  current XP`.
- **Peak rule.** Each sub-skill stored `peakXP`, the highest current XP it ever had. Main XP was
  produced **only by XP that raised the peak**. Training back after decay, below the peak,
  produced no main XP. This closed the rotation exploits G-N2, G-N26 and G-S2.
  - Peak is not potential. Potential is also raised by books, trainers and external increments.
    If conversion were tied to potential, a trainer raising potential would make the following
    learning produce no main XP.
- **Conversion function.** For a sub-skill with peak level `L`:
  `g(L) = k × (1 + e^(−T/w)) / (1 + e^((L − T)/w))`; `mainXP += Δpeak × g(L)`.
- **Q-XP-2 (old answer: raw).** When potential was above peak, XP raising the peak got the ×1.5
  recall multiplier. Δpeak was converted **raw** (divided by the zone multiplier), so books and
  trainers made the sub-skill faster but did not boost the main skill.
- **Main XP was stored**, and the main AV was advanced by the engine-style XP curve.
- Normalization (reference path, budgets, `k` solved per group) and the script
  `scripts/normalize_conversion.py` were the same as now.

## 2. Why it was replaced

| Aspect | Old: XP transfer | New: level-derived + rust |
|---|---|---|
| Main skill can go down | no, monotonic | yes, it rusts when the whole skill is not used (design §4.4) |
| Vanilla / Requiem / Ordinator effects that read the main skill | never reflect decay | follow the main skill down and up without knowing about sub-skills |
| "No omnipotence" | relied only on sub-skill decay and sub-skill perks (Q-OTH-4) | also the main skill: abandoned skills lose vanilla effects |
| State | main XP accumulated and stored | main is recomputed from sub-skills; only rust `R`, idle meter `U_main` and the start baseline are stored |
| Rotation exploits (G-N2, G-N26, G-S2) | closed by the peak rule (a special rule) | closed by construction: main depends on levels, not on accumulated XP, so decaying and retraining cannot produce extra main skill |
| Experience skill cap (Q-XP-8) | XP earned at the cap had to be stored and released later | the computed value is clamped; it rises at once when the cap rises |
| Books/trainers and main skill (Q-XP-2) | "raw" conversion: they did not speed up the main skill | they speed up sub-skills (×1.5 recall up to potential), and the main skill follows the sub-skill levels, so the main skill also grows faster. Accepted: comparable to vanilla books raising the skill directly |
| Legendary | could keep working (main XP model) | disabled: a computed main skill cannot be reset |
| Complexity | simple, no extra state | extra rust state, hysteresis on writes, external sources must go through potential |

## 3. Relation between the two

The old model is a special case of the new one. With the peak rule the total main XP over the
game was always `Σ_i M_i(peak_i)`, with `M_i(L) = k_i · Σ_{l<L} cost_sub_i(l) · gn_i(l + 0.5)`,
because the conversion depended only on the level. The new model computes the main skill from
`Σ_i M_i(max(current_i, peak_i − R))`. With the rust `R ≡ 0` it gives exactly the old
behaviour.

## 4. Old design-v2 §4.4 text (verbatim)

```markdown
### 4.4 Conversion model (decided)

- **Peak rule.** Each sub-skill stores `peakXP`, the highest current XP it has ever had. Main
  XP is produced **only by XP that raises the peak**. Training back after decay (below peak)
  produces no main XP. This closes the rotation exploits G-N2, G-N26 and G-S2.
  - Peak is **not** potential. Potential is also raised by books, trainers and external
    increments (§4.1, §10). If conversion were tied to potential, a trainer raising potential
    would make the following learning produce *no* main XP, so training would hurt the main
    skill. Peak only follows the player's own practice.
- **Conversion function.** For a sub-skill with peak level `L`:
  `g(L) = k × (1 + e^(−T/w)) / (1 + e^((L − T)/w))`, a normalized logistic: `g(0) = k`,
  halved around `L ≈ T`, width `w`. `mainXP += Δpeak × g(L)`.
  - `k`, `T`, `w` are per sub-skill (config). Usually `k` is shared by a group of sub-skills
    (e.g. all Smithing materials), and `T` varies with how hard the sub-skill is.
  - Each sub-skill can contribute only a **finite** amount of main XP over its lifetime, so a
    high main skill requires breadth (several sub-skills). How much breadth is set per skill
    with `T`: high for skills where a specialist should go far (weapons), lower for skills where
    breadth is the theme (crafting, magic).
  - The sub-skill itself keeps growing for its own perks; only its contribution to the main
    skill fades.
- **Normalization.** (unchanged, see the current §4.4)
```
