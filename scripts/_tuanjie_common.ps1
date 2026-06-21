# Shared Tuanjie/Unity Hub helpers for RPG_Client (团结引擎 1.6.11).

$script:RequiredEditorVersion = '2022.3.61t12'
$script:RequiredTuanjieVersion = '1.6.11'
$script:RequiredInputSystem = '1.14.4-t1'
$script:RequiredUrp = '14.2.0-t1'

function Get-RepoRoot {
    $root = git rev-parse --show-toplevel 2>$null
    if (-not $root) {
        throw 'Not inside a git repository.'
    }
    return $root
}

function Get-ProjectUnityVersion {
    param([string]$Root = (Get-RepoRoot))

    $versionFile = Join-Path $Root 'ProjectSettings/ProjectVersion.txt'
    if (-not (Test-Path $versionFile)) {
        return $script:RequiredEditorVersion
    }

    foreach ($line in Get-Content $versionFile) {
        if ($line -match '^m_EditorVersion:\s*(.+)$') {
            return $Matches[1].Trim()
        }
    }

    return $script:RequiredEditorVersion
}

function Get-TuanjieHubRoots {
    return @(
        'D:\Program Files\Tuanjie\Hub\Editor'
        (Join-Path $env:ProgramFiles 'Tuanjie\Hub\Editor')
        (Join-Path ${env:ProgramFiles(x86)} 'Tuanjie\Hub\Editor')
        (Join-Path $env:ProgramFiles 'Unity\Hub\Editor')
        (Join-Path ${env:ProgramFiles(x86)} 'Unity\Hub\Editor')
    )
}

function Get-EditorExeInVersionDir {
    param([string]$VersionDir)

    foreach ($name in @('Tuanjie.exe', 'Unity.exe')) {
        $candidate = Join-Path $VersionDir "Editor\$name"
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    return $null
}

function Find-TuanjieEditorExe {
    param(
        [string]$ExpectedVersion = $script:RequiredEditorVersion,
        [string]$PreferredPath = ''
    )

    if (-not [string]::IsNullOrWhiteSpace($PreferredPath)) {
        if (Test-Path $PreferredPath) {
            return (Resolve-Path $PreferredPath).Path
        }
        throw "UnityPath not found: $PreferredPath"
    }

    foreach ($hubRoot in Get-TuanjieHubRoots) {
        if (-not (Test-Path $hubRoot)) { continue }
        $versionDir = Join-Path $hubRoot $ExpectedVersion
        $exe = Get-EditorExeInVersionDir -VersionDir $versionDir
        if ($exe) {
            return $exe
        }
    }

    return $null
}

function Resolve-UnityExe {
    param(
        [string]$PreferredPath = '',
        [string]$ExpectedVersion = $script:RequiredEditorVersion,
        [switch]$AllowFallback
    )

    $exe = Find-TuanjieEditorExe -ExpectedVersion $ExpectedVersion -PreferredPath $PreferredPath
    if ($exe) { return $exe }

    if ($AllowFallback) {
        foreach ($hubRoot in Get-TuanjieHubRoots) {
            if (-not (Test-Path $hubRoot)) { continue }
            $newest = Get-ChildItem -Path $hubRoot -Directory -ErrorAction SilentlyContinue |
                Sort-Object Name -Descending |
                Select-Object -First 1
            if ($newest) {
                $candidate = Get-EditorExeInVersionDir -VersionDir $newest.FullName
                if ($candidate) {
                    Write-Warning "Required $ExpectedVersion not found; using $($newest.Name)"
                    return $candidate
                }
            }
        }
    }

    return $null
}

function Assert-Tuanjie1611Installed {
    param([string]$PreferredPath = '')

    $exe = Resolve-UnityExe -PreferredPath $PreferredPath -ExpectedVersion $script:RequiredEditorVersion
    if (-not $exe) {
        throw "Tuanjie $script:RequiredTuanjieVersion (Editor $script:RequiredEditorVersion) is not installed.`nInstall it from Tuanjie Hub before opening or building this project.`nExpected: ${env:ProgramFiles}\Tuanjie\Hub\Editor\$script:RequiredEditorVersion\Editor\Tuanjie.exe"
    }
    return $exe
}

function Test-ProjectMatches1611 {
    param([string]$Root = (Get-RepoRoot))

    $versionFile = Join-Path $Root 'ProjectSettings/ProjectVersion.txt'
    $manifestFile = Join-Path $Root 'Packages/manifest.json'
    $issues = New-Object System.Collections.Generic.List[string]

    if (Test-Path $versionFile) {
        $text = Get-Content $versionFile -Raw
        if ($text -notmatch [regex]::Escape($script:RequiredEditorVersion)) {
            $issues.Add("ProjectVersion.txt 不是 $script:RequiredEditorVersion")
        }
        if ($text -notmatch [regex]::Escape($script:RequiredTuanjieVersion)) {
            $issues.Add("ProjectVersion.txt 不是 Tuanjie $script:RequiredTuanjieVersion")
        }
    }
    else {
        $issues.Add('缺少 ProjectSettings/ProjectVersion.txt')
    }

    if (Test-Path $manifestFile) {
        $manifest = Get-Content $manifestFile -Raw | ConvertFrom-Json
        $input = $manifest.dependencies.'com.unity.inputsystem'
        $urp = $manifest.dependencies.'com.unity.render-pipelines.universal'
        if ($input -ne $script:RequiredInputSystem) {
            $issues.Add("manifest inputsystem expected $script:RequiredInputSystem (current $input)")
        }
        if ($urp -ne $script:RequiredUrp) {
            $issues.Add("manifest URP expected $script:RequiredUrp (current $urp)")
        }
    }
    else {
        $issues.Add('缺少 Packages/manifest.json')
    }

    $exe = Find-TuanjieEditorExe -ExpectedVersion $script:RequiredEditorVersion
    if (-not $exe) {
        $issues.Add("Hub 未安装 Editor $script:RequiredEditorVersion")
    }

    return @{
        Ok     = ($issues.Count -eq 0)
        Issues = $issues
        Exe    = $exe
    }
}
