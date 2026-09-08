$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$dependency = Join-Path $repoRoot 'extern/commonlibf4'
$revision = 'ca31eeb6c7353555973bc351c6733d6492f2c66e'
if (Test-Path -LiteralPath $dependency) {
    throw 'extern/commonlibf4 already exists. Keep the existing dependency or move it aside before bootstrapping.'
}
git clone --no-checkout https://github.com/Dear-Modding-FO4/commonlibf4.git $dependency
if ($LASTEXITCODE) { throw 'CommonLibF4 clone failed' }
git -C $dependency checkout --detach $revision
if ($LASTEXITCODE) { throw 'CommonLibF4 checkout failed' }
git -C $dependency submodule update --init --recursive
if ($LASTEXITCODE) { throw 'CommonLibF4 submodule setup failed' }
