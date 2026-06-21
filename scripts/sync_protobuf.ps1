<#
.SYNOPSIS
  Generate Protobuf/*.cs from Common/*.proto for Unity client.

.DESCRIPTION
  Uses 3Party/protoc/bin/protoc.exe only (via fetch_3party.ps1).
  Does not modify Common submodule (read-only).

.PARAMETER SkipJunction
  Skip creating assets/_Project/Protobuf junction to root Protobuf/.

.PARAMETER Offline
  Do not download protoc; require local 3Party bundle.

.EXAMPLE
  git submodule update --init Common
  .\scripts\sync_protobuf.ps1
#>
param(
    [switch]$SkipJunction,
    [switch]$Offline
)

$ErrorActionPreference = 'Stop'

$repoRoot = git rev-parse --show-toplevel 2>$null
if (-not $repoRoot) {
    throw 'Run this script from the RPG_Client git repository root.'
}
Set-Location $repoRoot

$fetch = Join-Path $repoRoot '3Party\fetch_3party.ps1'
$fetchArgs = @('-ProtocOnly')
if ($Offline) { $fetchArgs += '-Offline' }
& $fetch @fetchArgs
if ($LASTEXITCODE -ne 0) {
    throw "fetch_3party (protoc) failed with exit code $LASTEXITCODE"
}

$protoc = Join-Path $repoRoot '3Party\protoc\bin\protoc.exe'
if (-not (Test-Path $protoc)) {
    throw "protoc not found: $protoc"
}

if (-not (Test-Path 'Common\LoginMsg.proto')) {
    throw 'Common submodule missing .proto files. Run: git submodule update --init Common'
}

Write-Host "Using protoc: $protoc"
& $protoc --version

$outDir = Join-Path $repoRoot 'Protobuf'
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

$protoFiles = Get-ChildItem 'Common\*.proto' | ForEach-Object { $_.Name }
Write-Host "Generating C# into Protobuf/ ($($protoFiles.Count) proto files) ..."
& $protoc -I Common --csharp_out=$outDir @protoFiles
if ($LASTEXITCODE -ne 0) {
    throw "protoc failed with exit code $LASTEXITCODE"
}

$count = (Get-ChildItem $outDir -Filter '*.cs').Count
Write-Host "Generated $count .cs file(s)."

if (-not $SkipJunction) {
    $junction = Join-Path $repoRoot 'assets\_Project\Protobuf'
    $target = $outDir
    New-Item -ItemType Directory -Force -Path (Split-Path $junction -Parent) | Out-Null
    if (Test-Path $junction) {
        $item = Get-Item $junction -Force
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            Write-Host "Junction already exists: $junction"
        }
        else {
            Write-Warning 'assets/_Project/Protobuf exists and is not a junction; skip mklink.'
        }
    }
    else {
        cmd /c "mklink /J `"$junction`" `"$target`""
        Write-Host "Created junction: $junction -> Protobuf/"
    }
}

Write-Host 'Done.'
