# Download Noto Sans SC Regular for RPGClient UI (SIL OFL 1.1)
$ErrorActionPreference = "Stop"

$fontDir = $PSScriptRoot
$fontFile = Join-Path $fontDir "NotoSansSC-Regular.otf"
$fontUrl = "https://raw.githubusercontent.com/notofonts/noto-cjk/main/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf"

if (Test-Path $fontFile) {
    $size = (Get-Item $fontFile).Length
    if ($size -gt 1000000) {
        Write-Host "Font already present: $fontFile"
        exit 0
    }
    Remove-Item $fontFile -Force
}

Write-Host "Downloading NotoSansSC-Regular.otf..."
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Invoke-WebRequest -Uri $fontUrl -OutFile $fontFile -UseBasicParsing

if (-not (Test-Path $fontFile)) {
    throw "Font download failed: $fontFile"
}

Write-Host "Font ready: $fontFile"
