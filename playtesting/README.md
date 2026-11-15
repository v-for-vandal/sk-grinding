# Playtesting scenarios

Things that can only be decided by playing. One file per scenario: why it is played, what to
do, what to record, what result is expected and which design decision waits for it. The
design ([`design-v2.md`](../design-v2.md), [`skills/`](../skills/README.md)) links here where it
waits for a result; the prototype's technical checklist is
[`plan/prototype-1-testing.md`](../plan/prototype-1-testing.md) (does the code work), this
folder is about **balance and design** (are the numbers and rules right).

## Format of a scenario file

- **Status:** `waiting` (needs features not built yet), `ready` (can be played now), `done`
  (result recorded below, decision updated).
- **Why:** the question it answers, with links to the design text / question.
- **Needs:** mod version or features, save, settings (MCM, debug).
- **Steps:** what to do in game.
- **Record:** log lines, panel values, numbers to write down.
- **Expected / pass:** what result confirms the design; what result means a change.
- **Results:** filled after each run (date, version, numbers, conclusion).

## Index

| Scenario | Status | Waits for it |
|---|---|---|
| [`requiem-xp-scale.md`](requiem-xp-scale.md): use points per action, vanilla vs LoreRim | ready | design-v2 §4.2.1, Q-XP-10 |
| [`practice-pace.md`](practice-pace.md): raw XP per active hour of normal play | ready | Q-ML-7 (`P_skill`), Q-DYN-15 (`P_sub`) |
| [`spell-forms.md`](spell-forms.md): spell form of beam-looking spells (Sunbeam) | ready | skills/magic Q-MAG-9, R-32 |
| [`skill-books.md`](skill-books.md): book points with and without the Skilled trait | waiting (book hook) | design-v2 §10.1, R-30 |
| [`dragon-fight.md`](dragon-fight.md): budgets and XP in a LoreRim dragon fight | waiting (budgets) | design-v2 §11.2 (budgets, target worth) |
| [`weapon-damage.md`](weapon-damage.md): weapon damage from the sub-skill keeps fights fun | waiting (damage effect) | weapons *Continuous effects* (damage kept 2026-11-15) |
