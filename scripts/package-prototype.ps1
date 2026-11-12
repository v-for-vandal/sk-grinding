$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot
$target = Join-Path $root 'dist/SkillPractice'
$dll = Join-Path $root 'build/prototype/Release/SkillPractice.dll'
if (!(Test-Path $dll)) { throw 'Build SkillPractice.dll first' }
New-Item -ItemType Directory -Force "$target/SKSE/Plugins/SkillPractice/skills" | Out-Null
Copy-Item $dll "$target/SKSE/Plugins/SkillPractice.dll"
Copy-Item "$root/build/prototype/Release/SkillPractice.pdb" "$target/SKSE/Plugins/" -ErrorAction SilentlyContinue
Copy-Item "$root/config/SkillPractice.toml","$root/config/smithing-materials.toml","$root/config/magic-directions.toml" "$target/SKSE/Plugins/SkillPractice/"
Copy-Item "$root/config/skills/*.toml" "$target/SKSE/Plugins/SkillPractice/skills/"
Copy-Item "$root/config/kid/SPO_WeaponClass_KID.ini" $target
Compress-Archive -Path "$target/*" -DestinationPath "$root/dist/SkillPractice.zip" -Force
Write-Host "MO2 package: $root/dist/SkillPractice.zip"
