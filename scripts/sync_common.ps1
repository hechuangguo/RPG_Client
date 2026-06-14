# Fetch latest RPG_Common main and update Common submodule pointer
$ErrorActionPreference = "Stop"

$repoRoot = git rev-parse --show-toplevel 2>$null
if (-not $repoRoot) {
    throw "Not inside a git repository."
}

Set-Location $repoRoot
Write-Host "Updating Common from RPG_Common remote ..."
git submodule update --init Common
git submodule update --remote Common

$status = git -C Common status --porcelain
if ($status) {
    Write-Warning "Common has uncommitted changes. Commit in RPG_Common first."
}

Write-Host "Run from repo root to record new pointer:"
Write-Host "  git add Common"
Write-Host "  git commit -m `"chore: sync Common submodule`""
