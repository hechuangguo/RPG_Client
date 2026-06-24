<#
.SYNOPSIS
  本地 HTTP 服务模拟地图 CDN。

.PARAMETER Port
  监听端口，默认 8765。

.PARAMETER Root
  CDN 根目录，默认 build/map_cdn/rpg/map

.EXAMPLE
  .\scripts\serve_map_cdn.ps1
#>
param(
    [int]$Port = 8765,
    [string]$Root = ''
)

$ErrorActionPreference = 'Stop'
$repoRoot = git rev-parse --show-toplevel
if (-not $Root) {
    $Root = Join-Path $repoRoot 'build/map_cdn/rpg/map'
}

if (-not (Test-Path $Root)) {
    throw "CDN root not found: $Root`nRun .\scripts\build_map_packages.ps1 first."
}

$serveRoot = Split-Path $Root -Parent
Write-Host "Serving $serveRoot at http://127.0.0.1:${Port}/"
Write-Host "Map CDN base URL: http://127.0.0.1:${Port}/rpg/map/"
Write-Host 'Press Ctrl+C to stop.'

Set-Location $serveRoot
python -m http.server $Port
