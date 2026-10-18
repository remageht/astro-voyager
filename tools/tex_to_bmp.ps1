<#
  tools/tex_to_bmp.ps1 - shrink an embedded FBX texture clip to a 24-bit BMP
  that src/ObjModel.cpp can read without any image-decoding dependency.

  Loaded by tools/fbx_ship_export.mjs; uses System.Drawing, so it only runs on
  Windows (the exporter drops a warning and skips textures elsewhere).

  Usage:
    pwsh -NoProfile -File tools/tex_to_bmp.ps1 -Source color.jpg -Dest out.bmp -Size 1024
#>
param(
  [Parameter(Mandatory = $true)][string]$Source,
  [Parameter(Mandatory = $true)][string]$Dest,
  [int]$Size = 1024
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing | Out-Null

$src = [System.Drawing.Image]::FromFile((Resolve-Path -LiteralPath $Source).Path)
try {
  $bmp = New-Object System.Drawing.Bitmap $Size, $Size, ([System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
  try {
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    try {
      $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
      $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
      $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
      $g.DrawImage($src, 0, 0, $Size, $Size)
    } finally { $g.Dispose() }
    $bmp.Save($Dest, [System.Drawing.Imaging.ImageFormat]::Bmp)
  } finally { $bmp.Dispose() }
} finally { $src.Dispose() }

$len = (Get-Item -LiteralPath $Dest).Length
Write-Host ("tex_to_bmp: {0} -> {1} ({2}x{2}, {3:N0} KB)" -f (Split-Path $Source -Leaf), (Split-Path $Dest -Leaf), $Size, ($len / 1KB))
