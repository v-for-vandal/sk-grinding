# Vanilla skill XP data

Reference note for [`design-v2.md`](../design-v2.md): Q-ML-7 (pace `P_skill` for the rust
`c`), Q-DYN-15 (`c_dec`), Q-XP-9 (raw use points), research R-22.

**Status (2026-11-11).** UESP could not be fetched directly (HTTP 403); the first version of this
note was built from search-engine snippets of UESP pages. **R-22 (static Mutagen dump of the
LoreRim profile, 3492 plugins) has since verified it against the plugins:**

- AVIF values (§2): all confirmed from `Skyrim.esm` + `Update.esm`.
- GMSTs (§3): vanilla and LoreRim values come from the plugin chains, not from UESP.
- Not verifiable from plugins: the formulas themselves (engine code) and GMSTs that exist in no
  plugin (engine defaults, §5).

The dump is local, in `~/tmp/research2/` (`skill-xp-report.md`, `tables/skill-avif-chain.tsv`,
`tables/gmst-skill-xp-chain.tsv`, `tables/gmst-skill-xp-comparison.tsv`). Perk override chains
from the same run: §6.

Cross-check: the Smithing values (Improve Mult 0.25, Improve Offset 300, total 15→100 ≈ 91,601)
match `config/skills/smithing.toml` and the N ≈ 91.6k computed by the normalization script.

## 1. Formulas (UESP Skyrim:Leveling)

| Item | Formula |
|---|---|
| XP to go from level L to L+1 | `ImproveMult × L^1.95 + ImproveOffset` (`fSkillUseCurve = 1.95`) |
| XP per use | `UseMult × (base XP × skill-specific multipliers) + UseOffset` (UESP: "verification needed") |
| Character XP per skill-up | `new skill level × fXPPerSkillRank` (default 1). Not used in LoreRim (Experience mod) |
| Character level-up cost | `(level + 3) × 25` |

The AVIF record's `AVSK` subrecord holds float[4]: Use Mult, Use Offset, Improve Mult,
Improve Offset. Mutagen calls them `UseMult`, `OffsetMult`, `ImproveMult`, `ImproveOffset`.

## 2. AVIF values (vanilla, patched Skyrim.esm + Update.esm; verified in R-22)

FormKeys `00044C:Skyrim.esm` (One-Handed) … `00045D:Skyrim.esm` (Enchanting). **Illusion's
EditorID is `AVMysticism`** (`00045B:Skyrim.esm`); there is no `AVIllusion`.

| Skill | Use Mult | Use Offset | Improve Mult | Improve Offset | XP 15→100 |
|---|---|---|---|---|---|
| One-Handed | 6.3 | 0 | 2 | 0 | 528,804 |
| Two-Handed | 5.95 | 0 | 2 | 0 | 528,804 |
| Archery | 9.3 | 0 | 2 | 0 | 528,804 |
| Block | 8.1 | 0 | 2 | 0 | 528,804 |
| Heavy Armor | 3.8 | 0 | 2 | 0 | 528,804 |
| Light Armor | 4 | 0 | 2 | 0 | 528,804 |
| Smithing | 1 (or 160, see below) | 0 | 0.25 | 300 | 91,601 |
| Alteration | 3 | 0 | 2 | 0 | 528,804 |
| Conjuration | 2.1 | 0 | 2 | 0 | 528,804 |
| Destruction | 1.35 | 0 | 2 | 0 | 528,804 |
| Enchanting | 900 | 0 | 1 | 170 | 278,852 |
| Illusion | 4.6 | 0 | 2 | 0 | 528,804 |
| Restoration | 2 | 0 | 2 | 0 | 528,804 |
| Alchemy | 0.75 | 0 | 1.6 | 65 | 428,568 |
| Lockpicking | 45 | 10 | 0.25 | 300 | 91,601 |
| Pickpocket | 8.1 | 0 | 0.25 | 250 | 87,351 |
| Sneak | 11.25 | 0 | 0.5 | 120 | 142,401 |
| Speech | 0.36 | 0 | 2 | 0 | 528,804 |

- **Smithing Use Mult: resolved.** `Skyrim.esm` has 160, `Update.esm` overrides it with 1
  (item-value-based XP). Patched vanilla is **1**.
- **In LoreRim** all 18 winning AVIF records have `UseMult = 0` and `ImproveOffset = 9999`
  (confirmed). Winners: mostly `Ordinator - Perks of Skyrim.esp`; Block, Sneak, Speech,
  Conjuration, Illusion: `LoreRim - Spells and Magic Effects.esp`; Destruction, Restoration:
  `Big Tweaks.esp`. The Improve values above are therefore vanilla only.

