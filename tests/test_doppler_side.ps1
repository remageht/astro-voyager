<#
.SYNOPSIS
Tests the Doppler beaming asymmetry of the SgrA* accretion disk.

.DESCRIPTION
Analytical justification for the expected Doppler side:
1. In `src/SceneManager.cpp`, `teleportTo("sgra")` spawns the camera at `(0, 0, spawnDistanceRs)` with `yaw=0` and `pitch=0`.
2. Based on `src/Camera.cpp`, standard forward is `(0, 0, -1)`. Thus, the camera sits on the +Z axis looking at the origin.
3. The right side of the screen corresponds to +X, the left side to -X.
4. In `shaders/bh.frag`, `vDir = vec3(-sin(phiDisk), 0.0, cos(phiDisk))` with `phiDisk = atan(z, x)`.
5. For +X (right side of the screen) `phi = 0`, so `vDir = (0, 0, 1)`: the material there moves in +Z, i.e. TOWARDS the camera.
6. Hence the right half of the disk approaches the observer and must be brighter (Doppler beaming), while the left half recedes and must be dimmer.

Why the measurement subtracts the sky:
The measurement window (X: 15-45% and 55-85%, Y: 45-95%) also contains the
left-bright procedural sky (in the disk-off frame it is roughly L 43.6 / R 36.0).
A raw on-frame comparison is therefore dominated by the sky and cancels the disk
signal. We measure the DISK CONTRIBUTION instead: the same deterministic frame is
rendered twice, with `--screenshot-all --disk` (docs/screens/disk/) and with
`--screenshot-all` (docs/screens/), and the per-side luminance difference on-off
is compared.

PASS criterion: deltaR >= 1.3 * deltaL (right/approaching side beamed)
                AND (deltaL + deltaR) >= 3.0 (the disk is actually visible).
Exit code 0 = PASS, 1 = FAIL.
#>

Add-Type -AssemblyName System.Drawing

function Get-Luminance {
    param(
        [System.Drawing.Bitmap]$Bitmap,
        [int]$XStart,
        [int]$XEnd,
        [int]$YStart,
        [int]$YEnd
    )
    $sum = 0.0
    $count = 0
    for ($y = $YStart; $y -lt $YEnd; $y++) {
        for ($x = $XStart; $x -lt $XEnd; $x++) {
            $pixel = $Bitmap.GetPixel($x, $y)
            $sum += 0.2126 * $pixel.R + 0.7152 * $pixel.G + 0.0722 * $pixel.B
            $count++
        }
    }
    if ($count -eq 0) { return 0.0 }
    return $sum / $count
}

$diskOnPath = "docs/screens/disk/sgra.png"
$diskOffPath = "docs/screens/sgra.png"

if (-not (Test-Path $diskOnPath)) {
    Write-Host "File not found: $diskOnPath"
    Write-Host "Run first: .\build-gl\Release\astro-voyager.exe --screenshot-all --disk"
    exit 1
}
if (-not (Test-Path $diskOffPath)) {
    Write-Host "File not found: $diskOffPath"
    Write-Host "Run first: .\build-gl\Release\astro-voyager.exe --screenshot-all"
    exit 1
}

$onBitmap = [System.Drawing.Bitmap]::FromFile((Resolve-Path $diskOnPath))
$offBitmap = [System.Drawing.Bitmap]::FromFile((Resolve-Path $diskOffPath))

if ($onBitmap.Width -ne 1280 -or $onBitmap.Height -ne 720) {
    Write-Host "Unexpected disk-on dimensions: $($onBitmap.Width)x$($onBitmap.Height)"
    $onBitmap.Dispose()
    $offBitmap.Dispose()
    exit 1
}
if ($offBitmap.Width -ne 1280 -or $offBitmap.Height -ne 720) {
    Write-Host "Unexpected disk-off dimensions: $($offBitmap.Width)x$($offBitmap.Height)"
    $onBitmap.Dispose()
    $offBitmap.Dispose()
    exit 1
}

# Left: X 15-45%, Right: X 55-85%, Y: 45-95% (the disk band, sky subtracted).
$yStart = [int](0.45 * 720)
$yEnd = [int](0.95 * 720)
$leftXStart = [int](0.15 * 1280)
$leftXEnd = [int](0.45 * 1280)
$rightXStart = [int](0.55 * 1280)
$rightXEnd = [int](0.85 * 1280)

$onL = Get-Luminance -Bitmap $onBitmap -XStart $leftXStart -XEnd $leftXEnd -YStart $yStart -YEnd $yEnd
$onR = Get-Luminance -Bitmap $onBitmap -XStart $rightXStart -XEnd $rightXEnd -YStart $yStart -YEnd $yEnd
$offL = Get-Luminance -Bitmap $offBitmap -XStart $leftXStart -XEnd $leftXEnd -YStart $yStart -YEnd $yEnd
$offR = Get-Luminance -Bitmap $offBitmap -XStart $rightXStart -XEnd $rightXEnd -YStart $yStart -YEnd $yEnd

$onBitmap.Dispose()
$offBitmap.Dispose()

# Disk contribution = disk-on minus disk-off, per side.
$deltaL = $onL - $offL
$deltaR = $onR - $offR
$totalDelta = $deltaL + $deltaR

Write-Host ("disk-on  left:  {0:F2}" -f $onL)
Write-Host ("disk-on  right: {0:F2}" -f $onR)
Write-Host ("disk-off left:  {0:F2}" -f $offL)
Write-Host ("disk-off right: {0:F2}" -f $offR)
Write-Host ("disk contribution left:  {0:F2}" -f $deltaL)
Write-Host ("disk contribution right: {0:F2}" -f $deltaR)

if ($deltaL -le 0.0) {
    Write-Host "FAIL: no measurable disk contribution on the left side"
    exit 1
}

$ratio = $deltaR / $deltaL
Write-Host ("ratio (right/left): {0:F3}" -f $ratio)

if ($deltaL -ge 1.3 * $deltaR) {
    Write-Host "FAIL: Doppler side MIRRORED: expected Right > Left, got Left > Right"
    exit 1
}
if ($ratio -lt 1.3) {
    Write-Host "FAIL: insufficient Doppler asymmetry (ratio < 1.3)"
    exit 1
}
if ($totalDelta -lt 3.0) {
    Write-Host ("FAIL: insufficient disk signal (deltaL + deltaR = {0:F2} < 3.0)" -f $totalDelta)
    exit 1
}

Write-Host "PASS: disk contribution is brighter on the right (approaching) side."
exit 0
