# Fetch latest RPG_Common main and update Client/Common submodule pointer
$ErrorActionPreference = "Stop"

$repoRoot = git rev-parse --show-toplevel 2>$null
if (-not $repoRoot) {
    throw "Not inside a git repository."
}

Set-Location $repoRoot
Write-Host "Updating Client/Common from RPG_Common remote ..."
git submodule update --init Client/Common
git submodule update --remote Client/Common

$status = git -C Client/Common status --porcelain
if ($status) {
    Write-Warning "Client/Common has uncommitted changes. Commit in RPG_Common first."
}

Write-Host "Run from repo root to record new pointer:"
Write-Host "  git add Client/Common"
Write-Host "  git commit -m `"chore: sync Common submodule`""
