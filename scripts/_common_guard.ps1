# Common submodule read-only guard for RPG_Client.
# Client must not edit Common/*.proto without explicit -AllowCommonEdit.

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

function Assert-CommonProtoReadonly {
    param(
        [switch]$AllowCommonEdit,
        [string]$Root = (git rev-parse --show-toplevel)
    )

    if (-not (Test-CommonInitialized -Root $Root)) {
        return
    }

    $dirty = Get-CommonProtoDirtyStatus -Root $Root
    if ($dirty.Count -eq 0) {
        return
    }

    if ($AllowCommonEdit) {
        Write-Host 'Warning: Common/*.proto has local changes (-AllowCommonEdit).' -ForegroundColor Yellow
        return
    }

    $commonPath = Get-CommonPath -Root $Root
    Write-Host ''
    Write-Host 'Common/*.proto local changes detected:' -ForegroundColor Red
    foreach ($line in $dirty) {
        Write-Host "  $line"
    }
    Write-Host ''
    Write-Host 'Diff summary:'
    git -C $commonPath diff --stat -- '*.proto' 2>$null

    throw "Common/*.proto has local changes. Edit protocol in RPG_Common, not in client.`nIf you explicitly allow edits, pass -AllowCommonEdit."
}

function Test-CommonHasUncommittedChanges {
    param([string]$Root = (git rev-parse --show-toplevel))

    if (-not (Test-CommonInitialized -Root $Root)) {
        return $false
    }

    $commonPath = Get-CommonPath -Root $Root
    return [bool](git -C $commonPath status --porcelain)
}
