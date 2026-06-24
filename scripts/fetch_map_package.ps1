<#
.SYNOPSIS
  SceneServer 集成测试：从 CDN 拉取 server 地图包到本地缓存。

.PARAMETER MapId
  地图 ID，默认 1002。

.PARAMETER CdnBaseUrl
  CDN 根 URL（须以 / 结尾或脚本会自动补全）。

.PARAMETER OutputDir
  本地缓存目录，默认 build/map_cache/data/map

.EXAMPLE
  .\scripts\fetch_map_package.ps1 -MapId 1002 -CdnBaseUrl http://127.0.0.1:8765/rpg/map/
#>
param(
    [int]$MapId = 1002,
    [string]$CdnBaseUrl = 'http://127.0.0.1:8765/rpg/map/',
    [string]$OutputDir = ''
)

$ErrorActionPreference = 'Stop'
$root = git rev-parse --show-toplevel
if (-not $OutputDir) {
    $OutputDir = Join-Path $root "build/map_cache/data/map/$MapId"
}

if (-not $CdnBaseUrl.EndsWith('/')) {
    $CdnBaseUrl += '/'
}

function Get-RemoteJson {
    param([string]$Url)
    Write-Host "GET $Url"
    return Invoke-RestMethod -Uri $Url -Method Get -TimeoutSec 30
}

function Save-RemoteFile {
    param([string]$Url, [string]$DestPath)
    Write-Host "GET $Url -> $DestPath"
    $dir = Split-Path $DestPath -Parent
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    Invoke-WebRequest -Uri $Url -OutFile $DestPath -TimeoutSec 60
}

$indexUrl = "${CdnBaseUrl}index.json"
$index = Get-RemoteJson -Url $indexUrl
$entry = $index.maps | Where-Object { $_.mapId -eq $MapId } | Select-Object -First 1
if (-not $entry) {
    throw "mapId $MapId not found in index.json"
}

$manifestUrl = "${CdnBaseUrl}server/$MapId/manifest.json"
$manifest = Get-RemoteJson -Url $manifestUrl

if ($manifest.packageVersion -ne $entry.packageVersion) {
    Write-Warning "packageVersion mismatch: index=$($entry.packageVersion) manifest=$($manifest.packageVersion)"
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$manifestPath = Join-Path $OutputDir 'manifest.json'
$manifest | ConvertTo-Json -Depth 10 | Set-Content -Path $manifestPath -Encoding UTF8

$files = @(
    'logic/spawn_points.json',
    'logic/npc_spawns.json',
    'collision/navmesh.bin'
)

foreach ($rel in $files) {
    $url = "${CdnBaseUrl}server/$MapId/$rel"
    $dest = Join-Path $OutputDir ($rel -replace '/', [IO.Path]::DirectorySeparatorChar)
    Save-RemoteFile -Url $url -DestPath $dest
}

$buildingsUrl = "${CdnBaseUrl}client/$MapId/scene/buildings.json"
$buildingsDest = Join-Path $OutputDir 'buildings.json'
try {
    Save-RemoteFile -Url $buildingsUrl -DestPath $buildingsDest
}
catch {
    Write-Warning "optional buildings.json not fetched: $_"
}

Write-Host ''
Write-Host "Map $MapId cached to $OutputDir"
Write-Host "packageVersion: $($manifest.packageVersion)"
Write-Host "contentHash: $($manifest.contentHash)"
