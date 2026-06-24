<#
.SYNOPSIS
  Generate World_1002.unity via Unity batchmode.

.EXAMPLE
  .\scripts\setup_world_scene.ps1
#>
param(
    [string]$UnityPath = ''
)

$ErrorActionPreference = 'Stop'
$root = git rev-parse --show-toplevel
Set-Location $root
. (Join-Path $PSScriptRoot '_tuanjie_common.ps1')

$unity = Assert-Tuanjie1611Installed -PreferredPath $UnityPath
New-Item -ItemType Directory -Force -Path (Join-Path $root 'logs') | Out-Null

& $unity -batchmode -nographics -quit `
    -projectPath $root `
    -executeMethod Rpg.Client.EditorTools.WorldSceneSetup.SetupWorldScene1002Batch `
    -logFile (Join-Path $root 'logs/unity_setup_world.log')

if ($LASTEXITCODE -ne 0) {
    throw 'World scene setup failed, see logs/unity_setup_world.log'
}

$worldScene = Join-Path $root 'assets/_Project/Scenes/World_1002.unity'
if (-not (Test-Path $worldScene)) {
    throw 'World_1002.unity not found. Close Unity Editor and re-run, or use menu RPG -> Setup World Scene 1002.'
}

Write-Host 'World scene OK: assets/_Project/Scenes/World_1002.unity'
