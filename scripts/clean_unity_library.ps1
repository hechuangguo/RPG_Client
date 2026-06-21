<#
.SYNOPSIS
  Clean Unity/Tuanjie local cache to fix Package Manager EBUSY (file locked).

.PARAMETER Full
  Remove entire Library/ (full reimport on next open).

.PARAMETER ForceKill
  Kill Unity/Tuanjie Editor processes if still running (does not kill Hub unless -KillHub).

.PARAMETER KillHub
  With -ForceKill, also stop Unity Hub / Tuanjie Hub (usually unnecessary for EBUSY).

.EXAMPLE
  .\scripts\clean_unity_library.ps1

.EXAMPLE
  .\scripts\clean_unity_library.ps1 -Full -ForceKill
#>
param(
    [switch]$Full,
    [switch]$ForceKill,
    [switch]$KillHub
)

$ErrorActionPreference = 'Stop'
$root = git rev-parse --show-toplevel
Set-Location $root

. (Join-Path $PSScriptRoot '_tuanjie_common.ps1')

# Editor locks PackageCache; Hub alone is usually safe to ignore.
$editorNames = @("Unity", "Tuanjie")
$hubNames = @("Unity Hub", "Tuanjie Hub", "UnityPackageManager")
$editors = Get-Process -Name $editorNames -ErrorAction SilentlyContinue
$hubs = Get-Process -Name $hubNames -ErrorAction SilentlyContinue

if ($editors) {
    if ($ForceKill) {
        Write-Host "Stopping Unity/Tuanjie Editor..."
        $editors | Stop-Process -Force -ErrorAction SilentlyContinue
        Start-Sleep -Seconds 2
    }
    else {
        Write-Host "Editor still running. Close it first, or use -ForceKill:"
        $editors | Format-Table Name, Id -AutoSize
        exit 1
    }
}

if ($hubs) {
    if ($KillHub -and $ForceKill) {
        Write-Host "Stopping Hub / Package Manager..."
        $hubs | Stop-Process -Force -ErrorAction SilentlyContinue
        Start-Sleep -Seconds 2
    }
    else {
        Write-Host "Note: Hub is running (OK). Use -ForceKill -KillHub only if clean still fails."
    }
}

function Remove-DirSafe {
    param([string]$Path)

    if (-not (Test-Path $Path)) {
        return
    }

    Write-Host "Removing: $Path"
    Remove-Item -LiteralPath $Path -Recurse -Force -ErrorAction Stop
}

$targets = @(
    (Join-Path $root "Temp")
    (Join-Path $root "obj")
    (Join-Path $root "Library/PackageCache")
    (Join-Path $root "Library/ScriptAssemblies")
    (Join-Path $root "Library/Bee")
    (Join-Path $root "Library/StateCache")
    (Join-Path $root "Library/SourceAssetDB")
    (Join-Path $root "Library/SourceAssetDB-lock")
    (Join-Path $root "Library/ArtifactDB")
    (Join-Path $root "Library/ArtifactDB-lock")
    (Join-Path $root "Library/ilpp.pid")
)

$packageCache = Join-Path $root "Library/PackageCache"
if (Test-Path $packageCache) {
    Get-ChildItem -LiteralPath $packageCache -Filter ".tmp-*" -Directory -ErrorAction SilentlyContinue |
        ForEach-Object { $targets += $_.FullName }
}

if ($Full) {
    $targets = @((Join-Path $root "Library"), (Join-Path $root "Temp"), (Join-Path $root "obj"))
}

foreach ($dir in ($targets | Select-Object -Unique)) {
    try {
        Remove-DirSafe -Path $dir
    }
    catch {
        Write-Host "Failed to remove (file locked): $dir"
        Write-Host $_.Exception.Message
        Write-Host ""
        Write-Host "In Tuanjie Package Manager error dialog: click Exit, NOT Continue."
        Write-Host "Then retry: .\scripts\clean_unity_library.ps1 -Full -ForceKill"
        exit 1
    }
}

Write-Host ''
Write-Host 'Done. Next steps:'
Write-Host "  1. Open project in Tuanjie $RequiredTuanjieVersion ($RequiredEditorVersion) via Hub"
Write-Host '  2. Run: .\scripts\align_tuanjie_editor.ps1'
Write-Host '  3. Wait for Package Manager to finish (no red errors in Console)'
Write-Host '  4. If needed: .\3Party\fetch_3party.ps1 -Offline'
Write-Host '  5. When Console is clean: menu RPG / Setup Boot Scene'
