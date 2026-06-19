# Generate assets/characters/player/*.png and anim.json
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$root = Split-Path -Parent $PSScriptRoot
$outDir = Join-Path $root "assets\characters\player"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

$frameW = 48; $frameH = 72; $frames = 4; $dirs = 4
$sheetW = $frameW * $frames; $sheetH = $frameH * $dirs

function Draw-CharacterFrame($g, $ox, $oy, $frame, $robe, $robeD, $skin, $hair, $female) {
    $leg = if ($frame % 2 -eq 0) { 0 } else { 2 }
    $bRobe = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb($robe))
    $bDark = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb($robeD))
    $bSkin = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb($skin))
    $bHair = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb($hair))
    $bSash = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(200, 170, 80))
    $bSword = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(210, 220, 235))

    $g.FillRectangle($bHair, $ox + 16, $oy + 4, 16, 12)
    $g.FillRectangle($bSkin, $ox + 17, $oy + 8, 14, 10)
    $g.FillRectangle($bRobe, $ox + 12, $oy + 18, 24, 28)
    $g.FillRectangle($bDark, $ox + 8, $oy + 20, 6, 22)
    $g.FillRectangle($bDark, $ox + 34, $oy + 20, 6, 22)
    $g.FillRectangle($bSash, $ox + 14, $oy + 38, 20, 5)
    $g.FillRectangle($bDark, $ox + 14 + $leg, $oy + 46, 8, 18)
    $g.FillRectangle($bDark, $ox + 26 - $leg, $oy + 46, 8, 18)
    $g.FillRectangle($bSword, $ox + 36, $oy + 22, 4, 28)
    if ($female) {
        $g.FillRectangle($bRobe, $ox + 6, $oy + 26, 5, 16)
    }
    $bRobe.Dispose(); $bDark.Dispose(); $bSkin.Dispose()
    $bHair.Dispose(); $bSash.Dispose(); $bSword.Dispose()
}

$variants = @(
    @{ name = "warrior_male";   robe = 70;  robeD = 50;  skin = 245; hair = 30;  female = $false },
    @{ name = "warrior_female"; robe = 75;  robeD = 55;  skin = 255; hair = 50;  female = $true },
    @{ name = "mage_male";      robe = 80;  robeD = 55;  skin = 245; hair = 30;  female = $false },
    @{ name = "mage_female";    robe = 85;  robeD = 60;  skin = 255; hair = 50;  female = $true }
)

foreach ($v in $variants) {
    $bmp = New-Object System.Drawing.Bitmap $sheetW, $sheetH
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.Clear([System.Drawing.Color]::Transparent)
    for ($d = 0; $d -lt $dirs; $d++) {
        for ($f = 0; $f -lt $frames; $f++) {
            $ox = $f * $frameW
            $oy = $d * $frameH
            $robe = if ($v.name -like "mage*") { 80 + $d * 2 } else { $v.robe }
            $robeD = if ($v.name -like "mage*") { 55 + $d } else { $v.robeD }
            Draw-CharacterFrame $g $ox $oy ($f + $d) $robe $robeD $v.skin $v.hair $v.female
        }
    }
    $g.Dispose()
    $path = Join-Path $outDir "$($v.name).png"
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
    Write-Host "Wrote $path"
}

$anim = @"
{
  "frameWidth": 48,
  "frameHeight": 72,
  "framesPerDir": 4,
  "directions": 4,
  "fps": 8
}
"@
$anim | Set-Content -Path (Join-Path $outDir "anim.json") -Encoding UTF8
Write-Host "Wrote anim.json"
