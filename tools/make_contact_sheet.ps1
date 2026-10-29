# Compose a PNG contact sheet from a directory of rendered frames.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File tools/make_contact_sheet.ps1 `
#       -InputDir blender/out/turntable_frames `
#       -Out docs/screens/ring_ship_turntable.png `
#       -Cols 4 -CellWidth 640 -CellHeight 360
param(
    [Parameter(Mandatory = $true)][string]$InputDir,
    [Parameter(Mandatory = $true)][string]$Out,
    [int]$Cols = 4,
    [int]$CellWidth = 640,
    [int]$CellHeight = 360,
    [int]$Gap = 6,
    [string]$Background = '#080b12'
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$resolved = (Resolve-Path -Path $InputDir).Path
$files = @(Get-ChildItem -Path $resolved -Filter '*.png' -File | Sort-Object Name)
if ($files.Count -eq 0) {
    throw "No PNG frames found in $resolved"
}

$rows = [math]::Ceiling($files.Count / [double]$Cols)
$sheetW = $Cols * $CellWidth + ($Cols - 1) * $Gap
$sheetH = $rows * $CellHeight + ($rows - 1) * $Gap

$sheet = New-Object System.Drawing.Bitmap $sheetW, $sheetH
$g = [System.Drawing.Graphics]::FromImage($sheet)
$g.Clear([System.Drawing.ColorTranslator]::FromHtml($Background))
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality

for ($i = 0; $i -lt $files.Count; $i++) {
    $col = $i % $Cols
    $row = [math]::Floor($i / $Cols)
    $x = $col * ($CellWidth + $Gap)
    $y = $row * ($CellHeight + $Gap)

    $img = [System.Drawing.Image]::FromFile($files[$i].FullName)
    try {
        $scale = [math]::Min($CellWidth / [double]$img.Width,
                             $CellHeight / [double]$img.Height)
        $drawW = [int][math]::Round($img.Width * $scale)
        $drawH = [int][math]::Round($img.Height * $scale)
        $drawX = $x + [int][math]::Floor(($CellWidth - $drawW) / 2)
        $drawY = $y + [int][math]::Floor(($CellHeight - $drawH) / 2)
        $g.DrawImage($img, $drawX, $drawY, $drawW, $drawH)
    }
    finally {
        $img.Dispose()
    }
}

$g.Dispose()
$outDir = Split-Path -Parent $Out
if ($outDir -and -not (Test-Path $outDir)) {
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
}
$outPath = if ([System.IO.Path]::IsPathRooted($Out)) {
    $Out
} else {
    Join-Path (Get-Location) $Out
}
$sheet.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
$sheet.Dispose()
Write-Host ("Contact sheet: {0} ({1}x{2}, {3} frames)" -f $Out, $sheetW, $sheetH, $files.Count)
