# Grinding analysis

Companion to [`design-v2.md`](design-v2.md). Lists known ways to farm skill XP with little or no
risk or meaningful play: classic vanilla methods (`G-V*`) and new ones that this design
introduces or could introduce (`G-N*`). For each one: whether the **current** design (draft v2,
decisions only, proposals excluded) already prevents it.

**Status legend**

| Status | Meaning |
|---|---|
| **Solved** | Current decisions prevent it. |
| **Partial** | Current decisions reduce it but do not prevent it. |
| **No** | Not addressed. |
| **Open** | Outcome depends on an open question (referenced). |

"Toolkit" in the last column refers to the generic anti-grind toolkit proposed in design §11.2:
**TW** = target worth, **PB** = per-target budget, **ME** = meaningful effect,
**DR** = diminishing repetition.

Important: because vanilla XP awards are used as the **base XP** that we route to sub-skills,
every vanilla grind still works in our mod unless an explicit rule blocks it. Suppressing vanilla
XP by itself solves nothing.

Methods marked *(verify)* may already be patched by Requiem/Lorerim or may not work exactly as
described; check during research.

## 1. Vanilla grinding methods

| ID | Skill | Method | Status | Possible fix |
|---|---|---|---|---|
| G-V1 | 1H / 2H / Archery | Attack a follower or another friendly NPC who does not turn hostile. | No | TW (hostile + in combat only) |
| G-V2 | 1H / 2H / Archery | Attack your own summon or reanimated thrall. | No | TW (exclude teammates/summons) |
| G-V3 | 1H / 2H / Archery | Hit a helpless target: paralyzed, calmed, sleeping, ragdolled, surrendered. | No | TW |
| G-V4 | 1H / 2H / Archery | Hit an essential/unkillable NPC (brawls, quest-essential NPCs) for as long as you want. | No | TW + PB |
| G-V5 | Archery | Repeatedly shoot an unaware or harmless target from stealth. *(verify XP bonus)* | No | TW + PB |
| G-V6 | Heavy / Light Armor | Let a weak enemy (skeever, mudcrab) hit you for a long time. | Partial — damage/level cut-off (design §11.1) | TW + PB |
| G-V7 | Heavy / Light Armor | Get hit in a brawl, by a follower or by a summon. | Partial — cut-off helps only if damage is low | TW |
| G-V8 | Block | Block a weak enemy's hits for a long time. | No | TW + PB (same as armor) |
| G-V9 | Block | Bash spam on a weak or helpless target. | No | TW + PB |
| G-V10 | Smithing | Mass-craft cheap items (iron daggers, leather bracers), temper them. | Solved — XP by material units, not value; daily Smithing bucket (active time); DR by item type; temper XP only when quality rises (skills/smithing/design.md, §11.1) | — |
| G-V11 | Destruction | Cast cheap spells at a follower, a summon or an unkillable target. | No | TW + PB |
| G-V12 | Restoration | Heal yourself while standing in a weak damage source; ward a weak caster's spells. | No | ME (XP by HP actually restored) + TW for the damage source |
| G-V13 | Alteration | Spam self-buffs out of combat (Oakflesh, Candlelight, Detect Life, Transmute). | No | ME (buffs count only in combat / with hostiles near) + DR |
| G-V14 | Illusion | Spam Muffle/Invisibility; Calm/Fear the same weak target repeatedly. | No | ME + DR + TW |
| G-V15 | Conjuration | Summon/dismiss spam; recast bound weapons; Soul Trap on any target. | Partial — with upkeep mechanic ON, summon XP only in combat (skills/conjuration/design.md); bound weapons and soul magic not covered | ME + DR |
| G-V16 | Enchanting | Enchant/disenchant cheap items in bulk. | No | DR; XP by enchantment strength instead of item value |
| G-V17 | (staves) | Cast staves at nothing; recharge with cheap soul gems. | No | ME (staff spell must affect a valid target) |
| G-V18 | Speech | Sell an expensive item and buy it back, repeat. | Partial — XP by saved gold vs reference (skills/speech/design.md), but a loop still saves gold both ways | Net buy/sell of the same item within a time window; DR |
| G-V19 | Speech | Repeated persuade/intimidate checks. Mostly one-shot in vanilla, low risk. | Partial (low risk) | per-topic once flag |
| G-V20 | Alchemy | Mass-craft cheap potions. | Partial — crafting XP is no longer the main source (skills/alchemy/design.md), but see G-N18 | DR |
| G-V21 | Alchemy + Enchanting | Fortify Restoration / Fortify Enchanting feedback loop (power exploit rather than grind). *(verify: likely patched by Requiem)* | No | out of scope unless Lorerim still allows it |
| G-V22 | Lockpicking | Repeatedly pick easy locks in respawning containers; XP for broken picks. *(verify)* | No | DR per lock; XP by lock level relative to sub-skill |
| G-V23 | Pickpocket | Steal from a follower and put it back, repeat. | No | TW-like (exclude followers/teammates) + DR per NPC/item |
| G-V24 | Sneak | Crouch near a sleeping or unaware NPC for hours. | No | XP only when a hostile/detecting NPC is near and actively searching; PB per NPC |
| G-V25 | Sneak | Repeated sneak attacks on sleeping/helpless NPCs. | No | TW |
| G-V26 | (Trainers) | Pay a trainer, then pickpocket the gold back. LoreRim keeps 5 trainings per level (Simply Smart Training carries unused ones over), so this is bounded per level. | Partial — trainers no longer raise main skill; but see G-N9 | cap trainer-granted potential (Q-BT-4) |

