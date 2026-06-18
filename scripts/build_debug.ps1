# Build RPGClient.exe (Windows x64, MSVC + VS generator, Debug)
$ErrorActionPreference = "Stop"

$clientDir = Split-Path $PSScriptRoot -Parent
$buildDir = Join-Path $clientDir "out\build\x64-Debug"
$cacheFile = Join-Path $buildDir "CMakeCache.txt"
$projectMarker = Join-Path $buildDir "RPGClient.vcxproj"

$repoRoot = git -C $clientDir rev-parse --show-toplevel 2>$null
if ($repoRoot -and -not (Test-Path (Join-Path $clientDir "Common\LoginMsg.h"))) {
    Write-Host "Common submodule missing; initializing ..."
    git -C $repoRoot submodule update --init Common
    if (-not (Test-Path (Join-Path $clientDir "Common\LoginMsg.h"))) {
        throw "Common/LoginMsg.h not found. Run: git submodule update --init --recursive"
    }
}

if (-not (Test-Path (Join-Path $clientDir "3Party\sfml\lib\sfml-graphics.lib"))) {
    Write-Host "SFML not found; running 3Party\download_and_build.ps1 ..."
    & (Join-Path $clientDir "3Party\download_and_build.ps1")
    if (-not (Test-Path (Join-Path $clientDir "3Party\sfml\lib\sfml-graphics.lib"))) {
        throw "SFML still missing after download. Run: .\3Party\download_and_build.ps1"
    }
}

$fontScript = Join-Path $clientDir "assets\fonts\fetch_font.ps1"
if (Test-Path $fontScript) {
    & $fontScript
}

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere not found. Install Visual Studio with C++ desktop development."
}

$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) {
    throw "MSVC C++ tools not found. Install 'Desktop development with C++' workload in Visual Studio Installer."
}

$cmakeCandidates = @(
    (Join-Path $vsPath "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"),
    "C:\Program Files\CMake\bin\cmake.exe"
)
$cmake = $cmakeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $cmake) {
    throw "cmake.exe not found. Install CMake or VS CMake component."
}

$needsConfigure = -not ((Test-Path $cacheFile) -and (Test-Path $projectMarker))
if ($needsConfigure) {
    if (Test-Path $buildDir) {
        Write-Host "Removing incomplete CMake cache ..."
        Remove-Item -Recurse -Force $buildDir
    }
    Write-Host "Configuring x64-debug preset ..."
    & $cmake --preset x64-debug
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

Write-Host "Building Debug ..."
& $cmake --build $buildDir --config Debug
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$exe = Join-Path $buildDir "bin\RPGClient.exe"
if (-not (Test-Path $exe)) {
    throw "Build finished but RPGClient.exe not found at: $exe"
}

Write-Host "Built: $exe"
exit 0
