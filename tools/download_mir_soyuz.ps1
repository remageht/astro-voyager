param(
  [switch]$Force
)

$ErrorActionPreference = 'Stop'
$baseUri = 'https://raw.githubusercontent.com/Utkarsh575/Smart-India-Hackathon-2022/main/public/mir'
$root = if ($PSScriptRoot) { Split-Path -Parent $PSScriptRoot } else { '.' }
$outDir = Join-Path $root 'blender/out/mir_src'
$texDir = Join-Path $outDir 'textures'
New-Item -ItemType Directory -Path $texDir -Force | Out-Null

$expectedHashes = @{
  'scene.gltf'                                     = 'B8D0F6A482FFA4A28DCEE2360D5AB11ADBD481451D3F39EEB10F65CEDCD49B7C'
  'scene.bin'                                      = '61F4B26AC7514204E1AD5ACE0CC1DB72F0E2515B611ED512FA5F0F6549F85C2C'
  'Mir_Antenna_baseColor.png'                      = '5D2BABDAF615C17055A3D8B2C783F19E89380A9254D09CA0C1D765681E4EE78A'
  'Mir_Antenna_metallicRoughness.png'              = '7BAD39AE2DD939C116B5B6E19DACC8CE5B96CBF114C0B39CC9CCC2D482E0CC32'
  'Mir_White_Isolation_baseColor.png'              = 'C4CA5C5B0D6F42DA9B7424E37835933A8B80493EB541F98AF71081BA02F9EA88'
  'Progress_black_Isolation_metallicRoughness.png' = 'AF5DE87A18935C46D9007AA71E0F3ED24E6A43E16C9D04E88BA5686733CEC41D'
  'Progress_black_Isolation_normal.png'            = '8EF5409ABEB8EC6662F4474B45E1EBFFB85CE0D3771530B1EB8F37F67BD63D46'
  'Progress_color_baseColor.png'                   = '4E93A916C9C43069958C57EC27FED501CA58AF2F994360DFB2370C9141D94A8C'
  'Progress_color_metallicRoughness.png'           = 'C2888E8F5A1654BC665A8A82736F6687548DB6660A278A9D19C5274A5877F247'
  'Progress_color_normal.png'                      = 'CC2C16C73560DD79EB2944B32537802AACAEFE5A8D1FE4B8937B26A705EFBB66'
  'Soyuz_black_Isolation_metallicRoughness.png'    = 'B63C0E262FB069EB0EBB21B6DF7311D74CD89B47159FA84CCD2B2B4FA741C1C2'
  'Soyuz_color_metallicRoughness.png'              = '94E69F1BD38DFD67CE8604AD75F1CD69842940139AB6E39544ACACC3A1BC1D29'
  'mir_color_baseColor.png'                        = '388FFE8017C755957A060CF65A4276D31EA0B26D433C8A49F1774900C249BEC3'
  'mir_color_metallicRoughness.png'                = 'C16410BF1CE01B8E1FB6B8E0E09EC3CED284AEF9718ED281831D96FB9301BD84'
  'mir_color_normal.png'                           = '6004B46318D77B6F0CB6CFAD018838E56E5E1697F29DC32B68BB96B89455ABF4'
}

function Download-Asset([string]$Uri, [string]$Destination, [string]$FileName) {
  if (-not $Force -and (Test-Path $Destination)) {
    $currentHash = (Get-FileHash $Destination -Algorithm SHA256).Hash.ToUpperInvariant()
    if ($expectedHashes.ContainsKey($FileName) -and ($currentHash -eq $expectedHashes[$FileName].ToUpperInvariant())) {
      Write-Host "[skip] $FileName (already verified)"
      return
    }
  }
  Write-Host "Downloading $FileName..."
  Invoke-WebRequest -Uri $Uri -OutFile $Destination -UseBasicParsing
}

Download-Asset "$baseUri/scene.gltf" (Join-Path $outDir 'scene.gltf') 'scene.gltf'
Download-Asset "$baseUri/scene.bin" (Join-Path $outDir 'scene.bin') 'scene.bin'

$textures = @(
  'Mir_Antenna_baseColor.png',
  'Mir_Antenna_metallicRoughness.png',
  'Mir_White_Isolation_baseColor.png',
  'Progress_black_Isolation_metallicRoughness.png',
  'Progress_black_Isolation_normal.png',
  'Progress_color_baseColor.png',
  'Progress_color_metallicRoughness.png',
  'Progress_color_normal.png',
  'Soyuz_black_Isolation_metallicRoughness.png',
  'Soyuz_color_metallicRoughness.png',
  'mir_color_baseColor.png',
  'mir_color_metallicRoughness.png',
  'mir_color_normal.png'
)

foreach ($t in $textures) {
  Download-Asset "$baseUri/textures/$t" (Join-Path $texDir $t) $t
}
Write-Host 'Done downloading Mir & Soyuz!'

# SHA256 provenance check
$allFiles = @(
  (Join-Path $outDir 'scene.gltf'),
  (Join-Path $outDir 'scene.bin')
) + ($textures | ForEach-Object { Join-Path $texDir $_ })
$mismatch = $false
foreach ($f in $allFiles) {
  $name = Split-Path $f -Leaf
  if ($expectedHashes.ContainsKey($name)) {
    $actual = (Get-FileHash $f -Algorithm SHA256).Hash.ToUpperInvariant()
    $expected = $expectedHashes[$name].ToUpperInvariant()
    if ($actual -ne $expected) {
      Write-Warning "SHA256 mismatch for ${name}: expected $expected, got $actual"
      $mismatch = $true
    }
  }
}
if ($mismatch) { throw 'SHA256 provenance check failed - aborting.' }
Write-Host 'SHA256 provenance check passed for all Mir & Soyuz files.'
