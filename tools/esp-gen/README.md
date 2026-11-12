# Deterministic plugin records

The .NET 8 console tool uses Mutagen.Bethesda.Skyrim 0.54.4. Run from the
repository root, with a .NET 8 or newer SDK:

```powershell
dotnet run --project tools/esp-gen --configuration Release -- .
```

Output: `dist/SkillPractice/SkillPractice.esp`, ESL flagged, Skyrim.esm master.
The 22 keyword EditorIDs come from `config/kid/SPO_WeaponClass_KID.ini`, sorted
ordinally. IDs are `0x800` through `0x815`; `keywords.json` lists the mapping.
The C++ table in `plugin/src/WeaponClasses.h` uses the same order.

The generator does not read or alter the user's installed ESPs.
