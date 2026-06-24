<#
.SYNOPSIS
  构建地图 CDN 包：World 场景 + Addressables + MapExport 双端包。

.PARAMETER UnityPath
  Unity/Tuanjie 可执行文件路径。

.PARAMETER SkipAddressables
  跳过 Addressables 构建（仅 MapExport）。

.EXAMPLE
  .\scripts\build_map_packages.ps1
#>
param(
    [string]$UnityPath = '',
    [switch]$SkipAddressables
)

$ErrorActionPreference = 'Stop'
$root = git rev-parse --show-toplevel
Set-Location $root
. (Join-Path $PSScriptRoot '_tuanjie_common.ps1')

$cdnRoot = Join-Path $root 'build/map_cdn/rpg/map'
New-Item -ItemType Directory -Force -Path (Join-Path $root 'logs') | Out-Null

try {
    $unity = Assert-Tuanjie1611Installed -PreferredPath $UnityPath
}
catch {
    Write-Host $_.Exception.Message
    Write-Host '可在编辑器中依次执行：RPG -> Setup World Scene 1002 / Setup Map Addressables / Export Map Package'
    exit 1
}

Write-Host "Using Unity: $unity"

$setupLog = Join-Path $root 'logs/unity_setup_world.log'
Write-Host 'Step 1: Setup World_1002 scene...'
& $unity -batchmode -nographics -quit `
    -projectPath $root `
    -executeMethod Rpg.Client.EditorTools.WorldSceneSetup.SetupWorldScene1002Batch `
    -logFile $setupLog
if ($LASTEXITCODE -ne 0) { throw "World scene setup failed, see $setupLog" }

$addrSetupLog = Join-Path $root 'logs/unity_setup_addressables.log'
Write-Host 'Step 2: Setup Map Addressables...'
& $unity -batchmode -nographics -quit `
    -projectPath $root `
    -executeMethod Rpg.Client.EditorTools.MapAddressablesSetup.SetupMapAddressablesBatch `
    -logFile $addrSetupLog
if ($LASTEXITCODE -ne 0) { throw "Addressables setup failed, see $addrSetupLog" }

if (-not $SkipAddressables) {
    $addrBuildLog = Join-Path $root 'logs/unity_build_addressables.log'
    Write-Host 'Step 3: Build Addressables content...'
    & $unity -batchmode -nographics -quit `
        -projectPath $root `
        -executeMethod Rpg.Client.EditorTools.MapAddressablesBuild.BuildMapAddressablesBatch `
        -logFile $addrBuildLog
    if ($LASTEXITCODE -ne 0) { throw "Addressables build failed, see $addrBuildLog" }

    $serverData = Join-Path $root 'ServerData'
    if (Test-Path $serverData) {
        $addrDest = Join-Path $cdnRoot 'addressables'
        New-Item -ItemType Directory -Force -Path $addrDest | Out-Null
        Get-ChildItem -Path $serverData -Recurse | ForEach-Object {
            $rel = $_.FullName.Substring($serverData.Length).TrimStart('\', '/')
            $target = Join-Path $addrDest $rel
            if ($_.PSIsContainer) {
                New-Item -ItemType Directory -Force -Path $target | Out-Null
            }
            else {
                $targetDir = Split-Path $target -Parent
                New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
                Copy-Item $_.FullName $target -Force
            }
        }
        Write-Host "Addressables copied to $addrDest"
    }
}

$exportLog = Join-Path $root 'logs/unity_export_map.log'
Write-Host 'Step 4: MapExport dual package...'
$env:RPG_MAP_CDN_ROOT = $cdnRoot
& $unity -batchmode -nographics -quit `
    -projectPath $root `
    -executeMethod Rpg.Client.EditorTools.MapExportWindow.ExportMapPackageBatch `
    -logFile $exportLog
if ($LASTEXITCODE -ne 0) { throw "MapExport failed, see $exportLog" }

Write-Host ''
Write-Host "CDN package ready: $cdnRoot"
Write-Host 'Serve locally: .\scripts\serve_map_cdn.ps1'
