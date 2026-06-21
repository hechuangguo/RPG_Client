<#
.SYNOPSIS
  准备 3Party 构建依赖（protoc、Google.Protobuf），支持离线。

.PARAMETER Offline
  仅使用本地 zip/nupkg，不访问网络。

.PARAMETER ProtocOnly
  只处理 protoc。

.PARAMETER GoogleProtobufOnly
  只处理 Google.Protobuf。

.EXAMPLE
  .\3Party\fetch_3party.ps1

.EXAMPLE
  .\3Party\fetch_3party.ps1 -Offline
#>
param(
    [switch]$Offline,
    [switch]$ProtocOnly,
    [switch]$GoogleProtobufOnly
)

$ErrorActionPreference = 'Stop'

$repoRoot = git rev-parse --show-toplevel 2>$null
if (-not $repoRoot) {
    throw 'Run from RPG_Client repository root.'
}
Set-Location $repoRoot

$script:ProtocVersion = '25.3'
$script:ProtocZipName = 'protoc.zip'
$script:ProtocUrl = "https://github.com/protocolbuffers/protobuf/releases/download/v$($script:ProtocVersion)/protoc-$($script:ProtocVersion)-win64.zip"
$script:GoogleProtobufVersion = '3.25.3'
$script:GoogleProtobufNupkg = "Google.Protobuf.$($script:GoogleProtobufVersion).nupkg"
$script:GoogleProtobufUrl = "https://www.nuget.org/api/v2/package/Google.Protobuf/$($script:GoogleProtobufVersion)"

$protocDir = Join-Path $repoRoot '3Party\protoc'
$protocZip = Join-Path $protocDir $script:ProtocZipName
$protocExe = Join-Path $protocDir 'bin\protoc.exe'

$gpDir = Join-Path $repoRoot '3Party\google.protobuf'
$gpNupkg = Join-Path $gpDir $script:GoogleProtobufNupkg
$gpDll = Join-Path $gpDir 'lib\netstandard2.0\Google.Protobuf.dll'

$pluginsDir = Join-Path $repoRoot 'assets\_Project\Plugins'
$pluginsDll = Join-Path $pluginsDir 'Google.Protobuf.dll'

function Expand-NupkgDll {
    param(
        [string]$NupkgPath,
        [string]$DestDllPath
    )

    $temp = Join-Path $env:TEMP "Google.Protobuf_extract_$(Get-Random)"
    try {
        New-Item -ItemType Directory -Force -Path $temp | Out-Null
        Copy-Item -LiteralPath $NupkgPath -Destination (Join-Path $temp 'pkg.zip') -Force
        Expand-Archive -LiteralPath (Join-Path $temp 'pkg.zip') -DestinationPath $temp -Force
        $src = Join-Path $temp 'lib\netstandard2.0\Google.Protobuf.dll'
        if (-not (Test-Path $src)) {
            throw "Google.Protobuf.dll not found in nupkg: $NupkgPath"
        }
        New-Item -ItemType Directory -Force -Path (Split-Path $DestDllPath -Parent) | Out-Null
        Copy-Item -LiteralPath $src -Destination $DestDllPath -Force
    }
    finally {
        if (Test-Path $temp) {
            Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
}

function Ensure-ProtocBundle {
    if (Test-Path $protocExe) {
        Write-Host "protoc OK: $protocExe"
        return
    }

    New-Item -ItemType Directory -Force -Path $protocDir | Out-Null

    if (-not (Test-Path $protocZip)) {
        if ($Offline) {
            throw "Offline: missing $protocZip . Run online once: .\3Party\fetch_3party.ps1"
        }
        Write-Host "Downloading protoc v$($script:ProtocVersion) ..."
        Invoke-WebRequest -Uri $script:ProtocUrl -OutFile $protocZip -UseBasicParsing
    }

    Write-Host "Extracting $protocZip ..."
    Expand-Archive -LiteralPath $protocZip -DestinationPath $protocDir -Force
    if (-not (Test-Path $protocExe)) {
        throw "protoc extract failed: $protocExe"
    }
    Write-Host "protoc ready: $protocExe"
}

function Ensure-GoogleProtobufBundle {
    New-Item -ItemType Directory -Force -Path $gpDir | Out-Null

    if (-not (Test-Path $gpDll)) {
        if (-not (Test-Path $gpNupkg)) {
            if ($Offline) {
                throw "Offline: missing $gpNupkg . Run online once: .\3Party\fetch_3party.ps1"
            }
            Write-Host "Downloading Google.Protobuf $($script:GoogleProtobufVersion) ..."
            Invoke-WebRequest -Uri $script:GoogleProtobufUrl -OutFile $gpNupkg -UseBasicParsing
        }
        Write-Host "Extracting Google.Protobuf from nupkg ..."
        Expand-NupkgDll -NupkgPath $gpNupkg -DestDllPath $gpDll
    }

    New-Item -ItemType Directory -Force -Path $pluginsDir | Out-Null
    Copy-Item -LiteralPath $gpDll -Destination $pluginsDll -Force
    Write-Host "Google.Protobuf OK: $gpDll"
    Write-Host "Copied to Plugins: $pluginsDll"
}

$doProtoc = -not $GoogleProtobufOnly
$doGp = -not $ProtocOnly

if ($doProtoc) {
    Ensure-ProtocBundle
}
if ($doGp) {
    Ensure-GoogleProtobufBundle
}

Write-Host '3Party fetch done.'