## 3. Base XP per action ("base XP" in the formula above)

| Skill | Base XP |
|---|---|
| One-/Two-Handed | base weapon damage. Perks, enchantments, tempering and power attacks do not raise it. UESP also mentions a "×0.7" factor for Two-Handed (meaning unconfirmed) |
| Archery | base damage of the bow. Arrows, draw, sneak, perks and tempering do not count |
| Block | 1 per point of raw damage blocked. Settings (secondary, engine defaults): `fBlockSkillUseMult` 2, `fWeaponBlockSkillUseMult` 1, `fShieldBashSkillUseBase` 5, `fWeaponBashSkillUseBase` 1 |
| Heavy / Light Armor | 1 per point of raw damage received. More pieces worn gives more XP; armor rating does not matter |
| Alteration, Conjuration, Destruction, Illusion | base magicka cost of the spell (at skill 0, no perks). Dual casting gives the same as a single cast. Destruction XP is lower on higher difficulties (Legendary: −75%) |
| Restoration | 1 per point healed (healing spells); 1 per magicka spent (other spells) |
| Smithing: craft | `quantity × (25 + 3 × itemValue^0.65)` (GMST `fConstructibleSkillUse*`) |
| Smithing: temper | UESP: `3.8 × ΔitemValue^0.5 × ΔQ^0.5`. The plugin GMSTs do not fit that form: `fTemperingSkillUseMult` 1, `…ItemValMult` 8, `…ItemValExp` 0.6, `…ItemValConst` 25 (Update.esm, not changed by LoreRim); exact formula unverified. Smelting gives no XP |
| Alchemy | 1 per gold of value created, plus 1 per potion |
| Enchanting | 1 per item enchanted (value, effect and soul size do not matter); disenchant: 1 per 400 gold of enchantment value (disputed); recharge: 0.05 / 0.1 / 0.2 / 0.4 / 0.6 by soul size |
| Lockpicking | success: 2 / 3 / 5 / 8 / 13 (Very Easy … Very Hard, `fSkillUsageLockPick*`); broken pick: 0.25. Re-picking a lock gives nothing |
| Pickpocket | 1 per gold of value stolen (`fPickpocketSkillUsesCurve` 0.8, effect unconfirmed) |
| Sneak | 2.5 for becoming hidden near an enemy; 0.625 per second hidden (`fSkillUsageSneakPerSecond`); sneak attack: melee 30, ranged 2.5 (`fSneakAttackSkillUsage*`), flat |
| Speech | 1 per gold in transactions; persuade, intimidate or bribe: `75 × Speech level` (as UESP states it) |

**LoreRim overrides** that change these raw amounts (R-22, full override chains; vanilla =
`Skyrim.esm` + `Update.esm`):

| GMST | Vanilla | LoreRim winner | Effect |
|---|---|---|---|
| `fConstructibleSkillUseMult` / `Exp` / `Const` | 3 / 0.65 / 25 | 0.5 / 1 / 0 (Requiem.esp) | craft XP `25 + 3 × V^0.65` → `0.5 × V` (V = item value; formula matched against a mod script, not the engine) |
| `fSneakAttackSkillUsageMelee` | 30 | 60 (Requiem.esp) | ×2 |
| `fSneakAttackSkillUsageRanged` | not in any vanilla plugin (engine default; UESP: 2.5) | 5 (Requiem.esp, new record `29F52D:Requiem.esp`) | probably ×2 |
| `fDisenchantSkillUse` | 0.0025 | 0 (Requiem.esp) | no XP for disenchanting |
| `fSkillUsageRechargeMult` | 0.0002 | 0.001 (Requiem - Magic Redone.esp) | recharge XP ×5 |
| `fAlchemySkillFactor` | 1.5 | 2 (Requiem - Alchemy Redone.esp) | skill-dependence of potion strength, not an XP multiplier |
| `fSkillUsageLockPick*` (VeryEasy…VeryHard, Broken) | 2 / 3 / 5 / 8 / 13, 0.25 | **0 (StaticSkillLeveling.esp)** | raw Lockpicking use points are zero while SSL is loaded (R-16, Q-XP-9) |
| `iSneakSkillUseDistance` | not in vanilla plugins | 800 (Requiem.esp) | — |

Not changed by LoreRim: `fSkillUseCurve` 1.95, `fSkillUsageSneakPerSecond` 0.625,
`fPickpocketSkillUsesCurve` 0.8, `fEnchantingSkillFactor` 1.25, all `fTemperingSkillUse*`.

## 4. Implications

