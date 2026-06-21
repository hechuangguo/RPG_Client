<#
.SYNOPSIS
  Unity Windows Standalone 构建（batchmode，团结引擎 1.6.11）。

.PARAMETER OutputPath
  输出目录，默认 build/unity/bin

.PARAMETER UnityPath
  可选：Unity.exe 完整路径；省略则要求 Hub 已安装 2022.3.61t12

.EXAMPLE
  .\scripts\build_unity_client.ps1
#>
param(
    [string]$OutputPath = 'build/unity/bin',
    [string]$UnityPath = ''
)

$ErrorActionPreference = 'Stop'
$root = git rev-parse --show-toplevel
Set-Location $root

. (Join-Path $PSScriptRoot '_tuanjie_common.ps1')

$check = Test-ProjectMatches1611 -Root $root
if (-not $check.Ok) {
    foreach ($issue in $check.Issues) {
        Write-Host "  - $issue" -ForegroundColor Yellow
    }
    throw 'Project must match Tuanjie 1.6.11 before build. Run .\scripts\align_tuanjie_editor.ps1'
}

$unity = Assert-Tuanjie1611Installed -PreferredPath $UnityPath
Write-Host "Using Unity: $unity"

& (Join-Path $root '3Party\fetch_3party.ps1')
if ($LASTEXITCODE -ne 0) { throw 'fetch_3party failed' }

.\scripts\sync_streaming_assets.ps1
.\scripts\sync_protobuf.ps1 -SkipJunction

$out = Join-Path $root $OutputPath
New-Item -ItemType Directory -Force -Path $out | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $root 'logs') | Out-Null

Write-Host "Building Windows player -> $out"
& $unity -batchmode -nographics -quit `
    -projectPath $root `
    -buildWindows64Player (Join-Path $out 'RPGClient.exe') `
    -logFile (Join-Path $root 'logs/unity_build.log')

$builtExe = Join-Path $out 'RPGClient.exe'
if ($LASTEXITCODE -ne 0 -or -not (Test-Path $builtExe)) {
    throw "Unity build failed (exit=$LASTEXITCODE). See logs/unity_build.log"
}

Write-Host "Build OK: $out\RPGClient.exe"
