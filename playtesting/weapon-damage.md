# Weapon damage from the sub-skill: does it keep the game fun?

- **Status:** waiting (weapon damage effect, *hit-damage-multiplier*, prototype 2).
- **Why:** the weapon sub-skill changes weapon damage (−20 % at 0, 0 at 50, +25 % at 100,
  +35.5 % at 200; [weapons](../skills/weapons/design.md) *Continuous effects*), on top of
  Requiem's own scaling with the main skill and perks. Damage is the most balance-sensitive
  number in LoreRim. The designer kept it for now (2026-11-15) on the condition that it does not
  make fights worse: a new weapon must not feel useless, a trained one must not trivialize
  fights, and switching weapons must stay a real choice.
- **Needs:** prototype 2 with the damage effect applied; MCM debug *Set current*; a mid-level
  save (main One-Handed ≈ 50) and a low-level one.

## Steps

1. **New weapon, trained character.** Main One-Handed ≈ 50, `1h-sword` ≈ 100, `1h-mace` at its
   start level. Fight the same group (e.g. a bandit camp at the same difficulty) with the sword,
   then with a mace of similar base damage. Then raise `1h-mace` by normal play for ~1 hour.
2. **Low start.** New character (One-Handed 15): fight the first dungeon with one weapon; note if
   −20 % feels like a punishment or like learning.
3. **Top end.** `1h-sword` 200 (*Set current*): the same group and a dragon / a strong boss.
4. **Combinations.** With a shield (`with-shield` 0) and dual wielding (`dual-wield` 0) at
   `1h-sword` 75: the penalty is largest there (−11 pp with a shield).
5. Repeat 1 with the damage effect off (config / MCM) for a direct comparison.

## Record

- Hits to kill the same enemy type with each setup (or time to kill), from the log / panel.
- Subjective notes per run: frustrating / fine / too easy; did you want to switch back to the
  trained weapon, and why.
- Whether the panel made it clear why damage was lower.

## Expected / pass

- A new weapon at its start level takes noticeably more hits, but the fight is still winnable and
  the gap closes within a few hours of use.
- At 200 the bonus is felt, but enemies of the same tier still need several hits; no one-shotting
  outside sneak attacks.
- The shield / dual-wield penalty reads as "learning a style", not as a trap.
- **Change** if: a new weapon feels useless (lower `pen` or raise the start level), the top end
  trivializes fights (lower `M`), or the comparison with the effect off is clearly more fun
  (drop the damage effect and keep only the non-damage effects of R-37).

## Results

(none yet)