- **Raw use points depend on GMSTs that LoreRim mods changed.** The base XP our hook receives
  (Q-XP-9) is LoreRim's, not vanilla. For example, Smithing craft XP under Requiem is
  `0.5 × itemValue`. If SSL stays in the load order (R-16), our ESL must restore the
  `fSkillUsageLockPick*` values.
- For `P_skill` (Q-ML-7) these tables give **XP per action**, not XP per hour. Turning them into
  a pace needs an estimate of actions per hour (hits, spells, crafts), or the playtest log
  (R-17). UESP has no estimate of skill-ups per hour in normal play.
- Vanilla XP 15→100 for combat and magic skills is ≈ 529k / UseMult "actions worth of base XP".
  For example, One-Handed needs ≈ 84k points of base weapon damage.

## 5. Engine defaults to read in game

These GMSTs exist in **no** active plugin; their value comes from the engine (or a DLL). The user
reads them with the console (read-only):

```text
getgs fSkillUsageSneakHidden
getgs fBlockSkillUseMult
getgs fWeaponBlockSkillUseMult
getgs fShieldBashSkillUseBase
getgs fWeaponBashSkillUseBase
getgs fXPPerSkillRank
```

and, to compare runtime with the plugins: `getgs fSkillUsageSneakPerSecond`,
`getgs fSkillUsageRechargeMult`, `getgs fSkillUsageLockPickBroken`.

## 6. Other R-22 findings

- **Experience 3.7.3** (`LoreRim - MCM and INI Settings/SKSE/Plugins/Experience.ini`):
  `bEnableSkillXP = false` (no character XP from skill-ups), `bEnableReading = false`,
  `bEnableKilling = true`; skill cap `fSkillCapBase = 50`, `fSkillCapMult = 1`,
  `bUseRacialCaps = false`, so **cap = 50 + character level** (51 at level 1, 100 at level 50),
  confirmed by the mod description. Character XP: location discovery 10, clearing 100 (Fort /
  Nordic ruin / Dwemer ruin / dragon lair 200); quests 200 (main, Daedric, DLC), 150 (Civil War),
  100 (factions, side), 0 (misc).
- **SSL.** No master switch found. The quest script with `bDisableSkillLeveling = 1` calls
  `ModSkillsLeveling(0.0)` (zeroes skill-use multipliers); `-1` restores only multipliers saved
  earlier, so it does **not** guarantee vanilla. Stopping new points: MCM "points per level" = 0
  and "level multiplier" = 0, plus zeroing the bank `SSL_PointsLeftAfterLvlUp`
  (`000B12:StaticSkillLeveling.esp`), because
  `points = multiplier × playerLevel + pointsLeft + pointsPerLevel`. The plugin has 7 GMSTs,
  11 GLOBs and the start quest `StaticSkillLevelingQuest` (`000D7E:StaticSkillLeveling.esp`).
  Two MCM layers in the files (defaults: 5 points / costs 1-1-1-1; user layer: 30 points / costs
  1-2-3-6); the active values of a save must be read in MCM.
- **Jail.** `Less Jail.esp` (Requiem - Jail Duration Fix) sets `iCrimeMaxNumberofDaysinJail`
  100 → 7 and `iCrimeDaysInPrisonMod` 10 → 1000 (both created by Requiem.esp): roughly one day
  per 1000 gold of bounty, at most 7 days. How the engine maps days to lost skill progress was not
  established.
- **Legendary.** Not disabled. Requiem sets `fLegendarySkillResetValue = 5` and only adds
  warning texts ("Requiem does not support this feature"). Our mod must block it itself
  (design §4.4.4).
- **Perks.** 842 PERK records have more than one version; 983 perks / 2418 versions exported
  (all tree perks plus `NextPerk` chains); 341 perks are overridden by two or more plugins besides
  the owner. Tables: `perk-override-chain.tsv`, `perk-override-diff.tsv`, `perk-conflicts.tsv`,
  `avif-perktree-chain.tsv`, `avif-perktree-diff.tsv`. Mutagen 0.54.4 could not decode the
  effects of three versions (`00080E:Requiem - Special Feats.esp`,
  `2474CB:Wintersun - Faiths of Skyrim.esp` in Wintersun - Reqtificated,
  `00082D:Wintersun - Reqtificated.esp`).
- Final perk-tree owners: Ordinator for most skills; Block: LoreRim - Weaponmaster (then
  LoreRim - Spells and Magic Effects, same tree); Speech: Requiem - Special Feats; Conjuration,
  Illusion: Requiem - Magic Redone; Destruction, Restoration: Big Tweaks; Sneak: LoreRim - Spells
  and Magic Effects.
