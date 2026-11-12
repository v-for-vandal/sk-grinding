# Skill Practice prototype build

Target: Skyrim AE 1.6.1170, SKSE 2.2.6, Visual Studio Build Tools 2022
(Desktop development with C++, Windows SDK, CMake), C++23. Install the .NET 10
SDK. No game files are written by the build.

From the repository root in PowerShell:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/setup-build.ps1
# Uses .tools/dotnet/dotnet.exe when present, otherwise the installed dotnet SDK.
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-prototype.ps1
```

The script configures CMake, builds Release, runs CTest, generates the ESP and
packages `dist/SkillPractice.zip`. `-CoreOnly` skips the DLL and package; `-Clean`
also rebuilds CommonLib from scratch. On another machine pass paths explicitly:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-prototype.ps1 -Game 'C:\Games\Skyrim Special Edition' -SkseSources 'C:\SDK\skse\scripts\Source'
```

`-Game` must contain `Papyrus Compiler/PapyrusCompiler.exe` (Creation Kit) and
`Data/Source/Scripts`, including `TESV_Papyrus_Flags.flg`. `-SkseSources` points
to the SKSE source scripts; when omitted it is derived from `-LoreRim` using
the local MO2 mod directory name. `-McmSdk` can override the downloaded public
SDK directory. Skyrim, Creation Kit and SKSE sources must be installed separately;
they are not stored in this repository. Dependency setup and the first build
need network access. Build support is Windows x64.

Track `plugin/`, `config/`, `tools/esp-gen/`, `papyrus/`, `mcm/`, `ui/` and
`scripts/`, including `.gitmodules` and the CommonLib submodule reference.
`.tools/`, `build/`, `dist/`, and .NET `bin/` / `obj/` are generated and ignored.
`scripts/package-prototype.ps1` creates the ZIP after the DLL, ESP and Papyrus
script have been built; normally `build-prototype.ps1` runs it automatically.
Dependencies use the pinned vcpkg baseline in `plugin/vcpkg.json`.
CommonLibSSE-NG is a pinned submodule; its original license remains there.
PrismaUI_API.h is copied from the framework's 1.5.1 tag. No project license
has been selected (Q-TECH-2).

MCM public SDK tag v1.6.2: `9df6b69348dea57b3247b9f812711384f1113bab`.
PrismaUI API tag 1.5.1: `aaef0713a6b4c775a654c427871f0fe53b75fcb6`.
The prototype rejects runtimes other than 1.6.1170 before installing hooks.

For independent normalization fixtures (Python 3.11+):

```powershell
python scripts/prototype-reference.py
```

For browser checks, serve `ui/` over localhost and open `/test.html`; its result
must say PASS. `PracticeTests config ui/mock.json` refreshes the sample snapshot.
Mock/test files are excluded from the MO2 package.

Do not claim runtime checks have passed based on a clean build. Follow
`plan/prototype-1-testing.md` and send the resulting logs.
