<#
.SYNOPSIS
Tests the Doppler beaming asymmetry of SgrA* disk.

.DESCRIPTION
Analytical Justification for Expected Doppler Side:
1. In `src/SceneManager.cpp`, `teleportTo("sgra")` spawns the camera at `(0, 0, spawnDistanceRs)` with `yaw=0` and `pitch=0`.
2. Based on `src/Camera.cpp`, standard forward is `(0, 0, -1)`. Thus, the camera is on the +Z axis looking at the origin.
3. The right side of the screen corresponds to +X, and the left side to -X.
4. In `shaders/bh.frag`, `vDir = vec3(-sin(phiDisk), 0.0, cos(phiDisk))` where `phiDisk = atan(z, x)`.
5. For +X (right side of screen), `phi = 0`, so `vDir = (0, 0, 1)`. This vector points in the +Z direction, which is TOWARDS the camera.
6. Therefore, the right side of the disk is approaching the observer and should be blueshifted/brighter due to Doppler beaming. The left side (-X) has `vDir = (0, 0, -1)`, which is moving away (redshifted/dimmer).

We check the average luminance of the left vs right side.
#>

Add-Type -AssemblyName System.Drawing

$imagePath = "docs/screens/disk/sgra.png"
if (-not (Test-Path $imagePath)) {
    Write-Host "File not found: $imagePath"
    exit 1
}

$bitmap = [System.Drawing.Bitmap]::FromFile($imagePath)
if ($bitmap.Width -ne 1280 -or $bitmap.Height -ne 720) {
    Write-Host "Unexpected image dimensions: $($bitmap.Width)x$($bitmap.Height)"
    $bitmap.Dispose()
    exit 1
}

$leftSum = 0.0
$rightSum = 0.0
$pixelCount = 0

# Left: 15-45% X, Right: 55-85% X, Y: 45-55%
$yStart = [int](0.45 * 720)
$yEnd = [int](0.55 * 720)
$leftXStart = [int](0.15 * 1280)
$leftXEnd = [int](0.45 * 1280)
$rightXStart = [int](0.55 * 1280)
$rightXEnd = [int](0.85 * 1280)

for ($y = $yStart; $y -lt $yEnd; $y++) {
    for ($x = 0; $x -lt ($leftXEnd - $leftXStart); $x++) {
        $leftPixel = $bitmap.GetPixel($leftXStart + $x, $y)
        $rightPixel = $bitmap.GetPixel($rightXStart + $x, $y)

        $leftSum += 0.2126 * $leftPixel.R + 0.7152 * $leftPixel.G + 0.0722 * $leftPixel.B
        $rightSum += 0.2126 * $rightPixel.R + 0.7152 * $rightPixel.G + 0.0722 * $rightPixel.B
        $pixelCount++
    }
}

$bitmap.Dispose()

$leftAvg = $leftSum / $pixelCount
$rightAvg = $rightSum / $pixelCount

Write-Host "Left average luminance: $leftAvg"
Write-Host "Right average luminance: $rightAvg"

# Expected: Right side brighter
if ($rightAvg -gt ($leftAvg * 1.3)) {
    $ratio = $rightAvg / $leftAvg
    Write-Host "Ratio: $ratio"
    Write-Host "PASS: Right side is brighter as expected."
    exit 0
} elseif ($leftAvg -gt ($rightAvg * 1.3)) {
    $ratio = $leftAvg / $rightAvg
    Write-Host "Ratio: $ratio"
    Write-Host "FAIL: Doppler side MIRRORED: expected Right > Left, got Left > Right"
    exit 1
} else {
    Write-Host "FAIL: Insufficient Doppler asymmetry (Ratio < 1.3)"
    exit 1
}
