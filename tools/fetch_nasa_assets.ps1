# Fetch the public-domain NASA Science assets used by the astro-voyager
# showcases.  Nothing here is committed: GLB/EXR files are local, heavy
# sources, exactly like the OBJ/BMP model atlases.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File tools/fetch_nasa_assets.ps1
#   powershell -ExecutionPolicy Bypass -File tools/fetch_nasa_assets.ps1 -Force
#   powershell -ExecutionPolicy Bypass -File tools/fetch_nasa_assets.ps1 -SkipStarMap
#
# Sources:
#   NASA 3D Resources (public domain, NASA media usage guidelines)
#   NASA/Goddard Space Flight Center Scientific Visualization Studio,
#   "Deep Star Maps 2020" (Gaia DR2: ESA/Gaia/DPAC).
param(
    [switch]$Force,
    [switch]$SkipStarMap
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$srcDir = Join-Path $root 'blender\out\nasa_src'
$texDir = Join-Path $root 'res\textures'
New-Item -ItemType Directory -Force -Path $srcDir | Out-Null
New-Item -ItemType Directory -Force -Path $texDir | Out-Null

$assets = @(
    @{
        Name = 'voyager_b.glb'
        Url = 'https://raw.githubusercontent.com/nasa/NASA-3D-Resources/master/3D%20Models/Voyager%20Probe%20(B)/Voyager%20Probe%20(B).glb'
        Sha256 = 'BD86DED828DD3F459293AEE4FFC3CD0998D8DB67439317C8299650A1174C3289'
        Size = 1720864
        Dest = $srcDir
        Credit = 'NASA 3D Resources - Voyager Probe (B)'
    },
    @{
        Name = 'hubble_a.glb'
        Url = 'https://raw.githubusercontent.com/nasa/NASA-3D-Resources/master/3D%20Models/Hubble%20Space%20Telescope%20(A)/Hubble%20Space%20Telescope%20(A).glb'
        Sha256 = 'E5BA4DE15C7D359AC8FA1AB7E286AFF42DEC09C0FADAE3DB99252587F39FA384'
        Size = 1694988
        Dest = $srcDir
        Credit = 'NASA 3D Resources - Hubble Space Telescope (A)'
    },
    @{
        Name = 'parker.glb'
        Url = 'https://raw.githubusercontent.com/nasa/NASA-3D-Resources/master/3D%20Models/Parker%20Solar%20Probe/Parker%20Solar%20Probe.glb'
        Sha256 = '4D26E84EFD29A78C5D07E474F6272BB111668B58DA5A7555E305A65C72DDB1C5'
        Size = 443572
        Dest = $srcDir
        Credit = 'NASA 3D Resources - Parker Solar Probe'
    }
)

if (-not $SkipStarMap) {
    $assets += @{
        Name = 'starmap_2020_4k_gal.exr'
        Url = 'https://svs.gsfc.nasa.gov/vis/a000000/a004800/a004851/starmap_2020_4k_gal.exr'
        Sha256 = 'BCE1D2C364720FD13EBBFAF1D444404A96B7CB4D7D8DDC88D44447358552361A'
        Size = 40550793
        Dest = $texDir
        Credit = 'NASA/Goddard SVS - Deep Star Maps 2020 (galactic)'
    }
}

function Get-Sha256([string]$Path) {
    return (Get-FileHash -Path $Path -Algorithm SHA256).Hash.ToUpperInvariant()
}

$downloaded = 0
$skipped = 0
foreach ($a in $assets) {
    $dest = Join-Path $a.Dest $a.Name
    if ((Test-Path $dest) -and -not $Force) {
        $hash = Get-Sha256 $dest
        if ($hash -eq $a.Sha256) {
            Write-Host ("[skip] {0} ({1:N0} bytes)" -f $a.Name, $a.Size)
            $skipped++
            continue
        }
        throw "Local file $dest has an unexpected SHA256. Re-run with -Force to replace it."
    }

    $tmp = "$dest.part"
    if (Test-Path $tmp) { Remove-Item -Force $tmp }
    Write-Host ("[get ] {0} <- {1}" -f $a.Name, $a.Url)
    Invoke-WebRequest -Uri $a.Url -OutFile $tmp -UseBasicParsing `
        -Headers @{ 'User-Agent' = 'astro-voyager' }

    $hash = Get-Sha256 $tmp
    if ($hash -ne $a.Sha256) {
        Remove-Item -Force $tmp
        throw "SHA256 mismatch for $($a.Name): got $hash, expected $($a.Sha256)"
    }
    Move-Item -Force $tmp $dest
    $downloaded++
}

Write-Host ""
Write-Host ("NASA assets ready: {0} downloaded, {1} already present." -f $downloaded, $skipped)
Write-Host "  Models:  $srcDir"
if (-not $SkipStarMap) {
    Write-Host "  Starmap: $(Join-Path $texDir 'starmap_2020_4k_gal.exr')"
}
Write-Host ""
Write-Host "Credits:"
Write-Host "  NASA 3D Resources (public domain) - https://science.nasa.gov/3d-resources/"
Write-Host "  NASA/Goddard SVS, Deep Star Maps 2020 - https://svs.gsfc.nasa.gov/4851/"
Write-Host ""
Write-Host "Render showcases:"
Write-Host "  blender -b -P blender/scripts/render_nasa_showcase.py -- --all"
