# Skill Practice prototype build

Target: Skyrim AE 1.6.1170, SKSE 2.2.6, Visual Studio Build Tools 2022
(Desktop development with C++, Windows SDK, CMake), C++23. Install .NET 8 SDK
or newer. No game files are written by the build.

From the repository root in PowerShell:

```powershell
git submodule update --init --recursive
git clone https://github.com/microsoft/vcpkg.git .tools/vcpkg
git -C .tools/vcpkg checkout b58529e33332fbb6dfe20317a82009b78ceeab36
.tools/vcpkg/bootstrap-vcpkg.bat -disableMetrics
# The local SDK can be installed under .tools/dotnet; build-prototype uses that path.
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/build-prototype.ps1
```

The script configures CMake, builds Release, runs CTest, generates the ESP and
packages `dist/SkillPractice.zip`. `-CoreOnly` skips the DLL and package.
Dependencies use the pinned vcpkg baseline in `plugin/vcpkg.json`.
CommonLibSSE-NG is a pinned submodule; its original license remains there.
PrismaUI_API.h is copied from the framework's 1.5.1 tag. No project license
has been selected (Q-TECH-2).

Do not claim runtime checks have passed based on a clean build. Follow
`plan/prototype-1-testing.md` and send the resulting logs.
