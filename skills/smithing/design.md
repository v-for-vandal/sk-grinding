# Smithing

> Part of the design: [`design-v2.md`](../../design-v2.md) (was §7.4). Bare `§N` references point to design-v2.md. Index of skills: [`skills/README.md`](../README.md).

**Sub-skills (parent: Smithing):**

- **material** sub-skills: one per material (table below);
- **category** sub-skills: `one-handed-smithing`, `two-handed-smithing`, `light-armor-smithing`,
  `heavy-armor-smithing`, `clothing-smithing`, `bow-smithing`, `crossbow-smithing`. Arrows go to
  `bow-smithing`, bolts to `crossbow-smithing`;
- `jewelry` (rings, necklaces, circlets): a **special category** (decided, Q-SMI-7). Crafting
  jewelry feeds only `jewelry`, never a material sub-skill (a silver amulet gives nothing to
  `silver`). Gems in the recipe count only for `jewelry`.

**XP sources:** only **crafting** and **improving (tempering)**. Smelting and tanning give nothing.

- Material of an item: item keywords (`WeapMaterial*` / `ArmorMaterial*`), falling back to the
  main ingredient of the recipe, with `other-smithing` as the last resort.
- **Crafting** an item grants raw XP:
  - material sub-skill: `XP_mat = xpPerUnitMat × units`;
  - category sub-skill: `XP_cat = xpPerUnitCat × tier(material) × units`;
  - `units` = number of units of the main material in the recipe (COBJ). A recipe that makes a
    bundle (arrows, bolts) is one craft of one item: its `units` count once (decided, Q-SMI-10).
    Large items cost more
    material and give more XP; expensive materials teach the category more. XP per unit is the
    same for every item, so there is no "best item to spam". An optional per-item override
    table is allowed.
- **Improving** grants XP only when the item's quality level actually increases (G-S5). It uses
  the same formulas, applied to the units of the temper recipe, × `temperFactor` (config).
- **Tier is our concept, not a game property.** The engine has no material tier. Tiers live in the
  config table below. The default tier can be derived from the perk the recipe requires (Steel,
  Elven, Advanced, Glass, Ebony, Daedric, Dragon Smithing…).
- No cross-links within Smithing.

**Conversion into main Smithing** (model §4.4):

- Each sub-skill converts its own XP (contributions add up; Q-XP-1 decided as (a)).
- `k`: one value for all materials, another for all categories, a separate one for `jewelry`.
- `T` (where conversion fades): the same for all categories. For materials it grows with
  difficulty: iron is learned quickly and stops teaching Smithing early, glass teaches longer.
  `jewelry` fades very early.
- **Reference path:** `jewelry` + 3 categories (equal to each other) + `steel` + `orcish` +
  `glass`, each 0→75 ⇒ Smithing 15→100. `jewelry` 0→75 alone ⇒ Smithing 15→35.
- With the placeholder values in [`scripts/smithing.toml`](../../scripts/smithing.toml) (vanilla-like
  curve): N(15→100) ≈ 91.6k main XP; `jewelry` ≈ 9.5% of N; the rest is split 50/50 between
  categories and materials (Q-SMI-6). Reaching 200 in every path sub-skill gives Smithing ≈ 117.
  Training all sub-skills to 75 gives ≈ 150 (the main-skill cap is Q-DYN-8).

**Anti-grind (decided, details in §11):** daily Smithing XP bucket that refills only in active
game time (G-S1); diminishing repetition by item type (G-S3); improving gives XP only when
quality rises (G-S5).

**Material table** (Q-SMI-5): see the worksheet
[`refs/lorerim/materials.md`](../../refs/lorerim/materials.md), with 111 material / armor-set keywords
actually used in LoreRim, record counts, examples and proposals for sub-skill and tier.
Notes:
- Requiem armor keywords are **set families** (`REQ_ArmorSet_Guard`, `…_Vigilant`, …), not
  physical materials. For Smithing, the material of a set comes from its recipe ingredients.
- **Recipes (COBJ) are not in the research dump yet** (R-15). They are needed for: the set →
  material mapping, `units` per recipe (XP formulas above) and temper recipes.
- Placeholder tiers and `T` values are in `scripts/smithing.toml` (iron 1 … dragon 10).

- **Q-SMI-5.** Fill in the materials worksheet
  ([`refs/lorerim/materials.md`](../../refs/lorerim/materials.md)): Smithing sub-skill and tier
  (category XP weight) per keyword, `T` per sub-skill, and merges or splits (hide/leather,
  steel/steel plate, dragonbone/dragonscale, CC and modded materials).
  - **A:**
- **Q-SMI-6.** How is the budget left after `jewelry` split between categories and materials?
  The placeholder is 50/50, which means one category 0→75 gives about the same as one mid
  material.
  - **A:**
- **Q-SMI-8.** What counts as the "main material" in recipes with several ingredients (glass =
  malachite + moonstone; steel plate = steel + corundum; leather strips everywhere)? Proposal:
  sum the units of all ingredients that belong to the item's material (ingots or ore of that
  material). Ignore generic ones (leather strips, firewood). Fallback: the most numerous
  ingredient.
  - Your comment: "Creating glass from malachite and moonstone is smelting and it does not give
    XP. Recipe for armor should only have 'glass ingot' as ingredient, should it not?"
  - **Clarification:** the question is about the crafting recipe, not smelting. There is no
    single "glass ingot": in vanilla, glass items are made from two ingots, **Refined Malachite**
    and **Refined Moonstone** (plus leather strips); Elven uses Refined Moonstone + Iron Ingot,
    Steel Plate uses Steel + Corundum, Orcish Orichalcum + Iron, Daedric Ebony + Daedra Heart.
    (LoreRim's actual recipes are not dumped yet, R-15.) So for a glass cuirass, does `units`
    count both malachite and moonstone ingots, only malachite, or the most numerous one? The
    proposal above counts every ingot of the item's own material family (malachite + moonstone
    for glass; for Elven only moonstone, since iron belongs to another material).
  - **A:**
- **Q-SMI-9.** `temperFactor` and the temper XP source. Proposal: same formulas applied to the
  temper recipe units, `temperFactor = 1`. Temper recipes are already cheap (one unit per step),
  and they give XP only when quality rises.
  - **A:**
- **Q-SMI-4.** Perk changes (Smithing): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`perks.md`](perks.md)** (all tree perks of
  this skill, with handles; also `perks-removed.md`, `perks-ordinator.md` there); see also [`refs/perk-requirements.md`](../../refs/perk-requirements.md) §5.
  - **A:**
