# Commit and push RPG_Client + Common submodule (when dirty).
# Usage: .\scripts\commit_push_all.ps1 -m "提交说明"
# Order: Common (RPG_Common) first, then main repo pointer + client code.
param(
    [Parameter(Mandatory = $true)]
    [Alias('m')]
    [string]$Message,
    [switch]$SkipCommon
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '_common_guard.ps1')
$GitUpstreamRef = '@{u}'

function Invoke-GitIn {
    param(
        [string]$WorkDir = "",
        [Parameter(Mandatory = $true, ValueFromRemainingArguments = $true)]
        [string[]]$GitArgs
    )
    $label = if ($WorkDir) { "($WorkDir) " } else { "" }
    Write-Host ("git " + $label + ($GitArgs -join " "))
    if ($WorkDir) {
        & git -C $WorkDir @GitArgs
    }
    else {
        & git @GitArgs
    }
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed: git $label$($GitArgs -join ' ')"
    }
}

function Get-GitUpstream {
    param([string]$WorkDir = "")
    if ($WorkDir) {
        return git -C $WorkDir rev-parse --abbrev-ref --symbolic-full-name $GitUpstreamRef 2>$null
    }
    return git rev-parse --abbrev-ref --symbolic-full-name $GitUpstreamRef 2>$null
}

function Get-CurrentBranch {
    param([string]$WorkDir = "")
    $branch = if ($WorkDir) {
        git -C $WorkDir rev-parse --abbrev-ref HEAD 2>$null
    }
    else {
        git rev-parse --abbrev-ref HEAD 2>$null
    }
    if (-not $branch -or $branch -eq "HEAD") {
        return $null
    }
    return $branch
}

function Test-HasUncommitted {
    param([string]$WorkDir = "")
    if ($WorkDir) {
        return [bool](git -C $WorkDir status --porcelain)
    }
    return [bool](git status --porcelain)
}

function Test-HasUnpushedCommits {
    param(
        [string]$WorkDir = "",
        [string]$Branch
    )
    if (-not $Branch) {
        return $false
    }
    $upstream = Get-GitUpstream -WorkDir $WorkDir
    if (-not $upstream) {
        return $true
    }
    if ($WorkDir) {
        $ahead = git -C $WorkDir rev-list --count "$upstream..HEAD" 2>$null
    }
    else {
        $ahead = git rev-list --count "$upstream..HEAD" 2>$null
    }
    return ($ahead -and [int]$ahead -gt 0)
}

function Test-IsAncestor {
    param(
        [string]$WorkDir = "",
        [string]$AncestorSha,
        [string]$DescendantSha
    )
    if ($WorkDir) {
        git -C $WorkDir merge-base --is-ancestor $AncestorSha $DescendantSha 2>$null
    }
    else {
        git merge-base --is-ancestor $AncestorSha $DescendantSha 2>$null
    }
    return ($LASTEXITCODE -eq 0)
}

function Ensure-OnBranch {
    param(
        [string]$WorkDir = "",
        [string]$Branch = "main"
    )
    $label = if ($WorkDir) { $WorkDir } else { "repo" }
    $currentBranch = Get-CurrentBranch -WorkDir $WorkDir

    if ($currentBranch -eq $Branch) {
        return
    }

    $detachedSha = $null
    if (-not $currentBranch) {
        if ($WorkDir) {
            $detachedSha = git -C $WorkDir rev-parse HEAD
        }
        else {
            $detachedSha = git rev-parse HEAD
        }
        $shortSha = $detachedSha.Substring(0, 7)
        Write-Host "${label}: detached HEAD (${shortSha}), checkout ${Branch} ..."
    }
    else {
        Write-Host "${label}: branch ${currentBranch}, checkout ${Branch} ..."
    }

    Invoke-GitIn -WorkDir $WorkDir -GitArgs fetch, origin
    Invoke-GitIn -WorkDir $WorkDir -GitArgs checkout, $Branch

    $remoteRef = "origin/$Branch"
    $remoteExists = if ($WorkDir) {
        git -C $WorkDir rev-parse --verify $remoteRef 2>$null
    }
    else {
        git rev-parse --verify $remoteRef 2>$null
    }
    if ($remoteExists) {
        try {
            Invoke-GitIn -WorkDir $WorkDir -GitArgs merge, --ff-only, $remoteRef
        }
        catch {
            Write-Warning "${label}: cannot fast-forward to ${remoteRef}; resolve manually before push."
        }
    }

    if ($detachedSha) {
        $headSha = if ($WorkDir) {
            git -C $WorkDir rev-parse HEAD
        }
        else {
            git rev-parse HEAD
        }
        if (-not (Test-IsAncestor -WorkDir $WorkDir -AncestorSha $detachedSha -DescendantSha $headSha)) {
            $shortSha = $detachedSha.Substring(0, 7)
            Write-Host "${label}: cherry-pick detached commit ${shortSha} onto ${Branch} ..."
            Invoke-GitIn -WorkDir $WorkDir -GitArgs cherry-pick, $detachedSha
        }
    }
}

function Get-AheadBehind {
    param(
        [string]$WorkDir = "",
        [string]$Upstream
    )
    if ($WorkDir) {
        $ahead = git -C $WorkDir rev-list --count "$Upstream..HEAD" 2>$null
        $behind = git -C $WorkDir rev-list --count "HEAD..$Upstream" 2>$null
    }
    else {
        $ahead = git rev-list --count "$Upstream..HEAD" 2>$null
        $behind = git rev-list --count "HEAD..$Upstream" 2>$null
    }
    return @{
        Ahead  = if ($ahead) { [int]$ahead } else { 0 }
        Behind = if ($behind) { [int]$behind } else { 0 }
    }
}

