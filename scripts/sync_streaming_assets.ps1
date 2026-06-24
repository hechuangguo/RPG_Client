<#
.SYNOPSIS
  将 config/script/database 同步到 StreamingAssets（Unity 运行时只读路径）。

.EXAMPLE
  .\scripts\sync_streaming_assets.ps1
#>
$ErrorActionPreference = "Stop"
$root = git rev-parse --show-toplevel
if (-not $root) { throw "Run from repo root." }
Set-Location $root

$dest = Join-Path $root "assets\StreamingAssets"
New-Item -ItemType Directory -Force -Path $dest | Out-Null

foreach ($dir in @("config", "script", "database", "basefile", "map")) {
    $src = Join-Path $root $dir
    if (-not (Test-Path $src)) { continue }
    $target = Join-Path $dest $dir
    if (Test-Path $target) { Remove-Item $target -Recurse -Force }
    Copy-Item $src $target -Recurse -Force
    Write-Host "Synced $dir -> assets/StreamingAssets/$dir"
}

Write-Host "Done."
