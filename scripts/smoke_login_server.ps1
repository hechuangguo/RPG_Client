<#
.SYNOPSIS
  LoginServer connectivity smoke test (TCP + client_config.xml).

.EXAMPLE
  .\scripts\smoke_login_server.ps1
#>
$ErrorActionPreference = "Stop"
$root = git rev-parse --show-toplevel
Set-Location $root

$configPath = Join-Path $root "config/client_config.xml"
if (-not (Test-Path $configPath)) {
    $configPath = Join-Path $root "assets/StreamingAssets/config/client_config.xml"
}

if (-not (Test-Path $configPath)) {
    throw "client_config.xml not found"
}

[xml]$cfg = Get-Content $configPath
$loginHost = $cfg.ClientConfig.loginHost
$loginPort = [int]$cfg.ClientConfig.loginPort
$tlsEnabled = $cfg.ClientConfig.Tls.enabled -eq "1"

Write-Host "LoginServer config: ${loginHost}:${loginPort} TLS=$tlsEnabled"

$tcp = Test-NetConnection -ComputerName $loginHost -Port $loginPort -WarningAction SilentlyContinue
if (-not $tcp.TcpTestSucceeded) {
    Write-Warning "TCP failed: cannot reach ${loginHost}:${loginPort}"
    Write-Warning "Start LoginServer and check firewall/routing, then retry."
    exit 1
}

Write-Host "TCP OK: ${loginHost}:${loginPort}"
Write-Host "Full protocol test: Unity Editor Play Boot scene, check logs/client_*.log"
exit 0
