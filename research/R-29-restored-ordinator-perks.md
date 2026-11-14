# R-29: Restored Ordinator perks: records, scripts, tree nodes

- **Status:** partly done (static, LoreRim dump `~/tmp/research/tables/` of 2026-11-11: spells and
  magic effects); the checks below are open.
- **For:** the perks restored to LoreRim trees ([`impl/perk-tree-nodes.md`](../impl/perk-tree-nodes.md)):
  - Smithing: ORD-SMI-02, -05, -08, -11, -12
    ([`skills/smithing/perks-ordinator.md`](../skills/smithing/perks-ordinator.md));
  - Restoration: ORD-RES-04 ([`skills/restoration/perks-ordinator.md`](../skills/restoration/perks-ordinator.md));
  - Alteration: ORD-ALT-06, -13, -17, -20, -21
    ([`skills/alteration/perks-ordinator.md`](../skills/alteration/perks-ordinator.md)).
- **Why:** the worksheets come from the **original** Ordinator esp; LoreRim loads a rebuilt one
  ([`skills/perks-guide.md`](../skills/perks-guide.md), *Bringing back Ordinator's own perks*).
  A type A perk works only if its records and scripts are still there and unchanged.

## Already confirmed (dump)

- In LoreRim's `Ordinator - Perks of Skyrim.esp` (winner, no other plugin overrides them):
  - Smithing: `ORD_Smh_DwarvenAutocannon_Spell` / `_Spell_Proc` / `_Spell_Proc_Electrobolt` and
    their effects (incl. `…_Effect_FiringLine`, `…_Fail_Blocked`, `…_Fail_NoCannon`);
  - Restoration: `ORD_Res_SpiritTutors_Spell_Ab`, `…_Spell_AbBlessing`, `…_Spell_ActorAb`;
  - Alteration: `ORD_Alt_Spellblade_Spell_Ab` (`01B58C`), `…_Spell_Proc` (`01B589`),
    `ORD_Alt_EnergyRoil_Spell_Proc` (`01BB32`: DamageResist 150, 10 ft, 4 s, hostile),
    `ORD_Alt_RendResistances_Spell_Ab` (`187D2A`), `…_Spell_Proc` (`187D28`: ResistMagic 15),
    `ORD_Alt_ArcaneThesis_Spell_Ab` / `_AbNew` / `_Spell_Power_Learn`.
- The ability effects are **Script** archetype: the logic (when to proc, which perk adds what) is in
  Papyrus, not in records.

## Checks

| # | Question | How | Decides |
|---|---|---|---|
| G1 | Do the PERK records (and rank 2 *Next Perk* records) exist in LoreRim's Ordinator esp, with the conditions of the worksheet? | xEdit, or extend the dump with all PERK records (not only tree nodes) | whether the worksheet's *Requires* is the base for our edits |
| G2 | Are the scripts there (LoreRim's Ordinator plus *Optimised Scripts for Ordinator*), and which perks do they check? | find the scripts attached to the ability effects above; read the source (or decompile the `.pex`) | that the restored perk actually works; the trigger rules below |
| G3 | Can the DLL add tree nodes at runtime? | prototype: add one node to `ActorValueInfo::perkTree` at `kDataLoaded`, open the perks menu, buy, save, load | variant of [`impl/perk-tree-nodes.md`](../impl/perk-tree-nodes.md) |
| G4 | Free grid positions next to each new parent (SMI-04, RES-01, ALT-01, ALT-09, ALT-24, and the chains below them) | AVIF perk-tree dump (`perk-trees.tsv` has no grid; extend it) | node positions |
| S1 | Dwarven Autocannon recipe (COBJ): which perk it requires (ORD-SMI-02 only, not ORD-SMI-01?), which workbench, the cannon item | xEdit / extend the dump with COBJ | whether the recipe needs a condition edit |
| S2 | Electrobolt, Remote Control, Firing Line, Spin Up have no records: does the autocannon script check them with `HasPerk`? | script source (G2) | they work as is |
| S3 | ORD-SMI-02 rank 2, ORD-SMI-05 rank 2: rank records exist and are linked | G1 | ranks |
| S4 | Can *The Art of Dwarven Blacksmithing* (SMI-04) be read below Smithing 30 (*Requiem Smithing Books Give Perks*)? | the book's script / conditions | whether ORD-SMI-02's `Smithing ≥ 20` ever matters |
| R1 | Spirit Tutors: are the two tutor NPCs, their placement and the dialogue / quest that gives the blessing in LoreRim's Ordinator esp? | xEdit (NPC_, REFR, QUST, DIAL with `ORD_Res_SpiritTutors`) | whether the perk does anything |
| A1 | Spellblade: where is the +20 % attack damage (the proc effect's script, or an entry point that checks `ORD_Alt_Spellblade_Keyword`)? What exactly is a "one-handed cast" in the script? Does it cast the Energy Roil proc *instead of* the Spellblade proc when the player has ORD-ALT-13? | script source (G2), perks with entry points on `ORD_Alt_Spellblade_Keyword` | gating of the effect; Energy Roil change |
| A2 | Energy Roil change: a condition on effect 1 of `01BB32` run on `PlayerRef` (`GetEquippedItemType` Left / Right == 1–4) and magnitude 100: do conditions of a self-centred area effect run per target, with the reference on the player? | in game: cast with a sword in the other hand / with fists, look at a target's armor (console `getav DamageResist`) | the change works without a script edit |
| A3 | Changing the perk description (Energy Roil: 100 points, weapon rule): runtime (`TESDescription` is read from the file on demand) or an ESL override of the PERK record (needs G1: no other plugin overrides it) | CommonLib, xEdit | how texts of changed perks are updated |
| A4 | Rend Resistances: what triggers the proc ("whenever you attack with a weapon": swing or hit)? | script source | description only |
| A5 | Arcane Thesis: the power is given by an ability script; the mastered spell is kept in a FormList; does it survive save / load? | script source; in game | that it works |
| A6 | Ritualist: what lets ritual spells be cast while moving (an entry point or flag of the perk not shown in the worksheet, or a script)? Which FormList is "is in list", and does it hold the LoreRim ritual spells (Magic Redone, Apocalypse, Triumvirate)? | xEdit on the PERK record and the FormList | whether the perk works for LoreRim's ritual spells or the list must be extended |
