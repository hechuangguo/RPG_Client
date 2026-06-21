<#
.SYNOPSIS
  将 3Party/google.protobuf 中的 DLL 同步到 assets/_Project/Plugins/。

.EXAMPLE
  .\scripts\fetch_google_protobuf.ps1

.EXAMPLE
  .\scripts\fetch_google_protobuf.ps1 -Offline
#>
param(
    [switch]$Offline
)

$ErrorActionPreference = 'Stop'
$repoRoot = git rev-parse --show-toplevel 2>$null
if (-not $repoRoot) { throw 'Run from RPG_Client repo root.' }

$fetch = Join-Path $repoRoot '3Party\fetch_3party.ps1'
$args = @('-GoogleProtobufOnly')
if ($Offline) { $args += '-Offline' }

& $fetch @args
if ($LASTEXITCODE -ne 0) {
    throw "fetch_3party failed with exit code $LASTEXITCODE"
}
