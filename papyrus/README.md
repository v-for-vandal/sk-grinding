# MCM glue

`SkillPractice_MCMScript.psc` is deliberately empty. It inherits MCM Helper's
configuration handling. The generated start-enabled quest attaches it and a
PlayerRef alias with `SKI_PlayerLoadGameAlias`.

Install the Creation Kit's Papyrus compiler, vanilla sources, SKSE sources and
MCM Helper public SDK scripts. The latter live at
https://github.com/Exit-9B/MCM-Helper/tree/master/scripts/public .
The build uses tag v1.6.2; clone it under `.tools/downloads/MCM-Helper`.
The public SDK contains the SkyUI interfaces needed by this script; do not
compile or redistribute those interface stubs as PEX files.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/compile-papyrus.ps1
```

Equivalent compiler invocation (adapt paths):

```text
PapyrusCompiler.exe SkillPractice_MCMScript.psc -i="papyrus;MCM-Helper/scripts/public;SKSE/Scripts/Source;Skyrim/Data/Source/Scripts" -f="Skyrim/Data/Source/Scripts/TESV_Papyrus_Flags.flg" -o="build/papyrus"
```

Only our PEX and PSC go in the MO2 package. Default and user INIs are read by
the DLL on load, new game and Journal close; all changed values are logged.
The wrapper copies CK compiler binaries into `.tools/papyrus-compiler` and adds
a CLR 4 activation config there. The game's compiler directory is unchanged.
