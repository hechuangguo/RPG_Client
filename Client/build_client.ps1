# Build RPGClient.exe (Windows x64, MSVC + Ninja)
$ErrorActionPreference = "Stop"

$clientDir = $PSScriptRoot
$buildDir = Join-Path $clientDir "build"

& (Join-Path $clientDir "assets\fonts\fetch_font.ps1")

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere not found. Install Visual Studio with C++ desktop development."
}

$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) {
    throw "MSVC C++ tools not found. Install 'Desktop development with C++' workload in Visual Studio Installer."
}

$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) {
    throw "vcvars64.bat not found at: $vcvars"
}

$cmakeCandidates = @(
    (Join-Path $vsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"),
    "C:\Program Files\CMake\bin\cmake.exe"
)
$cmake = $cmakeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $cmake) {
    throw "cmake.exe not found. Install CMake or VS CMake component."
}

$ninjaCandidates = @(
    (Join-Path $vsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"),
    "C:\Program Files\CMake\bin\ninja.exe"
)
$ninja = $ninjaCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $ninja) {
    throw "ninja.exe not found."
}

New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

$cmd = "call `"$vcvars`" >nul && `"$cmake`" -S `"$clientDir`" -B `"$buildDir`" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MAKE_PROGRAM=`"$ninja`" && `"$cmake`" --build `"$buildDir`" --config Release"
cmd.exe /c $cmd
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$exe = Join-Path $buildDir "bin\RPGClient.exe"
if (-not (Test-Path $exe)) {
    throw "Build finished but RPGClient.exe not found at: $exe"
}

Write-Host "Built: $exe"