## 2. New grinding methods specific to this design

| ID | Mechanic | Method | Status | Possible fix |
|---|---|---|---|---|
| G-N1 | Cross-links | Grind the cheapest/easiest sub-skill (e.g. `dagger`) to raise effective floors of all linked ones (all 1H ×1/2, all 2H ×1/4). | Partial — limited by `floorCap` 75 → linked floor ≤ 37; arguably harmless ("readiness", not skill) | Accept as intended |
| G-N2 | Zone multipliers | **Rotation exploit:** let sub-skills decay and retrain them in the ×1.5 "recall" zone; if main XP is computed after the zone multiplier, rotating weapons gives more main XP per action than sticking with one. | Solved — the main skill is computed from levels, so retraining returns it to the same value (design §4.4) | — |
| G-N3 | Multi-sub-skill events | One action feeds several sub-skills (armor type + material; magic form + archetype + sub-direction; weapon + with-shield). If each counts fully towards the main skill, these main skills grow 2–3× faster. | Solved — Q-XP-1 (a): every sub-skill counts, and `k` is normalized per main skill against a reference path (design §4.1, §4.4.5) | — |
| G-N4 | Armor materials | Rotate armor sets to keep many material sub-skills high (and their perks active). | Partial — decay limits it; only a real problem combined with G-N3 or G-N6 | — |
| G-N5 | Window rule `with-magic` | Alternate the cheapest spell (Candlelight) with weapon hits: every hit within 10 s qualifies, every cheap cast near a hit qualifies too. | No | complementary spell must be hostile and affect an enemy, or have minimum cost |
| G-N6 | Decay | **Token use:** one hit per day with each weapon resets every decay timer, so nothing ever decays — defeats the "no omnipotence" principle. | Solved — idle meter reduced by `c_dec × rawXP`, not reset by a single use (design §5.5) | — |
| G-N7 | Window rule `dual-wield` | Weak off-hand taps to make main-hand hits qualify. | Partial — needs real attacks, but they can be token ones | off-hand attack must land on a valid target (TW) |
| G-N8 | Window rule `with-shield` | Tap block near an enemy without blocking anything to get the ×1.0 instead of ×0.5. | No | "block" = a hit actually blocked |
| G-N9 | Trainers + potential | Many trainings (5 per level, carried over) with stacking potential → buy potential up to 200 for gold, then enjoy the ×1.5 recall zone all the way. | No | cap trainer potential (Q-BT-4) |
| G-N10 | Trainers + bonus | Stack training bonuses indefinitely. | Solved — capped at +30% | — |
| G-N11 | Books + potential | Read the same book many times / multiple copies. | Solved if "once per book" (Q-BT-1); vanilla flag is per base book | — |
| G-N12 | Speech-trading | With strong Speech perks, every purchase and sale "saves" gold relative to the Speech-15 reference, so a buy/sell loop of an expensive item yields XP both ways. | Partial — same as G-V18 | net loops; DR |
| G-N13 | Speech-trading | **Splitting transactions:** with a per-transaction `log(1 + saved)` (proposal in skills/speech/design.md), selling 2000 items saving 1 gold each gives far more XP than one sale saving 2000 (log is concave). | Open — Q-SPE-4 | apply the formula to cumulative saved gold per merchant per day, not per transaction |
| G-N14 | Alchemy consumption | Brew cheap long-duration potions (resists, waterbreathing): `magnitude × duration` is high for them, so drinking cheap potions gives big XP. | No | normalize strength per effect type, or use potion value |
| G-N15 | Alchemy consumption | Drink potions that do nothing (Restore Health at full health). | No | ME (Q-ALC-2) |
| G-N16 | Alchemy poisons | Own poisons ×2 on weak or harmless targets (skeevers, a follower). | Partial — on-hit rule prevents "apply and wipe", target not checked | TW |
| G-N17 | Alchemy crafting | "Large XP below floor" — currently can never trigger (Q-ALC-1). If changed to "below potential", decay + mass crafting could farm it. | Open — Q-ALC-1 | DR |
| G-N18 | Conjuration upkeep | Summons fight something harmless or endless (mudcrab, another summon, essential NPC) to collect "in combat" XP. | Partial — must be in combat | TW + PB on the summon's opponent; XP by damage dealt to valid targets |
| G-N19 | Speech-thuum | Shout at nothing (cooldown limits the rate). | Partial — cooldown | shout must affect a hostile |
| G-N20 | Alteration floor rule | Grind main Alteration with cheap buffs (G-V13) → raises floors of all magic sub-skills. | No — inherits G-V13 | fix G-V13; cap via `floorCap` |
| G-N21 | Jewelry | Fast-growing `jewelry` from mass-crafted cheap rings → its perks. | Accepted by design — "every smith starts as a jeweller"; main gain capped (0→75 ⇒ Smithing 15→35); limited by Smithing bucket + DR | — |
| G-N22 | Smithing materials | Craft one cheap item of every material to raise every material sub-skill. | Accepted — breadth is the intended way to grow Smithing; bounded by bucket and per-material fade `T` | — |
| G-N23 | `bash` (with weapon) | Weapon bash spam on a weak target. | No — same as G-V9 | TW + PB |
| G-N24 | `parry` | Parry a weak enemy for a long time. | No — same as G-V8 | TW + PB |
| G-N25 | `wands-staffs` | Staff spam now feeds Enchanting. | No — same as G-V17 | ME |
| G-N26 | Catch-up ×3 | Raise effective floors cheaply via G-N1, then get ×3 XP in the linked sub-skill. Only matters for main XP if main XP is computed after the zone multiplier. | Solved — catch-up below peak only restores the main skill up to `F(peak)`, never beyond (design §4.4) | — |
| G-N27 | Per-target budget | Rotate weapons, spells or armor pieces on one durable target (essential NPC, brawl, regenerating troll, endless summons) so that every sub-skill gets a fresh budget. | Solved — budget key is (target, category), not (target, sub-skill) (design §11.2, [`refs/per-target-budget.md`](refs/per-target-budget.md)) | — |

