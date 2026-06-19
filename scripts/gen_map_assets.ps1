# Generate map/1002 tileset, water tile, and building PNGs.
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$root = Split-Path -Parent $PSScriptRoot
$mapDir = Join-Path $root "map\1002"
$bldDir = Join-Path $mapDir "buildings"
New-Item -ItemType Directory -Force -Path $bldDir | Out-Null

function Save-Png($bitmap, $path) {
    $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bitmap.Dispose()
    Write-Host "Wrote $path"
}

function Fill-Rect($g, $brush, $x, $y, $w, $h) {
    $g.FillRectangle($brush, $x, $y, $w, $h)
}

# --- tileset 256x32: bluestone, bluestone_crack, grass, grass_flower ---
$tileW = 64; $tileH = 32
$tileset = New-Object System.Drawing.Bitmap ($tileW * 4), $tileH
$g = [System.Drawing.Graphics]::FromImage($tileset)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None

function Draw-StoneTile($g, $ox, $base, $dark, $crack) {
    $b = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb($base))
    Fill-Rect $g $b $ox 0 $tileW $tileH
    $pen = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb($dark), 1)
    for ($x = 0; $x -lt $tileW; $x += 16) {
        $g.DrawLine($pen, $ox + $x, 0, $ox + $x, $tileH)
    }
    for ($y = 0; $y -lt $tileH; $y += 8) {
        $g.DrawLine($pen, $ox, $y, $ox + $tileW, $y)
    }
    if ($crack) {
        $g.DrawLine($pen, $ox + 20, 4, $ox + 44, 28)
        $g.DrawLine($pen, $ox + 10, 20, $ox + 30, 8)
    }
    $pen.Dispose(); $b.Dispose()
}

function Draw-GrassTile($g, $ox, $flowers) {
    $b = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(55, 115, 55))
    Fill-Rect $g $b $ox 0 $tileW $tileH
    $b2 = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(40, 95, 42))
    for ($i = 0; $i -lt 12; $i++) {
        $gx = $ox + (($i * 17) % 56) + 4
        $gy = (($i * 7) % 22) + 4
        Fill-Rect $g $b2 $gx $gy 3 6
    }
    if ($flowers) {
        $f = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(220, 180, 80))
        Fill-Rect $g $f ($ox + 48) 10 4 4
        Fill-Rect $g $f ($ox + 12) 18 4 4
    }
    $b.Dispose(); $b2.Dispose()
    if ($flowers) { $f.Dispose() }
}

Draw-StoneTile $g 0 100 70 80 $false
Draw-StoneTile $g 64 95 65 75 $true
Draw-GrassTile $g 128 $false
Draw-GrassTile $g 192 $true
$g.Dispose()
Save-Png $tileset (Join-Path $mapDir "tileset.png")

# --- water 64x32 ---
$water = New-Object System.Drawing.Bitmap $tileW, $tileH
$g = [System.Drawing.Graphics]::FromImage($water)
$b = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(35, 110, 130))
Fill-Rect $g $b 0 0 $tileW $tileH
$pen = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(60, 150, 170), 1)
for ($x = 0; $x -lt $tileW; $x += 8) {
    $g.DrawLine($pen, $x, 8, $x + 16, 24)
}
$pen.Dispose(); $b.Dispose(); $g.Dispose()
Save-Png $water (Join-Path $mapDir "water_tile.png")

# --- buildings 128x96 ---
$ids = @("shop", "auction", "pawn", "martial", "inn", "plaza")
$colors = @(
    @(120, 85, 60), @(105, 80, 65), @(95, 70, 55),
    @(110, 75, 70), @(115, 88, 75), @(130, 100, 80)
)
for ($i = 0; $i -lt $ids.Count; $i++) {
    $bmp = New-Object System.Drawing.Bitmap 128, 96
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $c = $colors[$i]
    $body = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb($c[0], $c[1], $c[2]))
    $roof = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb($c[0] + 25, $c[1] + 15, $c[2] + 10))
    $trim = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(180, 150, 90))
    Fill-Rect $g $body 20 40 88 52
    $pts = @(
        [System.Drawing.Point]::new(12, 42),
        [System.Drawing.Point]::new(64, 8),
        [System.Drawing.Point]::new(116, 42)
    )
    $g.FillPolygon($roof, $pts)
    Fill-Rect $g $trim 44 58 40 28
    $g.Dispose()
    $body.Dispose(); $roof.Dispose(); $trim.Dispose()
    Save-Png $bmp (Join-Path $bldDir "$($ids[$i]).png")
}

Write-Host "Map assets generated under $mapDir"
