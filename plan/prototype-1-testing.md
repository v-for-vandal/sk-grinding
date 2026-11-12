# Prototype 1 testing

Build status: M0 and integrated runtime package built; pure-logic tests passed.
The implementer is finishing the remaining milestone checks and documentation.
Runtime results are pending; no Skyrim tests were run by the implementing agent.

1. Install [Prisma UI 1.5.1](https://www.nexusmods.com/skyrimspecialedition/mods/148718)
   through MO2, Install a new mod from archive, and enable it. Media Keys Fix SKSE
   is already in LoreRim. PrismaUI has no ESP. After launch check
   `Documents\My Games\Skyrim Special Edition\SKSE\PrismaUI.log`. If its DLL fails
   to load on 1.6.1170, stop and send that log.
2. Install `dist/SkillPractice.zip` through MO2 and enable it. Put SkillPractice.esp
   after Skyrim.esm. Use a separate test save. For clean numbers set SSL's points
   per level and level multiplier to zero (R-16). Do not use Legendary resets.
3. M0: launch via SKSE and inspect `Documents\My Games\Skyrim Special Edition\SKSE\SkillPractice.log`.
   Expect `[boot] managed=6`, the sub-skill count, all 22 weapon keywords resolved,
   no `[boot] config error`. Send the log if any are missing.

4. For R-17 set MCM → SkillPractice → Debug → Log level Debug, Verbose XP on.
   Perform each action three times, leaving two seconds between different actions:
   one-handed hit, two-handed hit, unarmed punch, shield block, weapon parry,
   shield bash, weapon bash, torch bash, forge craft, temper, smelt, tan,
   Destruction hit, self heal, heal another actor, ward absorbing a spell.
   Note the order and in-game time. Compare `[use]` with `[ctx]` and `[xp]`.
   Nonzero points before LoreRim's zero UseMult are the R-17 check 1 result.
5. R-17 check 2: read a skill book and, separately, use an SSL level-up on the
   test save. Record `[use] path=AddSkillExperience` and `[main] external`.
   This prototype logs AddSkillExperience in place of the undeclared
   PlayerSkills::AdvanceSkill; absence of a line does not prove no other path exists.
6. R-17 checks 3–4: send the whole log, including `[boot] hook` original-module
   lines. Microsecond timestamps and `frame` in `[ctx]`/`[use]` establish event order.
   Include which weapon/spell was equipped and whether Precision/TDM were enabled.
