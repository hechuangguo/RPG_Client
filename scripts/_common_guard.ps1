# Common submodule helpers for RPG_Client.
# Client and Server both edit RPG_Common via the Common/ submodule.

function Get-CommonPath {
    param([string]$Root = (git rev-parse --show-toplevel))

    return Join-Path $Root 'Common'
}

function Test-CommonInitialized {
    param([string]$Root = (git rev-parse --show-toplevel))

    $commonPath = Get-CommonPath -Root $Root
    return Test-Path (Join-Path $commonPath '.git')
}

function Get-CommonProtoDirtyStatus {
    param([string]$Root = (git rev-parse --show-toplevel))

    if (-not (Test-CommonInitialized -Root $Root)) {
        return @()
    }

    $commonPath = Get-CommonPath -Root $Root
    $lines = git -C $commonPath status --porcelain -- '*.proto' 2>$null
    if (-not $lines) {
        return @()
    }

    return @($lines | Where-Object { $_ -match '\S' })
}

function Write-CommonProtoDirtyNotice {
    param([string]$Root = (git rev-parse --show-toplevel))

    $dirty = Get-CommonProtoDirtyStatus -Root $Root
    if ($dirty.Count -eq 0) {
        return
    }

    $commonPath = Get-CommonPath -Root $Root
    Write-Host ''
    Write-Host 'Notice: Common/*.proto has local changes.' -ForegroundColor Yellow
    foreach ($line in $dirty) {
        Write-Host "  $line"
    }
    Write-Host ''
    Write-Host 'Diff summary:'
    git -C $commonPath diff --stat -- '*.proto' 2>$null
    Write-Host ''
}

function Test-CommonHasUncommittedChanges {
    param([string]$Root = (git rev-parse --show-toplevel))

    if (-not (Test-CommonInitialized -Root $Root)) {
        return $false
    }

    $commonPath = Get-CommonPath -Root $Root
    return [bool](git -C $commonPath status --porcelain)
}

# Backward-compatible alias; no longer blocks commits.
function Assert-CommonProtoReadonly {
    param(
        [switch]$AllowCommonEdit,
        [string]$Root = (git rev-parse --show-toplevel)
    )

    Write-CommonProtoDirtyNotice -Root $Root
}
