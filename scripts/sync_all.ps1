param(
    [switch]$AllowDirty
)

$ErrorActionPreference = "Stop"

function Invoke-Git {
    param(
        [Parameter(Mandatory = $true)][string]$Args
    )
    Write-Host "git $Args"
    & git $Args.Split(" ")
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed: git $Args"
    }
}

$repoRoot = git rev-parse --show-toplevel 2>$null
if (-not $repoRoot) {
    throw "Not inside a git repository."
}

Set-Location $repoRoot
Write-Host "Repo root: $repoRoot"

$dirty = git status --porcelain
if ($dirty -and -not $AllowDirty) {
    throw @"
Working tree is not clean.
Please commit/stash your changes first, or run:
  .\sync_all.bat
"@
}

$currentBranch = git branch --show-current
if (-not $currentBranch) {
    throw "Detached HEAD detected. Please checkout a branch first."
}

Write-Host ""
Write-Host "=== 1) Update main repository ($currentBranch) ==="
Invoke-Git "fetch --all --prune"
Invoke-Git "pull --ff-only origin $currentBranch"

Write-Host ""
Write-Host "=== 2) Init/sync submodules (including Common) ==="
Invoke-Git "submodule sync --recursive"
Invoke-Git "submodule update --init --recursive"

Write-Host ""
Write-Host "=== 3) Pull latest for each submodule tracking branch ==="
Invoke-Git "submodule update --remote --recursive"

Write-Host ""
Write-Host "=== Done ==="
Write-Host "Main HEAD: $(git rev-parse --short HEAD)"
Write-Host "Submodule status:"
git submodule status --recursive
