$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    $versionMatch = [regex]::Match((Get-Content xmake.lua -Raw), 'set_version\("(\d+\.\d+\.\d+)"\)')
    if (!$versionMatch.Success) { throw 'Cannot read release version from xmake.lua' }
    $releaseVersion = $versionMatch.Groups[1].Value
    if (!(Test-Path 'extern/commonlibf4/xmake.lua')) { & ./tools/bootstrap.ps1 }
    xmake f -m releasedbg -y
    if ($LASTEXITCODE) { throw 'Configuration failed' }
    xmake build ENBInputDisablerFO4
    if ($LASTEXITCODE) { throw 'Build failed' }
    xmake build input-state-tests
    if ($LASTEXITCODE) { throw 'Test build failed' }
    xmake run input-state-tests
    if ($LASTEXITCODE) { throw 'Input state tests failed' }
    New-Item -ItemType Directory -Force 'release' | Out-Null
    Copy-Item 'build/windows/x64/releasedbg/ENBInputDisablerFO4.dll' 'release/'
    Copy-Item 'build/windows/x64/releasedbg/ENBInputDisablerFO4.pdb' 'release/'
    Copy-Item 'res/ENBInputDisablerFO4.ini' 'release/'
    New-Item -ItemType Directory -Force 'dist/F4SE/Plugins' | Out-Null
    Copy-Item 'build/windows/x64/releasedbg/ENBInputDisablerFO4.dll' 'dist/F4SE/Plugins/'
    Copy-Item 'build/windows/x64/releasedbg/ENBInputDisablerFO4.pdb' 'dist/F4SE/Plugins/'
    Copy-Item 'res/ENBInputDisablerFO4.ini' 'dist/F4SE/Plugins/'
    Copy-Item README.md,LICENSE,EXCEPTIONS.md -Destination dist
    # Stage only the intended payload: unrelated/stale files in dist must not ship.
    $packageStage = Join-Path 'build/package-staging' ([guid]::NewGuid().ToString())
    New-Item -ItemType Directory -Force "$packageStage/F4SE/Plugins" | Out-Null
    foreach ($extension in 'dll','pdb','ini') {
        Copy-Item "dist/F4SE/Plugins/ENBInputDisablerFO4.$extension" "$packageStage/F4SE/Plugins/"
    }
    Copy-Item README.md,LICENSE,EXCEPTIONS.md -Destination $packageStage
    Compress-Archive -Path "$packageStage/F4SE","$packageStage/README.md","$packageStage/LICENSE","$packageStage/EXCEPTIONS.md" -DestinationPath "build/ENBInputDisablerFO4-$releaseVersion.zip" -Force
} finally { Pop-Location }