## 3. Smithing conversion model (design-v2 §4.4, skills/smithing/design.md)

| ID | Method | Status | Fix |
|---|---|---|---|
| G-S1 | Daily bucket bypass: craft, sleep/wait 24 h, repeat. | Solved — bucket refills only in active game time (design §11.1) | — |
| G-S2 | Decay cycle: conversion depends on level, so letting a sub-skill decay and retraining it yields main XP again, forever. | Solved — the main skill depends on levels, not accumulated XP (design §4.4) | — |
| G-S3 | Craft and break down (if a breakdown mod is installed): nearly free material loop. Not in Lorerim, but common. | Partial — DR by item type + daily bucket (subtracting XP on breakdown rejected: potential/peak are not meant to go down) | — |
| G-S4 | Cheapest item of an expensive material for the category sub-skill (dagger instead of cuirass). | Solved — `XP_cat = tier × units` | — |
| G-S5 | Craft → temper → sell, repeat. | Solved — temper XP only when quality rises, + bucket | — |
| G-S6 | Fast `jewelry` from cheap rings. | Accepted by design (= G-N21) | — |

## 4. Summary

- **Vanilla methods:** 1 of 26 solved (G-V10), 7 partial. Most of them reduce to three patterns:
  **harmless or invalid target** (G-V1…G-V9, G-V11, G-V23, G-V25), **effect without benefit**
  (G-V12, G-V13, G-V14, G-V17) and **repetition** (G-V10, G-V16, G-V18, G-V20, G-V22). Hence the
  proposal of a generic toolkit (TW/PB/ME/DR) instead of per-skill rules.
- **New methods:** the most dangerous ones are structural and decided by single answers:
  G-N3 (Q-XP-1, decided: (a) for all skills), G-N9 (Q-BT-4), G-N13 (Q-SPE-4).
  G-N2/G-N26 are solved by the level model (design §4.4), G-N6 by the decay idle meter (design §5.5).
- Window rules (G-N5, G-N7, G-N8) are cheap to fix: require the complementary event to be
  "real" (landed hit / blocked hit / hostile spell affecting an enemy).
