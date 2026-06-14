# Initialize RPG_Common submodule (run from repo root)
$ErrorActionPreference = "Stop"

$repoRoot = git rev-parse --show-toplevel 2>$null
if (-not $repoRoot) {
    throw "Not inside a git repository."
}

Set-Location $repoRoot
Write-Host "Initializing submodules in $repoRoot ..."
git submodule update --init --recursive
Write-Host "Done. Common headers: Common/"
