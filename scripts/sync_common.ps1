# Fetch latest RPG_Common main and update Common submodule pointer
param(
    [switch]$Offline
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '_common_guard.ps1')

$repoRoot = git rev-parse --show-toplevel 2>$null
if (-not $repoRoot) {
    throw 'Not inside a git repository.'
}

Set-Location $repoRoot
if (Test-CommonHasUncommittedChanges -Root $repoRoot) {
    Write-Warning 'Common has local uncommitted changes; submodule update --remote may conflict. Commit or stash first.'
}
Write-CommonProtoDirtyNotice -Root $repoRoot

Write-Host 'Updating Common from RPG_Common ...'
git submodule update --init Common
if ($LASTEXITCODE -ne 0) { throw 'submodule update --init Common failed' }

if (-not $Offline) {
    git submodule update --remote Common
    if ($LASTEXITCODE -ne 0) { throw 'submodule update --remote Common failed' }
}
else {
    Write-Host 'Offline: skip submodule update --remote'
}

Write-Host 'Run from repo root to record new pointer:'
Write-Host '  git add Common'
Write-Host '  git commit -m "chore: sync Common submodule"'
Write-Host ''
Write-Host 'Then regenerate client protobuf:'
Write-Host '  .\scripts\sync_protobuf.ps1'
