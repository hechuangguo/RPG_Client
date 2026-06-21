param(
    [switch]$AllowDirty,
    [switch]$AllowCommonEdit,
    [switch]$Offline
)

$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot '_common_guard.ps1')

function Invoke-Git {
    param(
        [Parameter(Mandatory = $true)][string]$Args
    )
    Write-Host "git $Args"
    & git $Args.Split(' ')
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed: git $Args"
    }
}

$repoRoot = git rev-parse --show-toplevel 2>$null
if (-not $repoRoot) {
    throw 'Not inside a git repository.'
}

Set-Location $repoRoot
Write-Host "Repo root: $repoRoot"
if ($Offline) {
    Write-Host 'Offline mode: skip fetch/pull and submodule --remote.'
}

Assert-CommonProtoReadonly -AllowCommonEdit:$AllowCommonEdit -Root $repoRoot

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
    throw 'Detached HEAD detected. Please checkout a branch first.'
}

if (-not $Offline) {
    Write-Host ''
    Write-Host "=== 1) Update main repository ($currentBranch) ==="
    try {
        Invoke-Git 'fetch --all --prune'
        Invoke-Git "pull --ff-only origin $currentBranch"
    }
    catch {
        Write-Host ''
        Write-Host 'Remote update failed (often network/proxy/firewall).' -ForegroundColor Yellow
        Write-Host 'If you only need local sync, run:' -ForegroundColor Yellow
        Write-Host '  .\sync_all.bat -Offline' -ForegroundColor Yellow
        throw
    }
}
else {
    Write-Host ''
    Write-Host '=== 1) Skip remote main repository update (Offline) ==='
}

Write-Host ''
Write-Host '=== 2) Init/sync submodules ==='
Invoke-Git 'submodule sync --recursive'
Invoke-Git 'submodule update --init --recursive'

if (-not $Offline) {
    Write-Host ''
    Write-Host '=== 3) Pull latest Common from remote ==='
    try {
        Invoke-Git 'submodule update --remote --recursive'
    }
    catch {
        Write-Host ''
        Write-Host 'Submodule remote update failed.' -ForegroundColor Yellow
        throw
    }
}
else {
    Write-Host ''
    Write-Host '=== 3) Skip submodule --remote (Offline) ==='
}

Write-Host ''
Write-Host '=== 4) Prepare 3Party (protoc + Google.Protobuf) ==='
$fetch = Join-Path $repoRoot '3Party\fetch_3party.ps1'
$fetchArgs = @()
if ($Offline) { $fetchArgs += '-Offline' }
& $fetch @fetchArgs
if ($LASTEXITCODE -ne 0) {
    throw "fetch_3party.ps1 failed with exit code $LASTEXITCODE"
}

Write-Host ''
Write-Host '=== 5) Regenerate Protobuf/*.cs from Common ==='
$syncProto = Join-Path $repoRoot 'scripts\sync_protobuf.ps1'
$protoArgs = @()
if ($Offline) { $protoArgs += '-Offline' }
& $syncProto @protoArgs
if ($LASTEXITCODE -ne 0) {
    throw "sync_protobuf.ps1 failed with exit code $LASTEXITCODE"
}

Write-Host ''
Write-Host '=== 6) Sync StreamingAssets ==='
$syncStreaming = Join-Path $repoRoot 'scripts\sync_streaming_assets.ps1'
& $syncStreaming
if ($LASTEXITCODE -ne 0) {
    throw "sync_streaming_assets.ps1 failed with exit code $LASTEXITCODE"
}

Write-Host ''
Write-Host '=== Done ==='
Write-Host "Main HEAD: $(git rev-parse --short HEAD)"
Write-Host 'Submodule status:'
git submodule status --recursive
