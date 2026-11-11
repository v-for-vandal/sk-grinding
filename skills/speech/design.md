# Speech

> Part of the design: [`design-v2.md`](../../design-v2.md) (was §9.1). Bare `§N` references point to design-v2.md. Index of skills: [`skills/README.md`](../README.md).

**Sub-skills (parent: Speech):** `speech-intimidation`, `speech-persuasion`, `speech-trading`,
`speech-thuum`.

- `speech-intimidation` / `speech-persuasion`: XP for successful intimidate / persuade checks
  in dialogue. **Bribery** is part of `speech-persuasion` (decided, Q-SPE-1).
- `speech-thuum`: XP for using shouts. This is a **new link** (vanilla shouts give no Speech XP);
  justified because Lorerim's Speech tree contains the Thu'um perks. Decided (Q-SPE-2):
  - XP scales with the **number of words** used;
  - an **offensive** shout (hostile magic effect) gives XP only when it affects a hostile
    actor; a non-offensive shout gives XP on use;
  - diminishing returns **per shout** (there are few shouts, so repeating one teaches less).
- `speech-trading`: XP for buying and selling, proportional to the gold saved compared to a
  **reference price** = price at Speech 15 with neutral modifiers (no perks, no disposition/
  faction modifiers). Saving 2 gold → very little XP, saving 2000 → a lot. Decided (Q-SPE-3,
  Q-SPE-4):
  - `saved = actualSellPrice − referenceSellPrice` (for buying: `referenceBuyPrice −
    actualBuyPrice`); a negative result counts as 0;
  - the log is applied to the **cumulative** saved gold `S` per merchant per game day: a
    transaction gives `kTrade × (log(1 + S_after) − log(1 + S_before))`, so splitting one sale
    into many small ones gives nothing extra (grinding-analysis G-N13); buy/sell loops are
    netted first (G-V18, G-N12).
  - Floor of `speech-trading` grows faster (`floorRate` override, e.g. 0.5).

- **Q-SPE-5.** Perk changes (Speech): describe all perks of this skill that must change
  compared to the base (Lorerim/Requiem) perk trees, plus any new ones. For each perk, state:
  (1) whether it has requirements on sub-skill levels (which sub-skill, which value — see
  Q-DYN-10 — and the threshold); (2) whether it is a **purchasable** perk (in the main-skill
  tree, costs a perk point) or an **automatic** perk owned only by a sub-skill (added/removed
  by the DLL at a threshold, §5.6). **Answer in the worksheet [`perks.md`](perks.md)** (all tree perks of
  this skill, with handles; also `perks-removed.md`, `perks-ordinator.md` there); see also [`refs/perk-requirements.md`](../../refs/perk-requirements.md) §5.
  - **A:**
