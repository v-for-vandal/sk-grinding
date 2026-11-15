# R-28: Ordinator Mage Ward and engine wards

- **For:** [`skills/restoration/perks-ordinator.md`](../skills/restoration/perks-ordinator.md)
  ORD-RES-24 Mage Ward, ORD-RES-17 Bastion Ward; `restoration/wards` XP
  ([`skills/restoration/design.md`](../skills/restoration/design.md)).
- **Status:** web research done 2026-11-12 (search snippets only: Nexus, UESP, Steam returned
  errors, no page read in full); the record checks below are open.

## Findings

**Engine wards (fairly well sourced, UESP AV indices and ward mods):**

- A ward is the actor value **WardPower** (AV 63, type *Buffer*). An incoming spell subtracts its
  magnitude from WardPower; at 0 the ward breaks (stagger, by most reports no damage passes).
- Vanilla ward spells: concentration, self, archetype *Accumulate Magnitude* on WardPower (builds
  up over about a second, `fMagicWardPowerMaxBase`). The ward is held in **one hand**; the other
  hand can cast another spell at the same time (general dual-cast-different-spells rule).
- Any effect that raises WardPower is a ward without a hand: Spellbreaker gives a flat
  WardPower bonus while blocking; *Snappy Wards* turns wards into *Value Modifier* and they still
  work.
- The `MagicWard` keyword drives conditions (perks, poison spells) and the hand-out ward
  animation; whether absorption needs it is not known (probably not, see Spellbreaker).
- Restoration XP: granted when a ward absorbs spell damage (not for holding it). Whether XP is
  tied to the ward spell or to WardPower is not known.

**Mage Ward (Ordinator 80, author's text and changelogs):**

- "Casting a spell also raises a ward that increases armor rating by 100 and negates up to 100
  points of spell damage and effects." Our dump: ability *Mage Ward* with *Ward - Mage*
  (ValueModifier WardPower 100) and *Shield - Mage* (PeakValueModifier DamageResist 100).
- So it is **not** a concentration ward spell: it is a WardPower effect that appears when the
  player casts a spell, with no ward in hand; it can be up while another spell is cast.
- Changelogs: it does not trigger while you already cast a ward; at first it "is not considered a
  ward for other effects", later Antimagic Field and Bastion Ward were extended to it (Bastion
  Ward's conditions check `ORD_Res_MageWard_Keyword`, i.e. its effect has **no** `MagicWard`);
  a fix for "invalid or negative WardPower values if interrupted early" (so it adds and removes
  WardPower itself, likely by script); old reports of its build-up rate going wrong.
- Not found: duration (while casting or N seconds after), which casts count (powers, scrolls,
  concentration), the exact trigger.

## Open checks (xEdit on the Ordinator esp LoreRim loads)

1. ORD-RES-24 perk: entry points, the ability, conditions, attached scripts (trigger, duration).
2. *Ward - Mage* MGEF: archetype, AV, keywords (`MagicWard`?), conditions.
3. `DA13SpellbreakerwardEffect`: has `MagicWard`?
4. In game: does a Mage Ward / Spellbreaker absorption call `UseSkill(Restoration)` (R-17 log)?

## Consequences for the design

- Our ward detection keyed on `MagicWard` (spell classifier, `restoration/wards`) will not see
  Mage Ward. Decide whether its absorptions give `restoration/wards` XP (check 4).