function Sync-WithRemote {
    param(
        [string]$WorkDir = "",
        [string]$Branch,
        [switch]$Submodule
    )
    $label = if ($WorkDir) { $WorkDir } else { "repo" }

    Invoke-GitIn -WorkDir $WorkDir -GitArgs fetch, origin

    $remoteRef = "origin/$Branch"
    $counts = Get-AheadBehind -WorkDir $WorkDir -Upstream $remoteRef
    if ($counts.Behind -le 0) {
        return
    }

    if ($Submodule -and $counts.Ahead -gt 0) {
        if (Test-HasUncommitted -WorkDir $WorkDir) {
            throw "${label}: submodule diverged with uncommitted changes; commit or stash first."
        }
        Write-Host "${label}: submodule diverged (ahead $($counts.Ahead), behind $($counts.Behind)), reset to ${remoteRef} ..."
        Invoke-GitIn -WorkDir $WorkDir -GitArgs reset, --hard, $remoteRef
        return
    }

    if ($Submodule) {
        Write-Host "${label}: behind remote by $($counts.Behind) commits, fast-forward to ${remoteRef} ..."
        Invoke-GitIn -WorkDir $WorkDir -GitArgs merge, --ff-only, $remoteRef
        return
    }

    Write-Host "${label}: behind remote by $($counts.Behind) commits, pull --rebase ..."
    try {
        Invoke-GitIn -WorkDir $WorkDir -GitArgs pull, --rebase, origin, $Branch
    }
    catch {
        if ($WorkDir) {
            git -C $WorkDir rebase --abort 2>$null
        }
        else {
            git rebase --abort 2>$null
        }
        throw "${label}: rebase conflict, aborted. Resolve manually then push (no force push)."
    }
}

function Commit-And-Push {
    param(
        [string]$WorkDir = "",
        [string]$Branch = "main"
    )
    $label = if ($WorkDir) { (Split-Path -Leaf $WorkDir) } else { "RPG_Client" }

    # 先在工作区提交，避免 detached HEAD + 本地改动时 checkout 覆盖文件
    if (Test-HasUncommitted -WorkDir $WorkDir) {
        Invoke-GitIn -WorkDir $WorkDir -GitArgs add, -A
        Invoke-GitIn -WorkDir $WorkDir -GitArgs commit, -m, $Message
        Write-Host "${label}: committed."
    }
    else {
        Write-Host "${label}: no uncommitted changes."
    }

    Ensure-OnBranch -WorkDir $WorkDir -Branch $Branch
    $branch = Get-CurrentBranch -WorkDir $WorkDir
    if (-not $branch) {
        throw "${label}: failed to checkout branch ${Branch}."
    }

    $isSubmodule = [bool]$WorkDir
    if ($isSubmodule) {
        Sync-WithRemote -WorkDir $WorkDir -Branch $branch -Submodule
    }

    $needsPush = Test-HasUnpushedCommits -WorkDir $WorkDir -Branch $branch
    if (-not $needsPush) {
        $upstream = Get-GitUpstream -WorkDir $WorkDir
        if (-not $upstream) {
            $needsPush = $true
        }
    }

    if ($needsPush) {
        if (-not $isSubmodule) {
            Sync-WithRemote -WorkDir $WorkDir -Branch $branch
        }
        Invoke-GitIn -WorkDir $WorkDir -GitArgs push, -u, origin, $branch
        Write-Host "${label}: pushed to origin/${branch}"
    }
    else {
        Write-Host "${label}: nothing to push."
    }
}

$repoRoot = git rev-parse --show-toplevel 2>$null
if (-not $repoRoot) {
    throw "Not inside a git repository."
}

Set-Location $repoRoot
Write-Host "Repo root: $repoRoot"
Write-Host "Commit message: $Message"
if ($SkipCommon) {
    Write-Host 'SkipCommon: Common submodule commit/push skipped.'
}
else {
    Write-Host 'Will commit/push Common submodule first when dirty.'
}
Write-Host ''

Write-CommonProtoDirtyNotice -Root $repoRoot

$commonPath = Join-Path $repoRoot 'Common'
if (-not $SkipCommon -and (Test-Path (Join-Path $commonPath '.git'))) {
    Write-Host '=== 1) Common submodule (RPG_Common) ==='
    Commit-And-Push -WorkDir $commonPath -Branch main
}
else {
    Write-Host '=== 1) Common submodule ==='
    if (Test-CommonHasUncommittedChanges -Root $repoRoot) {
        Write-Host 'Common has uncommitted changes (not pushed). Bump pointer in main repo commit if needed.'
    }
    else {
        Write-Host 'Skipped (no changes, or -SkipCommon).'
    }
}

Write-Host ""

Write-Host "=== 2) Main repository (RPG_Client) ==="
$mainBranch = Get-CurrentBranch
if (-not $mainBranch) {
    throw "Main repo is detached HEAD; checkout a branch first."
}
Commit-And-Push -Branch $mainBranch

Write-Host ""
Write-Host "=== Done ==="
Write-Host "Main HEAD: $(git rev-parse --short HEAD) ($(Get-CurrentBranch))"
if (Test-Path (Join-Path $commonPath ".git")) {
    Write-Host "Common HEAD: $(git -C Common rev-parse --short HEAD) ($(Get-CurrentBranch -WorkDir $commonPath))"
}
