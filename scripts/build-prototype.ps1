param([switch]$CoreOnly)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot
Set-Location $root
$vs = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vs) { throw 'Install VS 2022 Build Tools, Desktop development with C++.' }
$cmake = Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$vcpkg = Join-Path $root '.tools/vcpkg/scripts/buildsystems/vcpkg.cmake'
$plugin = if ($CoreOnly) { 'OFF' } else { 'ON' }
& $cmake -S plugin -B build/prototype -G 'Visual Studio 17 2022' -A x64 "-DCMAKE_TOOLCHAIN_FILE=$vcpkg" -DVCPKG_TARGET_TRIPLET=x64-windows-static-md "-DPRACTICE_PLUGIN=$plugin"
if ($LASTEXITCODE) { throw 'CMake configure failed' }
& $cmake --build build/prototype --config Release --parallel 4
if ($LASTEXITCODE) { throw 'C++ build failed' }
$ctest = Join-Path (Split-Path $cmake) 'ctest.exe'
& $ctest --test-dir build/prototype -C Release --output-on-failure
if ($LASTEXITCODE) { throw 'Unit tests failed' }
if ($CoreOnly) { return }
$env:DOTNET_CLI_HOME = Join-Path $root '.tools/cli'
$env:NUGET_PACKAGES = Join-Path $root '.tools/nuget'
$env:DOTNET_CLI_TELEMETRY_OPTOUT = '1'
& .tools/dotnet/dotnet.exe run --project tools/esp-gen --configuration Release -- $root
if ($LASTEXITCODE) { throw 'ESP generator failed' }
& "$PSScriptRoot/compile-papyrus.ps1"
& "$PSScriptRoot/package-prototype.ps1"
