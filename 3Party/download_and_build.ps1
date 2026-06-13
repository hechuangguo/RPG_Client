# Download/build third-party libraries for RPGClient
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$sfmlZip = Join-Path $root "SFML-2.6.1-win64.zip"
$sfmlDir = Join-Path $root "sfml"

if (-not (Test-Path (Join-Path $sfmlDir "lib\sfml-graphics.lib"))) {
    Write-Host "Downloading SFML 2.6.1..."
    Invoke-WebRequest -Uri "https://github.com/SFML/SFML/releases/download/2.6.1/SFML-2.6.1-windows-vc17-64-bit.zip" -OutFile $sfmlZip
    Expand-Archive -Path $sfmlZip -DestinationPath $root -Force
    if (Test-Path (Join-Path $root "SFML-2.6.1")) {
        if (Test-Path $sfmlDir) { Remove-Item $sfmlDir -Recurse -Force }
        Rename-Item (Join-Path $root "SFML-2.6.1") "sfml"
    }
}

$luaDir = Join-Path $root "lua"
if (-not (Test-Path (Join-Path $luaDir "lapi.c"))) {
    Write-Host "Cloning Lua 5.4.6..."
    git clone --depth 1 --branch v5.4.6 https://github.com/lua/lua.git $luaDir
}

Write-Host "3Party ready."
