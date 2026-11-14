# Deterministic plugin records

The .NET 10 console tool uses Mutagen.Bethesda.Skyrim 0.54.4. Run from the
repository root, with a .NET 10 SDK:

```powershell
dotnet run --project tools/esp-gen --configuration Release -- .
```

Output: `dist/SkillPractice/SkillPractice.esp`, ESL flagged, Skyrim.esm master.
The 22 keyword EditorIDs come from `config/kid/SPO_WeaponClass_KID.ini`, sorted
ordinally. IDs are `0x800` through `0x815`; `keywords.json` lists the mapping.
The C++ table in `plugin/src/WeaponClasses.h` uses the same order.

`0x816` is the start-enabled `SkillPractice_MCM` quest, with an alias forced
to Skyrim.esm|0x14 (PlayerRef). VMAD attaches SkillPractice_MCMScript and
SKI_PlayerLoadGameAlias. The generator reads the ESP back and checks keyword,
quest and alias counts. MCM Helper/SkyUI are script dependencies, not ESP masters.
The DLL starts the quest after load/new game when necessary, so no SEQ file is
required. MCM registration is still an in-game check.

The generator does not read or alter the user's installed ESPs.
