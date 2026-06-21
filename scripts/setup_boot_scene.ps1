<#
.SYNOPSIS
  Generate Boot.unity via Unity batchmode（团结引擎 1.6.11）。

.PARAMETER UnityPath
  Full path to Unity.exe. If omitted, requires Hub 2022.3.61t12.

.EXAMPLE
  .\scripts\setup_boot_scene.ps1
#>
param(
    [string]$UnityPath = ''
)

$ErrorActionPreference = 'Stop'
$root = git rev-parse --show-toplevel
Set-Location $root

. (Join-Path $PSScriptRoot '_tuanjie_common.ps1')

try {
    $unity = Assert-Tuanjie1611Installed -PreferredPath $UnityPath
}
catch {
    Write-Host $_.Exception.Message
    Write-Host ''
    Write-Host 'Option: Unity Editor menu RPG -> Setup Boot Scene'
    exit 1
}

Write-Host "Using Unity: $unity"

New-Item -ItemType Directory -Force -Path (Join-Path $root 'logs') | Out-Null

Write-Host 'Generating Boot scene via Unity batchmode...'
& $unity -batchmode -nographics -quit `
    -projectPath $root `
    -executeMethod Rpg.Client.EditorTools.BootSceneSetup.SetupBootSceneBatch `
    -logFile (Join-Path $root 'logs/unity_setup_boot.log')

if ($LASTEXITCODE -ne 0) {
    throw 'Boot scene setup failed, see logs/unity_setup_boot.log'
}

$logPath = Join-Path $root 'logs/unity_setup_boot.log'
if (Test-Path $logPath) {
    $logText = Get-Content $logPath -Raw
    if ($logText -match 'EBUSY:\s*resource busy or locked') {
        throw @"
Unity Package Manager could not write PackageCache (files locked).
1. In Tuanjie error dialog click Exit (not Continue)
2. Close all Tuanjie/Unity windows for this project
3. Run: .\scripts\clean_unity_library.ps1 -Full
4. Reopen project in Tuanjie $RequiredTuanjieVersion, wait for packages
5. Then: .\scripts\setup_boot_scene.ps1
See logs/unity_setup_boot.log
"@
    }
    if ($logText -match 'Fatal Error|Crash!!!|似乎有另一个正在运行的 Unity 实例') {
        throw 'Unity batchmode failed (close Unity Editor for this project first). See logs/unity_setup_boot.log'
    }
    if ($logText -notmatch 'BootSceneSetup') {
        throw 'BootSceneSetup did not run (check script compile errors). See logs/unity_setup_boot.log'
    }
}

$bootScene = Join-Path $root 'assets/_Project/Scenes/Boot.unity'
if (-not (Test-Path $bootScene)) {
    throw 'Boot.unity was not created'
}

$sceneText = Get-Content $bootScene -Raw
if ($sceneText -notmatch 'GameApp|GameUiController|GameRoot') {
    throw 'Boot.unity exists but looks like a placeholder; use RPG -> Setup Boot Scene in Editor'
}

Write-Host 'Boot scene OK: assets/_Project/Scenes/Boot.unity'
