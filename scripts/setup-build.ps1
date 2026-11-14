# Fetch pinned public build dependencies. Requires Git and network access.
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot
Push-Location $root
try {
    & git submodule update --init --recursive
    if ($LASTEXITCODE) { throw 'Submodule initialization failed' }
    $dependencies = @(
        @{ Path = '.tools/vcpkg'; Url = 'https://github.com/microsoft/vcpkg.git'; Commit = 'b58529e33332fbb6dfe20317a82009b78ceeab36' },
        @{ Path = '.tools/downloads/MCM-Helper'; Url = 'https://github.com/Exit-9B/MCM-Helper.git'; Commit = '9df6b69348dea57b3247b9f812711384f1113bab' }
    )
    foreach ($dependency in $dependencies) {
        if (!(Test-Path $dependency.Path)) {
            New-Item -ItemType Directory -Force (Split-Path $dependency.Path) | Out-Null
            & git clone $dependency.Url $dependency.Path
            if ($LASTEXITCODE) { throw "Clone failed: $($dependency.Url)" }
        }
        $head = & git -C $dependency.Path rev-parse HEAD
        if ($LASTEXITCODE) { throw "Not a Git checkout: $($dependency.Path)" }
        if ($head -ne $dependency.Commit) {
            $changes = & git -C $dependency.Path status --porcelain
            if ($LASTEXITCODE -or $changes) { throw "Dependency has local changes: $($dependency.Path)" }
            & git -C $dependency.Path checkout --detach $dependency.Commit
            if ($LASTEXITCODE) { throw "Checkout failed: $($dependency.Commit)" }
        }
    }
    & ./.tools/vcpkg/bootstrap-vcpkg.bat -disableMetrics
    if ($LASTEXITCODE) { throw 'vcpkg bootstrap failed' }
    Write-Host 'Build dependencies ready. See plugin/README.md for compiler and SDK requirements.'
}
finally { Pop-Location }
