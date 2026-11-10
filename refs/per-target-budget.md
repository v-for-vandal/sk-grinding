# Per-target budget (PB)

Reference note for [`design-v2.md`](../design-v2.md) §11.2 (anti-grind toolkit, item 2) and
[`grinding-analysis.md`](../grinding-analysis.md). Open questions are in design §11.2 (Q-AG-3…Q-AG-9).

**Idea:** one target (an NPC, a creature, a merchant, …) can only give a limited amount of XP. While
the target is "fresh" its XP is full. Once its budget is used up, XP falls off quickly. Over game
time the budget refills.

## 1. Decisions

| # | Decision |
|---|---|
| D-1 | The budget key is **(target, category)**, not (target, sub-skill). This closes **G-N27**: switching weapons, spells or armor pieces on the same durable target (essential NPC, brawl, regenerating troll, endless summons) to get a fresh budget for each sub-skill. |
| D-2 | **Combat** sources use a **damage budget**. **Non-combat** sources use an **XP budget**. |
| D-3 | Budget state is saved to the **SKSE co-save**, so save/reload does not reset budgets. |
| D-4 | **Debug:** the MCM Debug page and a console command show `spent / B` (and the current falloff multiplier `m`) for the target under the crosshair, per category. |

## 2. Data

```cpp
enum class BudgetCategory : uint8_t { /* see §3, final list per Q-AG-3 */ };

struct BudgetKey {
    RE::FormID     target;    // reference FormID of the actor / object
    BudgetCategory category;
};

struct BudgetEntry {
    float spent;              // damage (combat) or raw XP (non-combat) already "used"
    float lastGameHour;       // game time of last update, for lazy refill
};

std::unordered_map<BudgetKey, BudgetEntry> budgets;   // needs a hash for BudgetKey
```

- Who the `target` is:
  - offense (weapon, offensive spell, poison, bash): the actor being hit;
  - defense (armor, block, parry, ward): the **attacker**;
  - non-combat: the NPC, merchant or object the action is performed on.
- Pruning: drop an entry once its `spent` has refilled to 0 (checked lazily and on save).
  The map stays small (tens to hundreds of entries).
- Co-save: versioned record `'PTBG'` with entry count + entries. Dynamic references (FormID `FF…`)
  may not survive a reload; drop entries whose FormID no longer resolves on load.

## 3. Categories (proposal, final list per Q-AG-3)

| Category | Sources | Budget type | Target |
|---|---|---|---|
| `offense-melee` | One-Handed / Two-Handed hits, bash | damage | victim |
| `offense-ranged` | Archery | damage | victim |
| `offense-magic` | Destruction and other hostile damaging spells, staves | damage | victim |
| `offense-poison` | poisons (on hit) | damage | victim |
| `offense-summon` | damage dealt by player's summons/thralls (Conjuration upkeep, G-N18) | damage | victim |
| `defense-physical` | armor (being hit), block, parry | damage received | attacker |
| `defense-magic` | wards, magic resistance | damage absorbed/received | attacker |
| `pickpocket` | pickpocket | XP | NPC |
| `speech` | trading, persuasion | XP | merchant / NPC |

Lockpicking, smithing, enchanting and alchemy crafting have no meaningful "target". There the
**diminishing repetition** tool (DR) applies instead (per lock, per item).

## 4. Budget size

### 4.1 Combat: damage budget

- Offense: XP is granted only for damage up to `B = kOff × targetMaxHealth` (`kOff ≈ 1.5–2`,
  per category).
  - A normal enemy: killed → (almost) all damage counted.
  - Regeneration / healed enemies: `k > 1` leaves some room.
  - Essential NPCs, brawls, training dummies: XP stops after 1.5–2 health bars.
- Count the **effective** damage: clamp each hit to the target's remaining health (overkill on a
  one-shot does not count beyond what the target had).
- Defense: XP is granted only for damage received from one attacker up to `B = kDef × S(attacker)`,
  where `S` is the attacker's strength measure (Q-AG-5). Skeevers and mudcrabs stop giving
  armor/block XP quickly.
- The vanilla XP amount per hit is not changed by the budget, only multiplied by `m` (§5). The
  budget decides **how long** a target keeps giving XP.

### 4.2 Non-combat: XP budget

- `B = base_cat × f(targetLevel)` raw XP (before zone multipliers), with `base_cat` from config.
- Example: pickpocket, the same NPC gives full XP for the first few steals per refill period.
  Speech: one merchant gives full XP up to a daily amount (also addresses G-N13; see Q-SPE-4).

## 5. Falloff

Smooth, no hard cliff:

```
x = spent / B
m = 1                                , x ≤ 1
m = max(mMin, 1 / (1 + (x − 1)^p))   , x > 1        // p ≈ 2, mMin = 0 … 0.05
```

Each event: `m` is computed from `spent` **before** the event, then `spent += amount`. For combat
events the amount is the effective damage, for non-combat events it is the raw XP.

## 6. Refill (leaky bucket)

Lazy, using game time (same approach as decay, design §5.5):

```
dt     = nowGameHour − entry.lastGameHour
spent  = max(0, spent − B × dt / refillHours)
lastGameHour = nowGameHour
```

- `refillHours` defaults to 24 and can be set per category.
- Long boss fights are fine: the budget scales with the boss's health.
- Dungeon respawn (10–30 days) is far longer than `refillHours`, so it is not a problem.

## 7. Place in the XP pipeline

```
vanilla XP (base) → target worth → per-target budget (× m) → routing to sub-skills → zone multipliers
```

- `spent` records **raw** amounts, before zone multipliers. This is consistent with Q-XP-2.
- Context problem: the `AddSkillExperience` hook does not know the target. Each XP source needs a
  **context provider**: a hit, magic-hit, block or barter hook/event that remembers the last
  `(target, category, effective damage)` for the current frame or event. The XP hook then
  consumes it. If no context is found, the event goes through without a budget and is logged
  (in the debug log).

## 8. Config (per category)

```ini
[Budget.offense-melee]
enabled     = true
type        = damage        ; damage | xp
k           = 1.75          ; damage budget multiplier (type = damage)
base        = 0             ; XP budget base (type = xp)
p           = 2.0
mMin        = 0.0
refillHours = 24
```

## 9. Debug (decided, D-4)

- Crosshair target: per category `spent`, `B`, `x`, `m`, and time until full refill.
- Console command to reset the budget of the crosshair target, or of all targets.
- The verbose XP log (design §13) includes `category`, `target`, `m` for every event.

## 10. Reuse: daily crafting bucket

Crafting takes no game time, so crafting skills (Smithing, decided; Enchanting and Alchemy
proposed, Q-AG-13) get a **daily bucket** built on the same machinery:

- key: (player, category), e.g. (player, `craft-smithing`); one bucket per main skill, shared
  by all its sub-skills;
- XP budget (non-combat type), counted in raw XP before zone multipliers;
- same falloff curve (§5);
- **refill uses active game time only**: `dt` excludes sleeping, waiting, fast travel and jail
  (track via the Sleep/Wait menu and fast-travel start/end events, and the jail state).
  Otherwise "craft, sleep 24 h, repeat" would bypass the bucket (grinding-analysis G-S1).
  Keep an "active game hours" counter in the co-save and use it instead of game time for these
  buckets. This counter can also be useful for the per-target budgets.
