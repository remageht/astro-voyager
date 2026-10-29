$baseUri = 'https://raw.githubusercontent.com/Utkarsh575/Smart-India-Hackathon-2022/main/public/mir'
$outDir = 'blender/out/mir_src'
$texDir = Join-Path $outDir 'textures'
New-Item -ItemType Directory -Path $texDir -Force | Out-Null

Write-Host 'Downloading scene.gltf...'
Invoke-WebRequest -Uri "$baseUri/scene.gltf" -OutFile (Join-Path $outDir 'scene.gltf')

Write-Host 'Downloading scene.bin...'
Invoke-WebRequest -Uri "$baseUri/scene.bin" -OutFile (Join-Path $outDir 'scene.bin')

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
  Write-Host "Downloading texture $t..."
  Invoke-WebRequest -Uri "$baseUri/textures/$t" -OutFile (Join-Path $texDir $t)
}
Write-Host 'Done downloading Mir & Soyuz!'
