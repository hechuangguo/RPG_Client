<#
.SYNOPSIS
  Verify project is pinned to Tuanjie 1.6.11 (2022.3.61t12).

.DESCRIPTION
  Does NOT auto-align to other Hub versions. Use when opening the project or before CI build.

.EXAMPLE
  .\scripts\align_tuanjie_editor.ps1
#>
$ErrorActionPreference = 'Stop'
$root = git rev-parse --show-toplevel
Set-Location $root

. (Join-Path $PSScriptRoot '_tuanjie_common.ps1')

$result = Test-ProjectMatches1611 -Root $root
if ($result.Ok) {
    Write-Host "OK: 团结引擎 $RequiredTuanjieVersion / Editor $RequiredEditorVersion"
    Write-Host "Editor: $($result.Exe)"
    exit 0
}

Write-Host 'Project or Hub does not match Tuanjie 1.6.11:' -ForegroundColor Red
foreach ($issue in $result.Issues) {
    Write-Host "  - $issue"
}
Write-Host ''
Write-Host 'Fix: install Tuanjie 1.6.11 in Hub, ensure ProjectVersion.txt and Packages/manifest.json match repo.'
Write-Host 'Do NOT open this project with 2022.3.62f3c1 or other editors.'
exit 1
