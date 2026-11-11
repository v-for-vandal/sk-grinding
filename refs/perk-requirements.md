# Sub-skill requirements on existing perks

Reference note for [`design-v2.md`](../design-v2.md) §5.6 (Q-DYN-10…Q-DYN-13) and the per-skill
"perk changes" questions (Q-WPN-9, Q-ARM-4, Q-BLK-3, Q-SMI-4, Q-MAG-6, Q-ALT-2, Q-CON-5, Q-ENC-3,
Q-SPE-5, Q-ALC-5, Q-THF-2).

**Question:** can an existing (Lorerim / Requiem) perk get an extra requirement on one of our
sub-skills? Example: perk P requires `smithing-dwarven ≥ 70`.

**Short answer:** yes. The sub-skill value has to be exposed to the condition system first,
because sub-skills are not actor values.

Status: based on general knowledge of the CK perk system and CommonLibSSE-NG. Not verified in the
Lorerim setup. Items marked *(verify)* need confirmation on the Windows machine.

## 1. How perk requirements work

- Every `BGSPerk` has its own condition list (CK: "Perk Conditions"; CommonLibSSE-NG:
  `BGSPerk::perkConditions`, a `TESCondition`) *(verify field name)*.
- The skills menu (`StatsMenu`) evaluates it with the player as subject to decide whether the
  perk can be taken. The vanilla "Requires Smithing 50" is just the condition
  `GetBaseActorValue Smithing >= 50`, plus `HasPerk <previous perk>` for tree prerequisites.
- These conditions gate **acquisition only**. A perk that has been taken stays, even if the
  conditions later become false.
- This is a separate list from the conditions on the perk's **entry points / effects**. Those are
  evaluated each time the effect is used.

## 2. Exposing sub-skills to conditions

| Option | How | Verdict |
|---|---|---|
| **A. GlobalVariable per sub-skill** | ESL holds one `GLOB` per sub-skill (e.g. `SPO_Smithing_Dwarven`); DLL writes the value; condition `GetGlobalValue SPO_Smithing_Dwarven >= 70` | **recommended** |
| B. Spare actor value | DLL mirrors into an unused AV; condition `GetActorValue` | only a handful of AVs; conflicts with other mods |
| C. Custom condition function framework | if such a framework exists in Lorerim, the condition calls our DLL directly | research; adds a dependency |
| D. Hook perk purchase in `StatsMenu` | reject the purchase in code | fragile; the menu still shows the perk as available |

### 2.1 Keeping globals up to date

- Write the global whenever the sub-skill value changes.
- Also write it **when the skills menu opens** (menu-open event), and on game load. Decay is applied
  lazily, so without this the global may hold a stale value.
- Decided later (Q-DYN-10, Q-DYN-13): `current` is checked, and there is one global **per perk
  requirement** (the DLL evaluates the whole formula), not per sub-skill. See
  [`impl/perk-requirements.md`](../impl/perk-requirements.md).

## 3. Attaching the condition

| Option | How | Pros | Cons |
|---|---|---|---|
| ESP override | our ESP overrides the Lorerim `PERK` record and adds the condition | visible in CK/xEdit | overrides Lorerim records; load-order-dependent; breaks when Lorerim updates the perk; conflicts with patches |
| **Runtime injection** | on `SKSE::MessagingInterface::kDataLoaded` the DLL appends a `TESConditionItem` to `perk->perkConditions` (AND with existing), using a config table | no record overrides, survives Lorerim updates, data-driven | not visible in CK; needs correct construction of condition items *(verify `TESConditionItem` / `CONDITION_ITEM_DATA` layout and the `GetGlobalValue` function ID)* |

Recommendation: **runtime injection** from a config table, for example:

```ini
; perk (plugin|formID or EditorID)  sub-skill          value     op   threshold
[PerkRequirements]
Skyrim.esm|0x000CB40E = smithing-dwarven  floor  >=  70     ; Dwarven Smithing (example; verify FormID)
```

- To make an already-taken perk **stop working** below the requirement (Q-DYN-11), inject the
  same condition into every entry point / ability effect of that perk as well. For discrete
  perks, hysteresis (design §5.6) is applied on the DLL side: the global is written so that it
  stays "above" until `value < threshold − hysteresis`.

## 4. Showing the requirement to the player

- The skills menu probably does not render custom conditions as "Requires …" text *(verify)*. The
  player then sees a locked perk with no explanation.
- Options:
  - append "Requires dwarven smithing 70" to the perk description: at runtime from the DLL (edit
    the description text *(verify feasibility)*) or via an ESP override (back to the override
    cons);
  - show requirements in our PrismaUI panel (design §12.1);
  - both.

## 5. Two kinds of perks in this design

| Kind | What it is | How it is gated |
|---|---|---|
| **Purchasable** | an existing or new perk in a main-skill tree, bought with a perk point | perk conditions (main-skill level + optional sub-skill requirements from this note) |
| **Automatic (sub-skill) perk** | a perk owned by a sub-skill, not in any tree, no perk point | added / removed by the DLL at sub-skill thresholds with hysteresis (design §5.6) |

The per-skill questions in the design ask, for each skill, which perks change and which kind
each of them is.
