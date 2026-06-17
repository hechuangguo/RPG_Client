$ErrorActionPreference = "Stop"

$repoRoot = git rev-parse --show-toplevel 2>$null
if (-not $repoRoot) {
    $repoRoot = Split-Path -Parent $PSScriptRoot
}

Set-Location $repoRoot

$example = Join-Path $repoRoot "config/client_config.xml.example"
$local = Join-Path $repoRoot "config/client_config.xml"

if (-not (Test-Path $example)) {
    throw "Missing template: $example"
}

if (Test-Path $local) {
    Write-Host "Local config already exists: $local"
    exit 0
}

Copy-Item $example $local
Write-Host "Created local config from template: $local"
Write-Host "Edit loginHost and loginPort for your LoginServer."
