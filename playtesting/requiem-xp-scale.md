# Requiem XP scale: use points per action, vanilla vs LoreRim

- **Status:** ready (prototype 1: the `[xp]` log shows raw use points).
- **Why:** Requiem rescales armor and damage, so damage-based use points (Block, Heavy / Light
  Armor, maybe weapons) are many times the vanilla ones: in the first playtest 2–3 blocks took
  `block` from 20 to 25. The `Requiem` preset factors (design-v2 §4.2.1, **Q-XP-10**) must bring
  them back to the vanilla scale. Placeholders until measured: 0.1 for Block and armor, 1 for
  the rest.
- **Needs:** a vanilla profile (Skyrim + SKSE + our DLL, no Requiem) and the LoreRim profile;
  preset `Vanilla` (factor 1) in both, `debug.xp_mult = 1`, verbose XP log on.
- **Steps (same in both profiles, same character level if possible):**
  1. Block 10 hits of a bandit (one-handed weapon) with a shield; 10 with a weapon only.
  2. Take 10 hits in heavy armor, 10 in light armor.
  3. Hit a bandit 10 times with a sword, a greatsword, a bow (also 5 power attacks).
  4. Cast a Destruction and a Restoration spell 10 times each (control: should not differ).
- **Record:** per action, the raw use points of each award (`[xp]` lines), the enemy's level
  and the damage shown; Requiem version of the list.
- **Expected / pass:** magic equal in both profiles; for each damage-based skill, factor =
  mean(vanilla points) / mean(LoreRim points) per action. Write the factors into
  `config/xp-presets.toml` `[requiem]` and close Q-XP-10. If weapons follow base damage and do
  not differ, they keep 1. If one factor per skill does not fit both blocked damage and bash,
  record that (per-source factor).
- **Results:**
