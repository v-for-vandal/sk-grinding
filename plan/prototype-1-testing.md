# Prototype 1 testing

Build status: M0 package built and unit tests passed. Later milestones are in progress.
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

Further milestone scenarios will be added as the corresponding builds are completed.
