Add-Type -AssemblyName System.Drawing
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$out = Join-Path $root "app.ico"
Write-Output "out: $out"

$sizes = @(16, 32, 48, 256)

function New-StarFrame([int]$size) {
  $bmp = New-Object System.Drawing.Bitmap($size, $size)
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
  $g.Clear([System.Drawing.Color]::FromArgb(255, 10, 16, 32))

  $cx = $size * 0.5; $cy = $size * 0.5
  $rOut = $size * 0.36; $rIn = $size * 0.15
  $pts = New-Object 'System.Drawing.PointF[]' 10
  for ($i = 0; $i -lt 10; ++$i) {
    $ang = -1.57079632679 + $i * 0.31415926536
    if ($i % 2 -eq 0) { $r = $rOut } else { $r = $rIn }
    $pts[$i] = New-Object System.Drawing.PointF(
      [single]($cx + $r * [Math]::Cos($ang)),
      [single]($cy + $r * [Math]::Sin($ang)))
  }
  $gold = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 255, 196, 77))
  $g.FillPolygon($gold, $pts)

  $blue = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 102, 217, 255))
  $pr = $size * 0.07
  $g.FillEllipse($blue, $cx + $rOut * 0.75 - $pr, $cy + $rOut * 0.55 - $pr, $pr * 2, $pr * 2)

  $g.Dispose()
  return $bmp
}

$frames = @()
foreach ($s in $sizes) {
  $bmp = New-StarFrame $s
  $lock = $bmp.LockBits(
    (New-Object System.Drawing.Rectangle(0, 0, $s, $s)),
    [System.Drawing.Imaging.ImageLockMode]::ReadOnly,
    [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
  $rowBytes = $s * 4
  $px = New-Object byte[] ($rowBytes * $s)
  for ($y = 0; $y -lt $s; ++$y) {
    $src = [IntPtr]::Add($lock.Scan0, $y * $lock.Stride)
    [System.Runtime.InteropServices.Marshal]::Copy($src, $px, $y * $rowBytes, $rowBytes)
  }
  $bmp.UnlockBits($lock)
  $bmp.Dispose()

  # bottom-up
  $flipped = New-Object byte[] $px.Length
  for ($y = 0; $y -lt $s; ++$y) {
    [Array]::Copy($px, $y * $rowBytes, $flipped, ($s - 1 - $y) * $rowBytes, $rowBytes)
  }
  $maskStride = [Math]::Ceiling([Math]::Ceiling($s / 8.0) / 4.0) * 4
  $mask = New-Object byte[] ([int]($maskStride * $s))

  $fms = New-Object System.IO.MemoryStream
  $fbw = New-Object System.IO.BinaryWriter($fms)
  $fbw.Write([uint32]40)
  $fbw.Write([int32]$s)
  $fbw.Write([int32]($s * 2))
  $fbw.Write([uint16]1)
  $fbw.Write([uint16]32)
  $fbw.Write([uint32]0)
  $fbw.Write([uint32]($flipped.Length + $mask.Length))
  $fbw.Write([int32]0); $fbw.Write([int32]0)
  $fbw.Write([uint32]0); $fbw.Write([uint32]0)
  $fbw.Write($flipped)
  $fbw.Write($mask)
  $fbw.Flush()
  $frames += ,$fms.ToArray()
  $fbw.Dispose(); $fms.Dispose()
}
Write-Output ("frames: " + (($frames | ForEach-Object { $_.Length }) -join ","))

$total = 6 + 16 * $sizes.Count
for ($i = 0; $i -lt $frames.Count; ++$i) { $total += $frames[$i].Length }
Write-Output "expected total: $total"

$ms = New-Object System.IO.MemoryStream
$bw = New-Object System.IO.BinaryWriter($ms)
$bw.Write([uint16]0); $bw.Write([uint16]1); $bw.Write([uint16]$sizes.Count)
$offset = 6 + 16 * $sizes.Count
for ($i = 0; $i -lt $sizes.Count; ++$i) {
  $s = $sizes[$i]
  if ($s -ge 256) { $wb = [byte]0 } else { $wb = [byte]$s }
  $bw.Write($wb)
  $bw.Write([byte]($s -band 0xFF))
  $bw.Write([byte]0); $bw.Write([byte]0)
  $bw.Write([uint16]1); $bw.Write([uint16]32)
  $bw.Write([uint32]$frames[$i].Length)
  $bw.Write([uint32]$offset)
  $offset += $frames[$i].Length
}
Write-Output ("after entries, ms len: " + $ms.Length)
foreach ($f in $frames) { $bw.Write($f) }
Write-Output ("after frames, ms len: " + $ms.Length)
$bw.Flush()
Write-Output ("after flush, ms len: " + $ms.Length)

$data = $ms.ToArray()
Write-Output ("data bytes: " + $data.Length)
[System.IO.File]::WriteAllBytes($out, $data)
$bw.Dispose(); $ms.Dispose()
Write-Output ("file on disk: " + (Get-Item $out).Length)
if ($total -ne (Get-Item $out).Length) { throw "length mismatch" }
Write-Output "OK"