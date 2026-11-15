# R-30: LoreRim book trait and how skill books grant points

- **For:** design-v2 §10.1 (skill books → potential, compatibility with LoreRim's book trait).
- **Status:** web research done 2026-11-14 (Nexus, UESP, Fandom returned 403; facts from the
  LoreRim site, CommonLibSSE-NG and Custom Skills Framework sources, and search snippets); the
  record checks below are open.

## Findings

**Which mod.** LoreRim's traits come from **Biggie Traits** (Shazdeh, dabiggieboss; Nexus
136384). The installed profile (dump 2026-11-11) has *Biggie Traits* 1.0.35, *Biggie Traits -
Requiem* 1.0.15, *Additional Traits for Biggie Traits - Requiem Patch* and LoreRim's own
override *[LoreRim] Traits* (d2025.8.22).

- The book trait is **Skilled**. LoreRim's trait page
  (<https://www.lorerim.com/guides/character/traits>) says: "Reading skill books grants an extra
  skill point and trainers cost 50% less, however every 5 levels you don't gain a perk point",
  i.e. **2 per book, not 5**. *[LoreRim] Traits* may change the value: Biggie Traits 1.0.32 moved
  hard-coded values into globals *(unconfirmed)*. Read the in-game text of the trait.
- Other book bonuses in LoreRim: Wintersun, follower of **Xarxes**: "2 additional points from
  reading skill books" (<https://www.lorerim.com/guides/character/divine>); Dragonborn's
  **Scholar's Insight** (Black Book): +1 point. Skilled + Xarxes may give 1 + 1 + 2 = 4
  *(stacking unconfirmed)*.
- Biggie Traits can also cancel a book's gain and hide its popup (Giantkin, 1.0.30 / 1.0.33),
  so it touches book reading with scripts or native code *(unconfirmed how)*.

**Engine side (well sourced).**

- Perk entry point **Adjust Book Skill Points** (`RE::BGSEntryPoint::ENTRY_POINT::
  kAdjustBookSkillPoints = 5`, argument: perk owner) modifies the points a skill book gives. A
  +1 trait can be a plain perk on this entry point, no script *(unconfirmed for Skilled and
  Scholar's Insight)*.
- `bool TESObjectBOOK::Read(TESObjectREFR*)`: `RELOCATION_ID(17439, 17842)`; book data:
  `OBJ_BOOK::Flag::kAdvancesActorValue`, `kHasBeenRead`, `TeachesSkill()`, `GetSkill()`.
- **Proven hook site:** Custom Skills Framework (Exit-9B, MIT),
  `src/CustomSkills/Hooks/SkillBooks.cpp`, patches `Read` (AE 17842) at +0x103 (checks bytes
  `41 BD FF FF FF FF`): its callback returns early for read books, evaluates
  `kAdjustBookSkillPoints` on base 1.0, and can skip the vanilla skill grant. SE offset:
  Fuzzlesz/CustomSkills-1.5 fork *(unverified)*.
- Whether vanilla `Read` sets the level or adds a level's worth of XP is not known.

## Consequence for the design (proposal)

- Hook `TESObjectBOOK::Read` at the Custom Skills site: for a skill book not read before,
  evaluate `kAdjustBookSkillPoints` on base 1 → `n` points (vanilla 1, Skilled 2, more with
  other perks), skip the vanilla grant, and raise potential by `bookPotential × n` (`+2 × n`).
  So every **perk-based** bonus is kept automatically, without knowing the trait.
- **Script-based** bonuses (e.g. Xarxes, if it calls `IncrementSkill`) arrive as external
  increments and already become potential (§4.1). Check that they are not counted twice.

## Open checks

1. ~~In-game text of *Skilled*~~: the "+5" was a memory slip (the user mixed it up with
   "every 5 levels"); +1 per book stands. The proposal above is accepted (design-v2 §10.1).
2. xEdit: the Skilled perk in `BiggieTraits.esp` / `[LoreRim] Traits` (entry point or script),
   Scholar's Insight MGEF `DLC2BlackBookSkillBookEffect`, Xarxes' script call.
3. Runtime: [`playtesting/skill-books.md`](../playtesting/skill-books.md).
