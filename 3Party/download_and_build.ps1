# Download/build third-party libraries for RPGClient
$ErrorActionPreference = "Stop"
$thirdPartyRoot = $PSScriptRoot
$sfmlZip = Join-Path $thirdPartyRoot "SFML-2.6.1-win64.zip"
$sfmlDir = Join-Path $thirdPartyRoot "sfml"

if (-not (Test-Path (Join-Path $sfmlDir "lib\sfml-graphics.lib"))) {
    Write-Host "Downloading SFML 2.6.1..."
    Invoke-WebRequest -Uri "https://github.com/SFML/SFML/releases/download/2.6.1/SFML-2.6.1-windows-vc17-64-bit.zip" -OutFile $sfmlZip
    Expand-Archive -Path $sfmlZip -DestinationPath $thirdPartyRoot -Force
    if (Test-Path (Join-Path $thirdPartyRoot "SFML-2.6.1")) {
        if (Test-Path $sfmlDir) { Remove-Item $sfmlDir -Recurse -Force }
        Rename-Item (Join-Path $thirdPartyRoot "SFML-2.6.1") "sfml"
    }
}

$luaDir = Join-Path $thirdPartyRoot "lua"
if (-not (Test-Path (Join-Path $luaDir "lapi.c"))) {
    Write-Host "Cloning Lua 5.4.6..."
    git clone --depth 1 --branch v5.4.6 https://github.com/lua/lua.git $luaDir
}

# OpenSSL (FireDaemon prebuilt via winget, copied into 3Party/openssl)
$opensslDir = Join-Path $thirdPartyRoot "openssl"
$opensslMarker = Join-Path $opensslDir "lib\libssl.lib"
if (-not (Test-Path $opensslMarker)) {
    $fdInstallCandidates = @(
        (Join-Path ${env:ProgramFiles} "FireDaemon OpenSSL 4"),
        (Join-Path ${env:ProgramFiles} "FireDaemon OpenSSL 3")
    )
    $fdRoot = $fdInstallCandidates | Where-Object { Test-Path (Join-Path $_ "lib\libssl.lib") } | Select-Object -First 1
    if (-not $fdRoot) {
        Write-Host "FireDaemon OpenSSL not found; installing via winget..."
        winget install FireDaemon.OpenSSL --source winget --silent --accept-package-agreements --accept-source-agreements
        $fdRoot = $fdInstallCandidates | Where-Object { Test-Path (Join-Path $_ "lib\libssl.lib") } | Select-Object -First 1
    }
    if (-not $fdRoot) {
        throw "OpenSSL install failed. Run: winget install FireDaemon.OpenSSL --source winget"
    }
    if (Test-Path $opensslDir) { Remove-Item $opensslDir -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $opensslDir | Out-Null
    Copy-Item -Path (Join-Path $fdRoot "include") -Destination (Join-Path $opensslDir "include") -Recurse -Force
    Copy-Item -Path (Join-Path $fdRoot "lib") -Destination (Join-Path $opensslDir "lib") -Recurse -Force
    Copy-Item -Path (Join-Path $fdRoot "bin") -Destination (Join-Path $opensslDir "bin") -Recurse -Force
    Write-Host "OpenSSL copied from $fdRoot to $opensslDir"
}

Write-Host "3Party ready."
