param(
    [string]$Game = 'D:\SteamLibrary\steamapps\common\Skyrim Special Edition',
    [string]$LoreRim = 'D:\Modlists\LoreRim'
)
$ErrorActionPreference = 'Stop'
$root=Split-Path $PSScriptRoot
$compiler=Join-Path $Game 'Papyrus Compiler/PapyrusCompiler.exe'
$vanilla=Join-Path $Game 'Data/Source/Scripts'
$sdk=Join-Path $root '.tools/downloads/MCM-Helper/scripts/public'
$skse=Join-Path $LoreRim 'mods/Skyrim Script Extender (SKSE64)/Scripts/Source'
foreach ($path in @($compiler,$vanilla,$sdk,$skse)) { if (!(Test-Path $path)) { throw "Missing Papyrus requirement: $path" } }
# CK ships a CLR 2 executable. Use the installed CLR 4 without enabling a Windows
# feature or modifying the game's compiler directory.
$localCompiler=Join-Path $root '.tools/papyrus-compiler'
New-Item -ItemType Directory -Force $localCompiler | Out-Null
Get-ChildItem (Split-Path $compiler) -File | Copy-Item -Destination $localCompiler -Force
$runtimeConfig='<?xml version="1.0"?><configuration><startup useLegacyV2RuntimeActivationPolicy="true"><supportedRuntime version="v4.0" sku=".NETFramework,Version=v4.0"/></startup></configuration>'
$runtimeConfig | Set-Content -Encoding UTF8 "$localCompiler/PapyrusCompiler.exe.config"
$runtimeConfig | Set-Content -Encoding UTF8 "$localCompiler/PapyrusAssembler.exe.config"
$compiler=Join-Path $localCompiler 'PapyrusCompiler.exe'
New-Item -ItemType Directory -Force "$root/build/papyrus" | Out-Null
$imports="$root/papyrus;$sdk;$skse;$vanilla"
& $compiler "$root/papyrus/SkillPractice_MCMScript.psc" "-i=$imports" "-f=$vanilla/TESV_Papyrus_Flags.flg" "-o=$root/build/papyrus"
if ($LASTEXITCODE -or !(Test-Path "$root/build/papyrus/SkillPractice_MCMScript.pex")) { throw 'Papyrus compilation failed' }
