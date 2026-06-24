# Sync Unity.gitignore from github/gitignore and merge RPG_Client project rules.
# Usage: .\scripts\install_gitignore_unity.ps1

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$gitignorePath = Join-Path $repoRoot ".gitignore"
$fragmentPath = Join-Path $PSScriptRoot "gitignore.project.fragment"
$unityUrl = "https://raw.githubusercontent.com/github/gitignore/main/Unity.gitignore"

Write-Host "Downloading Unity.gitignore from $unityUrl"
$unityTemplate = (Invoke-WebRequest -Uri $unityUrl -UseBasicParsing).Content.TrimEnd()
$projectRules = [System.IO.File]::ReadAllText($fragmentPath)

$merged = $unityTemplate + $projectRules
[System.IO.File]::WriteAllText($gitignorePath, $merged, [System.Text.UTF8Encoding]::new($false))
Write-Host "Wrote $gitignorePath"
