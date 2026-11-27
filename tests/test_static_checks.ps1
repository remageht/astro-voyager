# Static checks for astro-voyager v1.0.0 (ASCII only).
# Run: powershell -ExecutionPolicy Bypass -File tests/test_static_checks.ps1
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$script:fail = 0

function Check($name, $cond) {
    if ($cond) { Write-Output ('PASS: ' + $name) }
    else { Write-Output ('FAIL: ' + $name); $script:fail++ }
}

function Read-File($rel) {
    Get-Content -LiteralPath (Join-Path $root $rel) -Raw -Encoding UTF8
}

$cmake = Read-File 'CMakeLists.txt'
$catalogH = Read-File 'src/Catalog.h'
$catalogCpp = Read-File 'src/Catalog.cpp'
$cameraH = Read-File 'src/Camera.h'
$cameraCpp = Read-File 'src/Camera.cpp'
$geoH = Read-File 'src/Geodesic.h'
$geoCpp = Read-File 'src/Geodesic.cpp'
$main = Read-File 'src/main.cpp'
$readme = Read-File 'README.md'
$catalogJson = Read-File 'res/catalog.json'
$bh = Read-File 'shaders/bh.frag'
$ci = Read-File '.github/workflows/build.yml'
$dockerfile = Read-File 'Dockerfile'
$ver = (Read-File 'VERSION').Trim()
$appCpp = Read-File 'src/AppGL.cpp'
$sceneCpp = Read-File 'src/SceneManager.cpp'

Check 'cmake project astro-voyager' ($cmake -match 'project\(astro-voyager')
Check 'cmake cxx17' ($cmake -match 'CMAKE_CXX_STANDARD 17')
Check 'cmake lists Catalog.cpp' ($cmake -match 'src/Catalog\.cpp')
Check 'cmake lists Camera.cpp' ($cmake -match 'src/Camera\.cpp')
Check 'cmake lists Geodesic.cpp' ($cmake -match 'src/Geodesic\.cpp')
Check 'cmake lists Binet.cpp' ($cmake -match 'src/Binet\.cpp')
Check 'cmake lists Kerr.cpp' ($cmake -match 'src/Kerr\.cpp')
Check 'cmake lists Interaction.cpp' ($cmake -match 'src/Interaction\.cpp')
Check 'cmake lists Constellation.cpp' ($cmake -match 'src/Constellation\.cpp')
Check 'cmake lists SolarSystem.cpp' ($cmake -match 'src/SolarSystem\.cpp')
Check 'cmake lists test_interaction' ($cmake -match 'test_interaction')
Check 'cmake gl option' ($cmake -match 'ASTROVOYAGER_ENABLE_GL')
Check 'version header matches VERSION' ((Read-File 'src/Version.h') -match ('kVersion = "' + [regex]::Escape($ver) + '"'))
Check 'cmake project version matches VERSION' ($cmake -match ('project\(astro-voyager VERSION ' + [regex]::Escape($ver)))
Check 'catalog version matches VERSION' ($catalogJson -match ('"version": "' + [regex]::Escape($ver) + '"'))
Check 'constellations json exists' (Test-Path (Join-Path $root 'res/constellations.json'))
Check 'constellation header exists' (Test-Path (Join-Path $root 'src/Constellation.h'))
Check 'constellation source exists' (Test-Path (Join-Path $root 'src/Constellation.cpp'))

Check 'catalog has sgra' ($catalogCpp -match 'sgra')
Check 'catalog has 3c273' ($catalogCpp -match 'qso-3c273')
Check 'catalog has ori' ($catalogCpp -match '"ori"')
Check 'catalog has uma' ($catalogCpp -match '"uma"')
Check 'catalog has m31' ($catalogCpp -match '"m31"')
Check 'catalog has m1' ($catalogCpp -match '"m1"')
Check 'catalog has sol' ($catalogCpp -match '"sol"')
Check 'catalog has m87' ($catalogCpp -match '"m87"')
Check 'catalog types' ($catalogH -match 'BlackHole')

Check 'camera clamp' ($cameraCpp -match 'clampNearHorizon')
Check 'camera forward' ($cameraH -match 'forward\(\)')
Check 'geodesic accel' ($geoCpp -match 'geodesicAcceleration')
Check 'geodesic rk4' ($geoCpp -match 'rk4Step')
Check 'geodesic trace' ($geoH -match 'traceRay')
Check 'renormalizeTime decl' ($geoH -match 'renormalizeTime')
Check 'renormalizeTime impl' ($geoCpp -match 'void renormalizeTime\(SchwState& s, double rs\)')
Check 'rk4Step renormalizes' ($geoCpp -match '(?s)renormalizeTime\(s, rs\);\s*\r?\n\}')
Check 'glsl null cone projection decl' ($bh -match 'float nullDt\(vec3 x, vec3 v, float rs\)')
Check 'glsl projectNull in march' ($bh -match 'projectNull\(x, v, dt, u_Rs\)')
Check 'ISCO 3 Rs' ($bh -match 'rIn = \(u_DiskInK > 0\.0\) \? u_DiskInK \* u_Rs : u_Rs \* 3\.0')
Check 'novikov thorne' ($bh -match 'pow\(rIn / rDisk, 3\.0\)')
Check 'beaming g4' ($bh -match 'g \* g \* g \* g')
Check 'no ringMod' ($bh -cnotmatch 'ringMod')
Check 'observer redshift' ($bh -match 'fCam')
Check 'glsl null projection' ($bh -match 'sqrt\(max\(spatial / f, 0\.0\)\)')
Check 'cpp null projection' ($geoCpp -match 'sqrt\(std::max\(spatial / f, 0\.0\)\)')
Check 'demo b_crit' ($main -match '3\.0 \* std::sqrt\(3\.0\) / 2\.0')
Check 'demo makeInwardRay' ($main -match 'makeInwardRay')
Check 'demo dt drift' ($main -match 'dt drift after 1000 steps')

Check 'interaction header exists' (Test-Path (Join-Path $root 'src/Interaction.h'))
Check 'interaction source exists' (Test-Path (Join-Path $root 'src/Interaction.cpp'))
Check 'interaction test source exists' (Test-Path (Join-Path $root 'tests/test_interaction.cpp'))
$interH = Read-File 'src/Interaction.h'
Check 'interaction mouse capture' ($interH -match 'shouldCaptureMouseForCamera')
Check 'interaction kb movement' ($interH -match 'shouldProcessKeyboardMovement')
Check 'interaction clamp radius' ($interH -match 'clampCameraRadius')

Check 'cli list' ($main -match '--list')
Check 'cli info' ($main -match '--info')
Check 'cli demo-geodesic' ($main -match 'demo-geodesic')
Check 'cli demo-kerr' ($main -match 'demo-kerr')
Check 'cli test-interaction' ($main -match 'test-interaction')

Check 'json has stations' ($catalogJson -match '"stations"')
Check 'json sgra params' ($catalogJson -match 'shellRadius')

Check 'bh shader has RK4' ($bh -match 'rk4')
Check 'bh shader capture' ($bh -match '1\.001')
Check 'bh shader shell' ($bh -match 'u_ShellRadius')
Check 'galaxy shader exists' (Test-Path (Join-Path $root 'shaders/galaxy.frag'))
Check 'nebula shader exists' (Test-Path (Join-Path $root 'shaders/nebula.frag'))
Check 'comet frag exists' (Test-Path (Join-Path $root 'shaders/comet.frag'))
Check 'time controls' ((Read-File 'src/SceneManager.cpp') -match 'm_timeScale')
Check 'star vert exists' (Test-Path (Join-Path $root 'shaders/star.vert'))
Check 'star frag exists' (Test-Path (Join-Path $root 'shaders/star.frag'))
$badSmooth = @(Get-ChildItem (Join-Path $root 'shaders') -Include *.vert,*.frag -Recurse | Select-String -Pattern 'smoothstep\(\s*([0-9.]+)\s*,\s*([0-9.]+)' | Where-Object { [double]$_.Matches[0].Groups[1].Value -gt [double]$_.Matches[0].Groups[2].Value })
Check 'no reversed smoothstep in shaders' ($badSmooth.Count -eq 0)

Check 'readme name' ($readme -match 'astro-voyager')
Check 'readme quickstart' ($readme -match 'cmake -S')

Check 'CI matrix' (($ci -match 'windows-latest') -and ($ci -match 'ubuntu-(24\.04|latest)'))
Check 'CI docker' ($ci -match 'docker build')
Check 'CI appimage job' ($ci -match 'build-appimage')
Check 'CI appimage packaging script' ($ci -match 'packaging/build-appimage\.sh')
Check 'CI appimage deps fetch' ($ci -match 'packaging/fetch-gl-deps\.sh')
Check 'CI appimage smoke' ($ci -match '--appimage-extract')
Check 'CI appimage artifact' ($ci -match 'upload-artifact')
Check 'docker healthcheck-free build image' ($dockerfile -match 'FROM debian')

$desktop = Read-File 'packaging/astro-voyager.desktop'
Check 'desktop file exists' (Test-Path (Join-Path $root 'packaging/astro-voyager.desktop'))
Check 'packaging script exists' (Test-Path (Join-Path $root 'packaging/build-appimage.sh'))
Check 'fetch script exists' (Test-Path (Join-Path $root 'packaging/fetch-gl-deps.sh'))
Check 'packaging icon exists' (Test-Path (Join-Path $root 'packaging/icon.png'))
Check 'desktop Name' ($desktop -match '(?m)^Name=astro-voyager\r?$')
Check 'desktop Exec' ($desktop -match '(?m)^Exec=astro-voyager\r?$')
Check 'desktop Icon' ($desktop -match '(?m)^Icon=astro-voyager\r?$')
Check 'desktop Categories' ($desktop -match '(?m)^Categories=')
Check 'appimage script appimagetool' ((Read-File 'packaging/build-appimage.sh') -match 'appimagetool')
Check 'appimage script linuxdeploy' ((Read-File 'packaging/build-appimage.sh') -match 'linuxdeploy')
Check 'appimage script AppRun' ((Read-File 'packaging/build-appimage.sh') -match 'AppRun')

Check 'catalog doc exists' (Test-Path (Join-Path $root 'docs/CATALOG.md'))
Check 'arch doc exists' (Test-Path (Join-Path $root 'docs/ARCHITECTURE.md'))
Check 'build doc exists' (Test-Path (Join-Path $root 'docs/BUILD.md'))
Check 'roadmap exists' (Test-Path (Join-Path $root 'docs/ROADMAP.md'))
Check 'release doc exists' (Test-Path (Join-Path $root 'docs/RELEASE.md'))
Check 'presets defined' ((Read-File 'src/SceneManager.h') -match 'PerfPreset')
Check 'env example no secret' ((Read-File '.env.example') -notmatch 'sk-|secret123|password')
Check 'cli disk flag' ($main -match '--disk')
Check 'test_doppler_side exists' (Test-Path (Join-Path $root 'tests/test_doppler_side.ps1'))
Check 'cmake lists Journey.cpp' ($cmake -match 'src/Journey\.cpp')
Check 'cmake lists test_journey' ($cmake -match 'test_journey')
Check 'journey header exists' (Test-Path (Join-Path $root 'src/Journey.h'))
Check 'journey source exists' (Test-Path (Join-Path $root 'src/Journey.cpp'))
Check 'cmake lists Achievements.cpp' ($cmake -match 'src/Achievements\.cpp')
Check 'cmake lists test_achievements' ($cmake -match 'test_achievements')
Check 'achievements header exists' (Test-Path (Join-Path $root 'src/Achievements.h'))
Check 'achievements source exists' (Test-Path (Join-Path $root 'src/Achievements.cpp'))

$sceneH = Read-File 'src/SceneManager.h'
$sceneCpp = Read-File 'src/SceneManager.cpp'
$appCpp = Read-File 'src/AppGL.cpp'
$planetFrag = Read-File 'shaders/planet.frag'
$solarCpp = Read-File 'src/SolarSystem.cpp'
Check 'editor spawn' (($sceneH -match 'spawnRockAt') -and ($sceneCpp -match 'spawnRockAt'))
Check 'menu state' (((Read-File 'src/AppGL.h') -match 'm_inMenu') -and ($appCpp -match 'm_inMenu'))
Check 'settings exposure' (($sceneH -match 'm_exposure') -and ($appCpp -match 'getExposure'))
Check 'help overlay' (((Read-File 'src/AppGL.h') -match 'm_helpOverlay') -and ($appCpp -match 'm_helpOverlay'))
Check 'custom body struct' ($sceneH -match 'CustomBody')
Check 'star clamp' (($interH -match 'pushOutOfSphere') -and ($appCpp -match 'pushOutOfSphere'))
$galaxyFrag = Read-File 'shaders/galaxy.frag'
Check 'm31 lod u_M31Dist set' (($sceneCpp -match 'u_M31Dist') -and ($galaxyFrag -match 'u_M31Dist'))
Check 'm31 starfield mesh' (($sceneH -match 'm_m31Stars') -and ($sceneCpp -match 'm_m31Stars'))
Check 'm31 close cam near' (($appCpp -match 'm31_close') -and ($appCpp -match 'position\.z = 8'))
Check 'bh capture horizon black' ($bh -match 'if \(r <= u_Rs \* 1\.001\)')
Check 'bh close cam disk smoothing via u_CamDist' (($bh -match 'uniform float u_CamDist') -and ($sceneCpp -match 'setUniform1f\("u_CamDist"'))
Check 'bh ring gain sgra 1.8' (($bh -match 'u_BHRingK') -and ($sceneCpp -match 'u_BHRingK", isSgra \? 1\.8f'))
Check 'bh novikov-thorne disk' (($bh -match 'NT_PEAK_NORM') -and ($bh -match 'blackBodyColor'))
Check 'bh closeK full at 8Rs with wide shadow gate' (($bh -match 'smoothstep\(8\.0, 10\.0, camDistRs\)') -and ($bh -match 'smoothstep\(u_Rs \* 2\.2, u_Rs \* 3\.0, minR\)'))
Check 'quasar core gain qso 2.2' (((Read-File 'shaders/quasar.frag') -match 'u_QCoreK') -and ($sceneCpp -match 'u_QCoreK'))
Check 'sgra thin disk and band' (($bh -match 'u_ThinDisk') -and ((Read-File 'shaders/sky.frag') -match 'u_BandK') -and ($sceneCpp -match 'u_BandK'))
Check 'warp frag exists with u_WarpK' ((Test-Path (Join-Path $root 'shaders/warp.frag')) -and ((Read-File 'shaders/warp.frag') -match 'uniform float u_WarpK'))
Check 'warp off by default' (((Read-File 'src/AppGL.h') -match 'm_warpOn = false') -and ($sceneCpp -match 'g_warpK = 0\.0f'))
Check 'warp jump machine 3-4s' ((($appCpp) -match 'm_warpStateT >= 3\.0f') -and (($appCpp) -match 'm_warpStateT >= 4\.0f'))
Check 'sun corona shell drawn' ($sceneCpp -match 'sun_corona')
Check 'saturn cassini division' ((Read-File 'shaders/ring.frag') -match 'Cassini')
Check 'jupiter great red spot' ((Read-File 'shaders/planet.frag') -match 'Great Red Spot')
Check 'bh photon sub-ring ring2' ($bh -match 'ring2')
Check 'crab wisps' ((Read-File 'shaders/nebula.frag') -match 'wisp')
Check 'star twinkle phase' ((Read-File 'shaders/star.vert') -match 'v_Tw =')
Check 'm31 tidal stream' ($galaxyFrag -match 'tidal')
Check 'tour dwell phase' ((Read-File 'src/Tour.cpp') -match 'TourPhase::Dwell')
Check 'cinematic tour hook' (($appCpp) -match 'm_tour\.active')
Check 'audio engine vendored' ((Test-Path (Join-Path $root 'src/audio/AudioEngine.h')) -and ((Read-File 'src/audio/AudioEngine.cpp') -match 'MINIAUDIO_IMPLEMENTATION'))
Check 'audio mute key and volume' ((($appCpp) -match 'toggleMute') -and ((Read-File 'src/AppGL.h') -match 'm_audioVolume'))
Check 'modern glass theme' (($appCpp) -match 'applyModernThemeOnce')
Check 'win32 gui no console option' ((Read-File 'CMakeLists.txt') -match 'ASTROVOYAGER_WIN32_GUI')
Check 'horizon capture cinematic' ((($appCpp) -match 'startHorizonCapture') -and ((Read-File 'src/AppGL.h') -match 'm_captureAnim'))
Check 'slide collisions and far plane' ((((Read-File 'src/Interaction.h') -match 'slideOnSphere') -and (($appCpp) -match '1e7f')))
Check 'horizon achievement wired' ((((Read-File 'src/Achievements.h') -match 'capturedHorizon') -and (($appCpp) -match 'capturedHorizon')))
Check 'warpjump pullback and switch' ((((Read-File 'src/WarpJump.cpp') -match 'Pullback') -and ((Read-File 'src/WarpJump.cpp') -match 'switchStation')))
Check 'warp burst envelope' ((((Read-File 'shaders/warp.frag') -match 'u_Burst') -and ($sceneCpp -match 'u_Burst')))
Check 'jump ship rig' ((($appCpp) -match 'shipPos') -and ($sceneCpp -match 'Ship'))
Check 'bh hot rim at ISCO' ($bh -match 'rimQ')
Check 'galaxy HDR core' ($galaxyFrag -match '8\.5')
Check 'harness fps pinned' (((Read-File 'src/AppGL.h') -match 's_harnessPinFps') -and (($appCpp) -match 's_harnessPinFps = true'))
Check 'catalog has m51' ($catalogCpp -match '"m51"')
Check 'catalog has m104' ($catalogCpp -match '"m104"')
Check 'catalog json has m51/m104' (($catalogJson -match '"m51"') -and ($catalogJson -match '"m104"'))
Check 'catalog has psr-vela' ($catalogCpp -match '"psr-vela"')
Check 'catalog type pulsar' (($catalogCpp -match 'Pulsar') -and ($catalogCpp -match '"pulsar"'))
Check 'catalog json has psr-vela' ($catalogJson -match '"psr-vela"')
Check 'pulsar shader exists' ((Test-Path (Join-Path $root 'shaders/pulsar.frag')) -and ($sceneCpp -match 'pulsar\.frag'))
Check 'pulsar render branch' (($sceneCpp -match 'StationType::Pulsar') -and ($sceneCpp -match 'm_pulsarShader'))
Check 'galaxy per-station uniforms' ((($galaxyFrag) -match 'u_GalArms') -and ($sceneCpp -match 'u_GalPitch'))
Check 'postfx wired into render' ((($sceneCpp) -match 'm_postFx\.beginScene') -and ($sceneCpp -match 'm_postFx\.endScene') -and (Test-Path (Join-Path $root 'shaders/post_comp.frag')))
Check 'post composite texelfetch passthrough' ((Read-File 'shaders/post_comp.frag') -match 'texelFetch')
Check 'relativity doppler wired' ((((Read-File 'shaders/post_comp.frag') -match 'u_Beta') -and ((Read-File 'shaders/sky.frag') -match 'u_VelDir')) -and (($sceneCpp) -match 'm_postFx\.setFlight'))
Check 'spin angle uniform driven by simTime' (($planetFrag -match 'uniform float u_SpinAngle') -and ($sceneCpp -match 'setUniform1f\("u_SpinAngle"') -and ($solarCpp -match 'spinRate'))
Check 'time pace presets up to 1000x' (($appCpp -match '300x') -and ($appCpp -match '1000x') -and ($sceneH -match '1000\.0f') -and ($appCpp -match 'Logarithmic'))
Check 'ship hull switchable 3 styles' ((($appCpp) -match 'Ring Ship') -and ($appCpp -match 'Shuttle') -and ($sceneCpp -match 'setShipStyle') -and ($sceneCpp -match 'shipStyle'))
Check 'warp engine red tint' ((Read-File 'shaders/star.frag') -match 'u_Tint') -and ($sceneCpp -match 'u_Tint", 1\.0f, 0\.22f, 0\.10f')
Check 'solid ship shader' (((Test-Path (Join-Path $root 'shaders/ship.vert')) -and (Test-Path (Join-Path $root 'shaders/ship.frag'))) -and ($sceneCpp -match 'shaders/ship\.vert'))
Check 'disk default on' ($sceneH -match 'diskOn = true')
Check 'm87 jet helical filaments' ($bh -match 'helix')
Check 'einstein ring lensing boost' ($bh -match 'lensMag')
Check 'photo mode F8' ((($appCpp) -match 'GLFW_KEY_F8') -and (($appCpp) -match 'drawPhotoPopup'))
Check 'auto orbit O' ((($appCpp) -match 'GLFW_KEY_O') -and (($appCpp) -match 'updateOrbit'))
Check 'gravity clock HUD' ((($appCpp) -match 'dtau|clockRate') -and (($appCpp) -match 'sqrt\(1\.0 - 1\.0'))
Check 'hdr scene fbo float16' ((Read-File 'src/PostFX.cpp') -match 'GL_RGBA16F')
Check 'composite single tonemap' (((Read-File 'shaders/post_comp.frag') -match 'acesNarkowicz') -and ((Read-File 'shaders/post_comp.frag') -match '1\.0 / 2\.2'))
Check 'beginScene clears targets' ((Read-File 'src/PostFX.cpp') -match 'glClear\(GL_COLOR_BUFFER_BIT \| GL_DEPTH_BUFFER_BIT')
Check 'constellation clear linear' ($sceneCpp -match '0\.00004f')
Check 'pulsar preset uniforms' (((Read-File 'shaders/pulsar.frag') -match 'u_SpinRate') -and ($sceneCpp -match 'setUniform1f\("u_SpinRate"') -and ($sceneCpp -match 'psr-crab') -and ($sceneCpp -match 'sgr-1806'))
Check 'catalog has pulsar family' ((($catalogCpp -match '"psr-crab"') -and ($catalogCpp -match '"sgr-1806"') -and ($catalogCpp -match '"psr-b1913"')) -and (($catalogJson -match '"psr-crab"') -and ($catalogJson -match '"sgr-1806"') -and ($catalogJson -match '"psr-b1913"')))
Check 'pulsar tick sfx' ((((Read-File 'src/audio/AudioEngine.h') -match 'playTick') -and ((Read-File 'src/audio/AudioEngine.cpp') -match 'VoiceType::Tick')) -and ($appCpp -match 'm_audio\.playTick'))
Check 'tertiary photon ring' ($bh -match 'ring3')
Check 'isco corona glow' (($bh -match 'ISCO corona') -and ($bh -match 'corCol'))
Check 'quasar ejecta blob' ((Read-File 'shaders/quasar.frag') -match 'ejPh')
Check 'galaxy lod all' ((($sceneCpp) -match 'm_m51Stars') -and (($sceneCpp) -match 'm_mwStars') -and (($sceneCpp) -match 'lodStars'))
Check 'milky way station' ((($catalogCpp) -match '"mw"') -and (($catalogJson) -match '"mw"'))
Check 'sun link home' ((($appCpp) -match 'startWarpJump\("sol"\)') -and (($sceneCpp) -match 'sunWorldPos'))
Check 'cluster station' ((($catalogCpp) -match '"m13"') -and (($catalogCpp) -match 'StarCluster') -and (($catalogJson) -match '"m13"'))

# --- v3.2.0 graphics pass ---
Check 'bh disk filament strands' (($bh -match 'filUv') -and ($bh -match 'strand'))
Check 'bh jet counter-jet asymmetry' (($bh -match 'sideJet') -and ($bh -match 'crossJet'))
Check 'quasar lensed secondary arcs' ((Read-File 'shaders/quasar.frag') -match 'arcPar')
Check 'saturn ring shadow on planet' (($planetFrag -match 'ringShadow') -and ($sceneCpp -match '"u_RingTilt"'))
Check 'planet eclipse on rings u_PlanetR' (((Read-File 'shaders/ring.frag') -match 'u_PlanetR') -and ($sceneCpp -match '"u_PlanetR"'))
Check 'galilean moons and titan' ((($solarCpp -match '"io"') -and ($solarCpp -match '"europa"') -and ($solarCpp -match '"ganymede"') -and ($solarCpp -match '"callisto"') -and ($solarCpp -match '"titan"')) -and (($solarCpp -match 'd\.parentId = "jupiter"') -and ($solarCpp -match 'd\.parentId = "saturn"')))
Check 'belt density 5200' ($sceneCpp -match 'setBelt\(5200')
Check 'earth ocean glint' (($planetFrag -match 'glint') -and ($planetFrag -match '1\.0 - land'))
Check 'sol earth close shots harness' (($appCpp -match 'sol_close') -and ($appCpp -match 'earth_close'))
Check 'real Earth day map and provenance are packaged' ((Test-Path (Join-Path $root 'res/textures/earth_daymap.jpg')) -and (Test-Path (Join-Path $root 'res/textures/earth_daymap.source.json')) -and ($sceneCpp -match 'res/textures/earth_daymap\.jpg') -and ($sceneCpp -notmatch 'res/models/earth_map\.bmp'))
Check 'Earth longitude is not mirrored and latitude is north-up' (($planetFrag -match 'atan\(-p\.z, p\.x\)') -and ($planetFrag -match '0\.5 - asin\(clamp\(p\.y'))
Check 'Earth albedo and land use the same geographic map' (($planetFrag -match 'texture\(u_EarthMap, earthUv\(spN\)\)') -and ($planetFrag -match 'land = surface\.a') -and ($planetFrag -match 'albedo = surface\.rgb'))
Check 'Earth excludes procedural elevation and close-range noise' (($planetFrag -match 'u_PlanetRelief == 1 && u_Body != 4') -and ($planetFrag -match 'u_Type == 2 && u_Body != 4'))
Check 'Earth texture is per-scene sRGB with clamped poles' (($sceneH -match 'unsigned int m_earthMapTex') -and ($sceneCpp -match 'GL_SRGB8_ALPHA8') -and ($sceneCpp -match 'GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE') -and ($sceneCpp -match 'SceneManager::~SceneManager'))
Check 'planet cloud and spot uniforms use floats' (($sceneCpp -match 'setUniform1f\("u_Clouds"') -and ($sceneCpp -notmatch 'setUniform1i\("u_(Clouds|Spot)"'))
Check 'Earth native regression checks continents and oceans' (($sceneCpp -match 'Earth landmark') -and ($sceneCpp -match 'Earth view') -and ($sceneCpp -match 'first == repeat') -and ($sceneCpp -match 'first != rotated') -and ($sceneCpp -match 'first != cloudy') -and ((Read-File 'src/main.cpp') -match '--capture-earth'))
Check 'planet radius uniform u_Radius' (($planetFrag -match 'uniform float u_Radius') -and ($sceneCpp -match 'setUniform1f\("u_Radius"'))

# --- v3.2.0 content pass ---
Check 'bh sgra orbital hot spot' ($bh -match 'spotPh')
Check 'quasar jet precession' ((Read-File 'shaders/quasar.frag') -match 'precA')
Check 'moon transit shadows' (($planetFrag -match 'moonShadow') -and ($sceneCpp -match '"u_MoonOcc\['))
Check 'earth night city lights' ($planetFrag -match 'cityN')
Check 'uranus faint rings' ((($solarCpp -match 'ringTilt = 1\.5f') -and ((Read-File 'src/SolarSystem.h') -match 'ringTilt')) -and ($sceneCpp -match 'def->ringTilt'))
Check 'pluto on orbit' (($solarCpp -match '"pluto"') -and ($solarCpp -match 'Kuiper'))

# --- v3.2.0 editor + close-detail pass ---
Check 'editor object palette' (($appCpp -match '"Gas giant"') -and ($appCpp -match '"Star"'))
Check 'custom body surface fields' ((($sceneH -match 'int detail') -and ($sceneH -match 'ringOuter')) -and ($sceneH -match 'colorB'))
Check 'custom spawn by template' (($sceneH -match 'spawnCustom') -and ($sceneCpp -match 'SceneManager::spawnCustom'))
Check 'selected object live lookup' (($sceneH -match 'findCustom') -and ($appCpp -match 'findCustom'))
Check 'custom surface mapping in render' (($sceneCpp -match 'custom\.detail') -and ($sceneCpp -match 'custom\.type'))
Check 'planet close-range detail nearK' ($planetFrag -match 'nearK')

# --- v3.3.0 ship survival pass ---
Check 'ship state in Types core' (((Read-File 'src/Types.h') -match 'struct ShipState') -and ((Read-File 'src/Types.h') -match 'float fuel') -and ((Read-File 'src/Types.h') -match 'float shield'))
Check 'camera includes types' (($cameraH -match 'Types\.h') -and ($cameraCpp -match 'applyGravityStep'))
Check 'warp jump fuel gate' (($appCpp -match 'ship::canJump') -and ($appCpp -match 'ship::spendJumpFuel'))
Check 'pulsar radiation zone' (($appCpp -match 'applyRadiation') -and ($appCpp -match 'kPulsarDamageRadii'))
Check 'low shield postfx' (((Read-File 'shaders/post_comp.frag') -match 'u_ChromaK') -and ((Read-File 'shaders/post_comp.frag') -match 'u_StaticK') -and ((Read-File 'src/PostFX.cpp') -match 'setShipFx'))
Check 'gravity mass fields' (($catalogH -match 'double mass') -and ((Read-File 'src/SolarSystem.h') -match 'float mass') -and ($appCpp -match 'gravAccelToward'))
Check 'ship hud and respawn' (($appCpp -match 'drawShipHud') -and ($appCpp -match 'ImGui::ProgressBar') -and ($appCpp -match 'teleportToStation\("sol"\)'))

# --- v3.3.0 speed fx + camera views pass ---
Check 'view modes core in Types' (((Read-File 'src/Types.h') -match 'ViewMode') -and ((Read-File 'src/Types.h') -match 'nextView') -and ((Read-File 'src/Types.h') -match 'offsetCameraPos') -and ((Read-File 'src/Types.h') -match 'speedFx'))
Check 'V key camera views' (($appCpp -match 'm_keyView') -and ($appCpp -match 'view::nextView') -and ($appCpp -match 'resetRelativity'))
Check 'render camera offset' (($appCpp -match 'applyViewCamera') -and ($appCpp -match 'view::offsetCameraPos') -and ($appCpp -match 'm_renderCam'))
Check 'speed fx composite streak' (((Read-File 'shaders/post_comp.frag') -match 'u_SpeedK') -and ((Read-File 'src/PostFX.cpp') -match 'setSpeedFx') -and ((Read-File 'src/PostFX.h') -match 'setSpeedFx'))
Check 'speed fx boosts relativity' (($sceneCpp -match 'setSpeedFx') -and ($sceneCpp -match 'betaEff') -and ($sceneH -match 'm_speedBeta'))
Check 'warp streak stability' (((Read-File 'shaders/warp.frag') -match 'smoothstep\(0\.0, 0\.06') -and ((Read-File 'shaders/warp.frag') -match '0\.9965') -and ($appCpp -match 'updateSpeedFx'))

# --- v3.3.0 flight feel pass: engine hum, thruster trail, shield flash ---
Check 'engine hum audio' (((Read-File 'src/audio/AudioEngine.h') -match 'setEngineLevel') -and ((Read-File 'src/audio/AudioEngine.cpp') -match 'm_engineCurrent') -and ($appCpp -match 'setEngineLevel'))
Check 'thruster scaled trail' (($sceneCpp -match 'warpdrive::thruster') -and ($sceneCpp -match 'setThruster') -and ($appCpp -match 'warpdrive::setThruster'))
Check 'shield impact flash' (((Read-File 'shaders/post_comp.frag') -match 'u_ShieldK') -and ($appCpp -match 'shieldFlashHit') -and ((Read-File 'src/PostFX.cpp') -match 'u_ShieldK'))

# --- v3.4.0 macOS GL port pass ---
Check 'gl compat header macos branch' (((Read-File 'src/GLCompat.h') -match 'OpenGL/gl3\.h') -and ((Read-File 'src/GLCompat.h') -match 'glewInit') -and ($appCpp -match '"GLCompat\.h"') -and ($appCpp -notmatch '#include <GL/glew\.h>'))
Check 'cmake macos gl branch' (($cmake -match 'elseif\(APPLE\)') -and ($cmake -match 'GLFW_INCLUDE_NONE') -and ($cmake -match 'framework CoreAudio') -and ($cmake -match 'OpenGL::GL'))
Check 'macos gl ci job' (($ci -match 'build-macos-gl') -and ($ci -match 'otool -L') -and ($ci -match 'astro-voyager-macos-arm64'))
Check 'macos font paths' ($appCpp -match '/System/Library/Fonts/Supplemental')

# --- v3.4.0 save profiles pass ---
Check 'savegame core module' (((Read-File 'src/SaveGame.h') -match 'kSaveSchema') -and ((Read-File 'src/SaveGame.cpp') -match 'saveDeserialize') -and ($cmake -match 'src/SaveGame\.cpp'))
Check 'save slot file names' (((Read-File 'src/SaveGame.cpp') -match 'save-sandbox\.json') -and ((Read-File 'src/SaveGame.cpp') -match 'save-adventure\.json') -and ((Read-File 'src/SaveGame.cpp') -match 'settings\.json'))
Check 'save quick keys and autosave' (($appCpp -match 'GLFW_KEY_F9') -and ($appCpp -match 'GLFW_KEY_F10') -and ($appCpp -match 'saveProfileNow\(true\)'))
Check 'save customs exact restore' (($sceneH -match 'addLoadedCustom') -and ($sceneCpp -match 'SceneManager::addLoadedCustom') -and ($sceneCpp -match 'from_chars'))
Check 'save settings apply' (($appCpp -match 'applySaveSettings') -and ($appCpp -match 'm_userOverrodePreset = true') -and ($appCpp -match 'saveUtcNowISO8601'))
Check 'save test wired' (($cmake -match 'tests/test_save\.cpp') -and ($cmake -match 'add_test\(NAME test_save'))

# --- v3.5.0 story journal pass ---
Check 'story journal module' (((Read-File 'src/StoryJournal.h') -match 'class StoryJournal') -and ((Read-File 'src/StoryJournal.cpp') -match 'storyCatalog') -and ($cmake -match 'src/StoryJournal\.cpp'))
Check 'story journal HUD panel' (($appCpp -match 'drawStoryPanel') -and ($appCpp -match 'm_storyOpen') -and ($appCpp -match 'm_keyJournal') -and ($appCpp -match 'm_prevJPressed'))
Check 'story journal refresh hook' (($appCpp -match 'StoryJournal::Progress') -and ($appCpp -match 'm_story\.refresh\(sp\)'))
Check 'story journal test wired' (($cmake -match 'tests/test_story\.cpp') -and ($cmake -match 'add_test\(NAME test_story'))

# --- v3.5.0 expedition pass ---
Check 'expedition core module' (((Read-File 'src/Expedition.h') -match 'class ExpeditionState') -and ((Read-File 'src/Expedition.cpp') -match 'expeditionChapters') -and ($cmake -match 'src/Expedition\.cpp'))
Check 'expedition slides module' (((Read-File 'src/StorySlides.h') -match 'storyIntroSlides') -and ((Read-File 'src/StorySlides.cpp') -match 'drawStoryArt') -and ($cmake -match 'src/StorySlides\.cpp'))
$cinematicStoryArt = @(
  'res/story/cinematic/expedition_00_mia_stars.png',
  'res/story/cinematic/expedition_01_academy.png',
  'res/story/cinematic/expedition_02_dimming_sun.png',
  'res/story/cinematic/expedition_03_cities_dark.png',
  'res/story/cinematic/expedition_04_bridge.png',
  'res/story/cinematic/expedition_05_launch.png',
  'res/story/cinematic/expedition_06_photon_ring.png',
  'res/story/cinematic/story_07_relativistic_jet.png',
  'res/story/cinematic/story_08_quasar.png',
  'res/story/cinematic/story_09_cluster.png',
  'res/story/cinematic/story_10_colony.png'
)
Check 'cinematic story art set is complete' (($cinematicStoryArt.Count -eq 11) -and (@($cinematicStoryArt | Where-Object { -not (Test-Path (Join-Path $root $_)) }).Count -eq 0))
Check 'story slides prefer cinematic art with vector fallback' (($appCpp -match 'storyArtImage\(sl\.art\)') -and ((Read-File 'src/StorySlides.cpp') -match 'story_10_colony\.png'))
Check 'expedition play hook' (($appCpp -match 'startExpeditionIntro') -and ($appCpp -match 'startNewAdventure'))
Check 'free flight button' ($appCpp -match '(?i)free flight')
Check 'expedition map key N' (($appCpp -match 'm_keyMap') -and ($appCpp -match 'm_expeditionMapOpen'))
Check 'expedition test wired' (($cmake -match 'tests/test_expedition\.cpp') -and ($cmake -match 'add_test\(NAME test_expedition'))
Check 'adventure finale branch' (($appCpp -match 'drawFinaleChoice') -and ((Read-File 'src/Expedition.h') -match 'finaleChoice') -and ((Read-File 'src/Expedition.cpp') -match 'kFinaleHorizon') -and ((Read-File 'src/StorySlides.cpp') -match 'far horizon ending'))
Check 'expedition intro shot' (($appCpp -match 'intro\.png') -and ($appCpp -match 'startExpeditionIntro\(\);'))
Check 'prologue closed after intro shot' ($appCpp -match '(?s)introFile.*m_slidesCurrent\.clear\(\)')

# --- post-stack pass: bloom v2 (Jimenez) + anamorphic streak + FXAA ---
Check 'bloom down13 shader' ((Test-Path (Join-Path $root 'shaders/post_down13.frag')) -and ((Read-File 'src/PostFX.cpp') -match 'post_down13'))
Check 'bloom up9 shader' ((Test-Path (Join-Path $root 'shaders/post_up9.frag')) -and ((Read-File 'src/PostFX.cpp') -match 'post_up9'))
Check 'karis firefly suppression' ((Read-File 'shaders/post_down13.frag') -match '1\.0 / \(1\.0 \+ luma')
Check 'soft knee threshold' ((Read-File 'shaders/post_down13.frag') -match 'u_Knee')
Check 'additive bloom upsample' ((Read-File 'src/PostFX.cpp') -match 'GL_ONE, GL_ONE')
Check 'anamorphic streak shader' ((Test-Path (Join-Path $root 'shaders/post_streak.frag')) -and ((Read-File 'shaders/post_comp.frag') -match 'u_Streak'))
Check 'fxaa wired after composite' ((Test-Path (Join-Path $root 'shaders/post_fxaa.frag')) -and ((Read-File 'src/PostFX.cpp') -match 'post_fxaa') -and ((Read-File 'src/PostFX.cpp') -match 'GL_RGBA8'))
Check 'comp single bloom pyramid' ((Read-File 'shaders/post_comp.frag') -match 'u_Bloom;')

# --- ship lighting pass: IBL + PBR-lite + scene key light ---
Check 'ship pbr-lite shader' (((Read-File 'shaders/ship.frag') -match 'dGGX') -and ((Read-File 'shaders/ship.frag') -match 'u_KeyDir') -and ((Read-File 'shaders/ship.frag') -match 'textureLod\(u_Env'))
Check 'ship ibl env uniforms' (($sceneCpp -match 'u_EnvMaxLod') -and ($sceneCpp -match 'm_skyboxLocal->bind'))
Check 'cubemap mip support' (((Read-File 'src/Cubemap.cpp') -match 'glGenerateMipmap') -and ((Read-File 'src/Cubemap.h') -match 'setMipmaps'))
Check 'scene key light per station type' (($sceneCpp -match 'u_KeyColor') -and ($sceneCpp -match 'StationType::Quasar') -and ($sceneCpp -match 'keyCol'))
Check 'ship hero shot ref' (($appCpp -match 'ship\.png') -and ($appCpp -match 'warpdrive::setShip\(true'))
Check 'ship ref committed' (Test-Path (Join-Path $root 'docs/screens/ship.png'))

# --- ship geometry/details pass: nozzles + nav lights + panel lines ---
Check 'ship nozzle emissive geometry' (($sceneCpp -match 'nozzleCol') -and ($sceneCpp -match 'engineEmi'))
Check 'ship nav lights' (($sceneCpp -match 'navRed') -and ($sceneCpp -match 'navGreen'))
Check 'ship panel line shading' (((Read-File 'shaders/ship.frag') -match 'u_Emissive') -and ((Read-File 'shaders/ship.frag') -match 'fract') -and ((Read-File 'shaders/ship.vert') -match 'v_PartPos'))
Check 'ring ship hull-mounted rings' (($sceneCpp -match 'ringZ') -and ($sceneCpp -match '1, 0\.78f, 0\.28f') -and ($sceneCpp -match '0\.14f, 0\.013f, 0\.013f') -and ($sceneCpp -match '0\.013f, 0\.14f, 0\.013f') -and (-not (Test-Path (Join-Path $root 'blender/scripts/generate_ring_ship.py')) -or ((Read-File 'blender/scripts/generate_ring_ship.py') -match 'Ring_Mount_')) -and ($sceneCpp -notmatch '0\.13f, 0\.014f, 0\.035f'))
Check 'ring ship showcase renderer' ((-not (Test-Path (Join-Path $root 'blender/scripts/render_ring_ship_showcase.py')) -or (((Read-File 'blender/scripts/render_ring_ship_showcase.py') -match 'ring_ship_albedo\.bmp') -and ((Read-File 'blender/scripts/render_ring_ship_showcase.py') -match 'no-flip-v'))) -and (Test-Path (Join-Path $root 'docs/screens/ring_ship_showcase.png')))
Check 'nasa science asset pipeline' ((-not (Test-Path (Join-Path $root 'blender/scripts/render_nasa_showcase.py')) -or (((Read-File 'blender/scripts/render_nasa_showcase.py') -match 'voyager_b\.glb') -and ((Read-File 'blender/scripts/render_nasa_showcase.py') -match 'starmap_2020_4k_gal'))) -and (Test-Path (Join-Path $root 'tools/fetch_nasa_assets.ps1')) -and (Test-Path (Join-Path $root 'docs/NASA_ASSETS.md')) -and (Test-Path (Join-Path $root 'docs/screens/voyager_showcase.png')) -and (Test-Path (Join-Path $root 'docs/screens/hubble_showcase.png')) -and (Test-Path (Join-Path $root 'docs/screens/parker_showcase.png')) -and ((Read-File 'tools/fetch_nasa_assets.ps1') -match 'NASA-3D-Resources'))
Check 'spacex dragon asset pipeline' ((-not (Test-Path (Join-Path $root 'blender/scripts/export_dragon_game_asset.py')) -or (Test-Path (Join-Path $root 'blender/scripts/render_spacex_dragon.py'))) -and ((Read-File 'src/SceneManager.cpp') -match 'dock-dragon') -and ((Read-File 'src/SceneManager.cpp') -match 'dragon\.obj') -and ((Read-File 'src/SceneManager.cpp') -match 'stScale = 0\.095f') -and ((Read-File 'src/AppGL.cpp') -match 'captureDragonCheck') -and ((Read-File 'src/AppGL.cpp') -match 'SpaceX Dragon'))
Check 'soyuz spacecraft asset pipeline' ((-not (Test-Path (Join-Path $root 'blender/scripts/export_soyuz_game_asset.py')) -or ((Test-Path (Join-Path $root 'blender/scripts/render_soyuz_hero.py')) -and (Test-Path (Join-Path $root 'blender/scripts/render_mir_hero.py')))) -and ((Read-File 'src/SceneManager.cpp') -match 'dock-soyuz') -and ((Read-File 'src/SceneManager.cpp') -match 'soyuz\.obj') -and ((Read-File 'src/SceneManager.cpp') -match 'stScale = 0\.085f') -and ((Read-File 'src/AppGL.cpp') -match 'Soyuz-MS'))
Check 'spacecraft asset attribution' ((Test-Path (Join-Path $root 'docs/SPACECRAFT_ASSETS.md')) -and ((Read-File 'README.md') -match 'Jon Ross') -and ((Read-File 'README.md') -match 'tashtego') -and ((Read-File 'README.md') -match 'CC BY-NC 4\.0') -and ((Read-File 'docs/SPACECRAFT_ASSETS.md') -match 'CC BY 4\.0') -and ((Read-File 'docs/SPACECRAFT_ASSETS.md') -match 'CC BY-NC 4\.0'))
Check 'orbital scale hierarchy' (((Read-File 'src/SceneManager.cpp') -match 'stScale = 0\.135f') -and ((Read-File 'src/SceneManager.cpp') -match 'stScale = 0\.06f') -and ((Read-File 'src/SceneManager.cpp') -match 'stScale = 0\.38f') -and ((Read-File 'src/SceneManager.cpp') -match 'stScale = 0\.35f') -and ((Read-File 'docs/SPACECRAFT_ASSETS.md') -match '0\.297') -and ((Read-File 'docs/SPACECRAFT_ASSETS.md') -match '0\.179'))
Check 'fleet story interlude' ((Test-Path (Join-Path $root 'res/story/06_last_watch.png')) -and ((Read-File 'src/Expedition.cpp') -match 'dock-dragon\|dock-soyuz') -and ((Read-File 'src/Expedition.cpp') -match 'version=2') -and ((Read-File 'src/StorySlides.cpp') -match '06_last_watch\.png') -and ((Read-File 'src/StorySlides.cpp') -match 'Epilogue · Legacy') -and ((Read-File 'src/AppGL.cpp') -match 'advanceExpedition\(dockId\)'))

# --- settings pass v3.6.0: raytracing quality + AA + ship + difficulty ---
Check 'settings ray tracing section' (($appCpp -match 'Ray quality') -and ($appCpp -match 'Shell radius'))
Check 'settings anti-aliasing toggle' (($appCpp -match 'AA mode') -and ($appCpp -match 'setFxaa'))
Check 'settings difficulty combo' (($appCpp -match 'Difficulty') -and ($appCpp -match 'difficultyTuning'))
Check 'settings ship hull and engine combos' (($appCpp -match 'engineNames') -and ($appCpp -match 'Emerald'))
Check 'settings persisted fields' (((Read-File 'src/SaveGame.cpp') -match 'shipReflections') -and ((Read-File 'src/SaveGame.cpp') -match 'enginePalette') -and ((Read-File 'src/SaveGame.cpp') -match 'aaMode'))
Check 'ship reflections uniform' ((Read-File 'shaders/ship.frag') -match 'u_ReflK')
Check 'difficulty tuning multipliers' (((Read-File 'src/Types.h') -match 'jumpCostK = 1\.6f') -and ((Read-File 'src/Types.h') -match 't\.hints = false'))
Check 'ring ship v2 bow spike and cluster' (($sceneCpp -match 'Ring Ship v2') -and ($sceneCpp -match '-1\.38f') -and ($sceneCpp -match '1\.0471976f'))

# --- warp field pass v3.6.0: пространство искривляется за кораблём ---
Check 'warp field shader lens' (((Read-File 'shaders/post_comp.frag') -match 'u_WarpUv') -and ((Read-File 'shaders/post_comp.frag') -match 'u_WarpK') -and ((Read-File 'shaders/post_comp.frag') -match 'wake'))
Check 'warp field screen anchor' (($sceneCpp -match 'setWarpLens') -and ($sceneCpp -match 'm_warpFx'))
Check 'warp field gated by warp amount' (($appCpp -match 'warpK\);') -and ((Read-File 'src/SceneManager.h') -match 'float warpK = 0\.0f'))

# --- ships v2 pass v3.6.0: детали Dart/Shuttle, варп-роли, дистанция/ETA ---
Check 'ship hull warp roles' (((Read-File 'src/Types.h') -match 'styleCanWarp') -and ($appCpp -match 'styleCanWarp'))
Check 'warp panel distance and eta' (($appCpp -match 'formatDistanceLy') -and ($appCpp -match 'formatEtaYears'))
Check 'catalog station distances' (($catalogH -match 'distanceLy') -and ($catalogCpp -match '26000\.0') -and ($catalogCpp -match '2\.54e6'))
Check 'dart v2 details' (($sceneCpp -match 'Dart v2') -and ($sceneCpp -match '0\.095f, -0\.04f'))
Check 'shuttle v2 details' (($sceneCpp -match 'Shuttle v2') -and ($sceneCpp -match '0\.35f \* static_cast<float>\(s\)'))

# --- portable release pass v3.6.0: Windows "с коробки" + вшитые шейдеры ---
Check 'embedded shaders header' (Test-Path (Join-Path $root 'src/EmbeddedShaders.h'))
Check 'embedded shaders generator' ((Test-Path (Join-Path $root 'packaging/embed_shaders.cmake')) -and ($cmake -match 'embed_shaders') -and ($cmake -match 'ASTRO_EMBED_CPP'))
Check 'shader embedded fallback' ((Read-File 'src/Shader.cpp') -match 'embeddedShader')
Check 'windows portable release job' (($ci -match 'build-windows-gl') -and ($ci -match 'astro-voyager\.exe') -and ($ci -match 'windows-x86_64\.zip'))

# --- fx split pass v3.6.0: ускорение и варп — разные явления, мягче ---
Check 'main menu hero decluttered' (($appCpp -notmatch '##heroGlassHud') -and ($appCpp -match '##pageArchive'))
Check 'warp lens only for warp drive' (($sceneCpp -match 'const float lensK = m_warpFx;') -and ((Read-File 'shaders/post_comp.frag') -match '0\.18 \* k'))
Check 'fx amplitudes softened' (((Read-File 'shaders/post_comp.frag') -match '0\.26, 0\.0, 0\.32') -and ((Read-File 'shaders/warp.frag') -match '0\.62 \+ fringe'))

# --- colonies pass v3.6.0: M13 «система Hercules» + колонизация ---
Check 'colony sites catalog' (($sceneCpp -match '"mars", "Mars", "sol"') -and ($sceneCpp -match '"hercules-b", "Hercules b", "m13"'))
Check 'm13 system detail' (($sceneCpp -match '5200') -and ($sceneCpp -match 'Hercules Prime') -and ($sceneCpp -match 'm_clusterBright'))
Check 'colony saved in profile' (((Read-File 'src/SaveGame.h') -match 'colonies') -and ((Read-File 'src/SaveGame.cpp') -match '"colonies"'))
Check 'colonization hotkey' (($appCpp -match 'm_keyColony') -and ($appCpp -match 'foundColony') -and ($appCpp -match 'm_colonyHintId'))
Check 'colony marker ring' (($sceneCpp -match 'hasColony\("hercules-b"\)') -and ($sceneCpp -match 'hasColony\("mars"\)'))
Check 'colony save round-trip test' ((Read-File 'tests/test_save.cpp') -match 'progress\.colonies = \{"mars", "hercules-b"\}')

# --- detail + cutscene pass v3.6.0: кламп звёзд, близость колоний ---
Check 'star max px clamp' (((Read-File 'shaders/star.vert') -match 'u_MaxPx') -and ($sceneCpp -match '24\.0f') -and ($sceneCpp -match '34\.0f'))
Check 'colony marker proximity gate' ($appCpp -match '60\.0f')

# --- world-anchored imposters v3.6.0: объекты, а не картинки ---
Check 'quasar world anchor' (((Read-File 'shaders/quasar.frag') -match 'u_ObjNdc') -and ($sceneCpp -match 'impNdcX'))
Check 'nebula world anchor' (((Read-File 'shaders/nebula.frag') -match 'u_ObjZoom') -and ($sceneCpp -match 'u_ObjZoom'))

# --- horizon cutscene v3.6.0: прилив, вспышка, выброс ---
Check 'horizon cutscene phases' (($appCpp -match 'm_captureT < 1\.4f') -and ($appCpp -match 'm_horizonWarped') -and ($appCpp -match '3\.2f'))
Check 'horizon postfx uniforms' (((Read-File 'shaders/post_comp.frag') -match 'u_HorizonK') -and ((Read-File 'src/PostFX.cpp') -match 'u_HorizonK'))
Check 'quasar capture instead of slide' ($appCpp -match 'qd <= qR \+ 0\.5')

# --- controls page v3.6.0: мышь + переназначение клавиш ---
Check 'controls settings saved' (((Read-File 'src/SaveGame.h') -match 'mouseSensitivity') -and ((Read-File 'src/SaveGame.cpp') -match 'keyColony'))
Check 'controls rebind ui' (($appCpp -match 'm_captureKeyFor') -and ($appCpp -match 'keyNameForCode'))
Check 'mouse sensitivity applied' ($appCpp -match '0\.003f \* m_mouseSensitivity')
Check 'rebindable hotkeys' (($appCpp -match 'm_keyJournal') -and ($appCpp -match 'm_keyMap') -and ($appCpp -match 'm_keyTour'))
Check 'controls save round-trip test' ((Read-File 'tests/test_save.cpp') -match 'keyColony = 88')

# --- ship inertia + orbit assist v3.6.0 ---
Check 'ship inertial flight' (($appCpp -match 'm_shipVel') -and ($appCpp -match 'resp = m_moveInputActive \? 3\.0 : 0\.9'))
Check 'orbit assist math' (((Read-File 'src/Types.h') -match 'orbitAssist') -and ((Read-File 'tests/test_interaction.cpp') -match 'orbitAssist'))

# --- asteroid barrage pass D v3.6.0: поле, уклонение, урон щита ---
Check 'warp asteroid field logic' (((Read-File 'src/WarpJump.h') -match 'kAstChance') -and ((Read-File 'src/WarpJump.cpp') -match 'asteroidEnter'))
Check 'asteroid dodge input' (((Read-File 'src/AppGL.cpp') -match 'setDodgeAxis') -and ((Read-File 'src/AppGL.h') -match 'm_astLane'))
Check 'asteroid shield damage' (($appCpp -match 'kAstDamage') -and ($appCpp -match 'shieldFlashHit'))
Check 'asteroid field test' ((Read-File 'tests/test_interaction.cpp') -match 'asteroid roll 0\.9')

# v3.8.0: полировка
Check 'hazard difficulty scaling' ((($appCpp) -match 'hazardK') -and ((Read-File 'src/Types.h') -match 'hazardK'))
Check 'load aborts ghost jump' ((((Read-File 'src/AppGL.h') -match 'stopJumpCine') -and (($appCpp) -match 'if \(m_jumpCine && stopJumpCine\)')) -and ((Read-File 'tests/test_interaction.cpp') -match 'hazard'))

# v3.8.1: AV-гигиена релиза
Check 'exe icon + versioninfo resource' ((Test-Path (Join-Path $root 'packaging/app.ico')) -and (Test-Path (Join-Path $root 'packaging/app.win.rc.in')))
Check 'rc wired into cmake' (($cmake -match 'app\.win\.rc\.in') -and ($cmake -match 'AV_ICON_PATH'))
Check 'av readme in zip' ((Test-Path (Join-Path $root 'packaging/win-av-readme.txt')) -and ((Read-File '.github/workflows/build.yml') -match 'CHECKSUMS\.txt'))
# v4.2.0: Cosmic Events Pass 1 - Kilonova GW170817
Check 'kilonova frag exists' (Test-Path (Join-Path $root 'shaders/kilonova.frag'))
Check 'catalog has gw170817' (($catalogCpp -match '"gw170817"') -and ($catalogJson -match '"gw170817"'))
Check 'catalog type kilonova' (($catalogH -match 'Kilonova') -and ($catalogCpp -match 'kilonova'))
Check 'kilonova render branch' (($sceneCpp -match 'StationType::Kilonova') -and ($sceneCpp -match 'm_kilonovaShader'))
Check 'kilonova inspector controls' (($appCpp -match 'Kilonova Event') -and ($appCpp -match 'triggerKilonovaExplosion'))
# v4.2.0: Cosmic Events Pass 2 - Supernova SN 1987A
Check 'supernova frag exists' (Test-Path (Join-Path $root 'shaders/supernova.frag'))
Check 'catalog has sn1987a' (($catalogCpp -match '"sn1987a"') -and ($catalogJson -match '"sn1987a"'))
Check 'catalog type supernova' (($catalogH -match 'Supernova') -and ($catalogCpp -match 'supernova'))
Check 'supernova render branch' (($sceneCpp -match 'StationType::Supernova') -and ($sceneCpp -match 'm_supernovaShader'))
Check 'supernova inspector controls' (($appCpp -match 'Supernova Event') -and ($appCpp -match 'triggerSupernovaExplosion'))
# Story & Romance Arc: Egorka & Mia
Check 'story journal romance entries' (((Read-File 'src/StoryJournal.cpp') -match '"sn1987a"') -and ((Read-File 'src/StoryJournal.cpp') -match '"gw170817"') -and ((Read-File 'src/StoryJournal.cpp') -match '"letter-to-earth"'))
Check 'story slides romance dialogues' (((Read-File 'src/StorySlides.cpp') -match 'Mia') -and ((Read-File 'src/StorySlides.cpp') -match 'Egorka') -and ((Read-File 'src/StorySlides.cpp') -match 'Casimir'))
Check 'story test wired' (($cmake -match 'tests/test_story\.cpp') -and ($cmake -match 'add_test\(NAME test_story'))
# v4.2.0: Cosmic Events Pass 3 - Tidal Disruption Event AT2019qiz (TDE)
Check 'tde frag exists' (Test-Path (Join-Path $root 'shaders/tde.frag'))
Check 'catalog has at2019qiz' (($catalogCpp -match '"at2019qiz"') -and ($catalogJson -match '"at2019qiz"'))
Check 'catalog type tidal_disruption' (($catalogH -match 'TidalDisruption') -and ($catalogCpp -match 'tidal_disruption'))
Check 'tde render branch' (($sceneCpp -match 'StationType::TidalDisruption') -and ($sceneCpp -match 'm_tdeShader'))
Check 'tde inspector controls' (($appCpp -match 'Tidal Disruption Event') -and ($appCpp -match 'triggerTdeExplosion'))
Check 'story journal has at2019qiz' (((Read-File 'src/StoryJournal.cpp') -match '"at2019qiz"') -and ((Read-File 'tests/test_story.cpp') -match '"at2019qiz"'))

# v4.2.0: Cosmic Events Pass 4 - Pillars of Creation (M16) & Crab Nebula PWN
Check 'pillars frag exists' (Test-Path (Join-Path $root 'shaders/pillars.frag'))
Check 'catalog has m16' (($catalogCpp -match '"m16"') -and ($catalogJson -match '"m16"'))
Check 'pillars render branch' (($sceneCpp -match 'm_pillarsShader') -and ($sceneCpp -match 'u_InfraredBlend'))
Check 'pillars inspector controls' (($appCpp -match 'Pillars of Creation') -and ($appCpp -match 'IR Blend'))
Check 'crab pwn controls' (($appCpp -match 'Crab PWN') -and ($appCpp -match 'PWN Mode') -and ($sceneCpp -match 'u_PwnMode'))
Check 'story journal has m16' (((Read-File 'src/StoryJournal.cpp') -match '"m16"') -and ((Read-File 'tests/test_story.cpp') -match '"m16"'))

# v4.6.0: Science Spectrometer, Ship Codex, and Live Radio Chatter
Check 'science header exists' (Test-Path (Join-Path $root 'src/ScienceSystem.h'))
Check 'science source exists' (Test-Path (Join-Path $root 'src/ScienceSystem.cpp'))
Check 'science test source exists' (Test-Path (Join-Path $root 'tests/test_science.cpp'))
Check 'cmake lists ScienceSystem.cpp' ((Read-File 'CMakeLists.txt') -match 'src/ScienceSystem\.cpp')
Check 'cmake lists test_science' ((Read-File 'CMakeLists.txt') -match 'test_science')

# v4.6.1: Planetary Landing, Deployable Gear & Surface Operations
Check 'landing header exists' (Test-Path (Join-Path $root 'src/LandingSystem.h'))
Check 'landing source exists' (Test-Path (Join-Path $root 'src/LandingSystem.cpp'))
Check 'landing test source exists' (Test-Path (Join-Path $root 'tests/test_landing.cpp'))
Check 'cmake lists LandingSystem.cpp' ((Read-File 'CMakeLists.txt') -match 'src/LandingSystem\.cpp')
Check 'cmake lists test_landing' ((Read-File 'CMakeLists.txt') -match 'test_landing')

# v4.8.0: Astro-Zettelkasten / Second Brain & Mind Matrix
Check 'zettelkasten header exists' (Test-Path (Join-Path $root 'src/Zettelkasten.h'))
Check 'zettelkasten source exists' (Test-Path (Join-Path $root 'src/Zettelkasten.cpp'))
Check 'zettelkasten test source exists' (Test-Path (Join-Path $root 'tests/test_zettelkasten.cpp'))
Check 'cmake lists Zettelkasten.cpp' ((Read-File 'CMakeLists.txt') -match 'src/Zettelkasten\.cpp')
Check 'cmake lists test_zettelkasten' ((Read-File 'CMakeLists.txt') -match 'test_zettelkasten')

# v5.3.0: Epilogue Cinematic & Chronicle Scroll
Check 'epilogue header exists' (Test-Path (Join-Path $root 'src/EpilogueSystem.h'))
Check 'epilogue source exists' (Test-Path (Join-Path $root 'src/EpilogueSystem.cpp'))
Check 'epilogue test source exists' (Test-Path (Join-Path $root 'tests/test_epilogue.cpp'))
Check 'cmake lists EpilogueSystem.cpp' ((Read-File 'CMakeLists.txt') -match 'src/EpilogueSystem\.cpp')
Check 'cmake lists test_epilogue' ((Read-File 'CMakeLists.txt') -match 'test_epilogue')
Check 'app has epilogue cinematic overlay' ((Read-File 'src/AppGL.cpp') -match 'drawEpilogueCinematicOverlay')


# Stage 4: Perf Benchmark Harness
Check 'benchmark header exists' (Test-Path (Join-Path $root 'src/BenchmarkSystem.h'))
Check 'benchmark source exists' (Test-Path (Join-Path $root 'src/BenchmarkSystem.cpp'))
Check 'benchmark test source exists' (Test-Path (Join-Path $root 'tests/test_benchmark.cpp'))
Check 'cmake lists BenchmarkSystem.cpp' ((Read-File 'CMakeLists.txt') -match 'src/BenchmarkSystem\.cpp')
Check 'cmake lists test_benchmark' ((Read-File 'CMakeLists.txt') -match 'test_benchmark')


# Stage 5: Atmosphere & Zettelkasten 2.0
Check 'atmosphere header exists' (Test-Path (Join-Path $root 'src/AtmosphereSystem.h'))
Check 'atmosphere source exists' (Test-Path (Join-Path $root 'src/AtmosphereSystem.cpp'))
Check 'atmosphere test source exists' (Test-Path (Join-Path $root 'tests/test_atmosphere.cpp'))
Check 'cmake lists AtmosphereSystem.cpp' ((Read-File 'CMakeLists.txt') -match 'src/AtmosphereSystem\.cpp')
Check 'cmake lists test_atmosphere' ((Read-File 'CMakeLists.txt') -match 'test_atmosphere')
Check 'zettel backlinks' ((Read-File 'src/Zettelkasten.h') -match 'getIncomingLinks')
Check 'zettel hypotheses' ((Read-File 'src/Zettelkasten.h') -match 'ZettelHypothesis')
Check 'zettel resolve' ((Read-File 'src/Zettelkasten.cpp') -match 'resolveHypothesis')


# Stage 6 Station Hub RP economy
Check 'station hub warpCapacitor' ((Read-File 'src/Types.h') -match 'warpCapacitorLevel')
Check 'station hub shieldMatrix' ((Read-File 'src/Types.h') -match 'shieldMatrixLevel')
Check 'station hub rcs' ((Read-File 'src/Types.h') -match 'rcsGimbalLevel')
Check 'station hub dock button' ((Read-File 'src/AppGL.cpp') -match 'DOCKING')
Check 'station hub RP spend' ((Read-File 'src/AppGL.cpp') -match 'spendResearchPoints')
Check 'station hub save warpCap' ((Read-File 'src/SaveGame.cpp') -match 'warpCapacitorLevel')


# Solar Rebirth cinematic
Check 'solar rebirth camera' ((Read-File 'src/EpilogueSystem.h') -match 'SolarRebirthCamera')
Check 'solar rebirth impl' ((Read-File 'src/EpilogueSystem.cpp') -match 'solarRebirthCamera')
Check 'solar rebirth app' ((Read-File 'src/AppGL.cpp') -match 'solarRebirthCamera')

# v5.4.0: Accessibility & UI Ergonomics (Issue #110)
Check 'accessibility uiScale in SaveGame.h' ((Read-File 'src/SaveGame.h') -match 'uiScale')
Check 'accessibility colorblindMode in SaveGame.h' ((Read-File 'src/SaveGame.h') -match 'colorblindMode')
Check 'accessibility AccessiblePalette in Types.h' ((Read-File 'src/Types.h') -match 'AccessiblePalette')
Check 'accessibility getAccessiblePalette in Types.h' ((Read-File 'src/Types.h') -match 'getAccessiblePalette')
Check 'accessibility settings tab in AppGL.cpp' ((Read-File 'src/AppGL.cpp') -match 'Accessibility')
Check 'accessibility font scaling in AppGL.cpp' ((Read-File 'src/AppGL.cpp') -match 'FontGlobalScale')
Check 'accessibility audio cue in AppGL.cpp' ((Read-File 'src/AppGL.cpp') -match 'm_audioProximityCue')


# Roadmap Item #5: Procedural Mission Generator (Issue #114)
Check 'mission header exists' (Test-Path (Join-Path $root 'src/MissionSystem.h'))
Check 'mission source exists' (Test-Path (Join-Path $root 'src/MissionSystem.cpp'))
Check 'mission test source exists' (Test-Path (Join-Path $root 'tests/test_mission.cpp'))
Check 'cmake lists MissionSystem.cpp' ((Read-File 'CMakeLists.txt') -match 'src/MissionSystem\.cpp')
Check 'cmake lists test_mission' ((Read-File 'CMakeLists.txt') -match 'test_mission')
Check 'mission contracts board in AppGL.cpp' ((Read-File 'src/AppGL.cpp') -match 'Contracts & Missions')
Check 'mission hud widget in AppGL.cpp' ((Read-File 'src/AppGL.cpp') -match 'drawMissionHud')

# Roadmap Item #3: Dynamic Weather & Atmospheric Re-entry (Issue #116)
Check 'weather type enum in AtmosphereSystem.h' ((Read-File 'src/AtmosphereSystem.h') -match 'enum class WeatherType')
Check 'weather condition in AtmosphereSystem.h' ((Read-File 'src/AtmosphereSystem.h') -match 'struct WeatherCondition')
Check 'updatePlanetaryWeather in AtmosphereSystem.cpp' ((Read-File 'src/AtmosphereSystem.cpp') -match 'updatePlanetaryWeather')
Check 'computeAngleOfAttackDeg in AtmosphereSystem.cpp' ((Read-File 'src/AtmosphereSystem.cpp') -match 'computeAngleOfAttackDeg')
Check 'weather uniforms in cockpit.frag' ((Read-File 'shaders/cockpit.frag') -match 'uniform int   u_WeatherType')
Check 'lightning flash in cockpit.frag' ((Read-File 'shaders/cockpit.frag') -match 'u_LightningFlash')
Check 'weather ambience in AudioEngine.h' ((Read-File 'src/audio/AudioEngine.h') -match 'setWeatherAmbience')
Check 'thunder crack in AudioEngine.cpp' ((Read-File 'src/audio/AudioEngine.cpp') -match 'VoiceType::Thunder')
Check 'reentry telemetry in CockpitController.h' ((Read-File 'src/CockpitController.h') -match 'setReentryTelemetry')
Check 'flight atmospheric entry in AppGL.cpp' ((Read-File 'src/AppGL.cpp') -match 'updatePlanetaryWeather')

# Roadmap Item #8: Automated Release Packaging & CI (Issue #118)
Check 'verify_release_assets script exists' (Test-Path (Join-Path $root 'packaging/verify_release_assets.py'))
Check 'build.yml has unified release job' ((Read-File '.github/workflows/build.yml') -match 'release:\s+name: Create & Publish GitHub Release')
Check 'build.yml generates sha256sums' ((Read-File '.github/workflows/build.yml') -match 'sha256sum \* > SHA256SUMS\.txt')
Check 'build.yml validates release assets' ((Read-File '.github/workflows/build.yml') -match 'verify_release_assets\.py')

# Stage 5 / Release 1.0: Gamepad & Steam Deck Integration (Issue #120)
Check 'gamepad header exists' (Test-Path (Join-Path $root 'src/GamepadSystem.h'))
Check 'gamepad source exists' (Test-Path (Join-Path $root 'src/GamepadSystem.cpp'))
Check 'gamepad test source exists' (Test-Path (Join-Path $root 'tests/test_gamepad.cpp'))
Check 'cmake lists GamepadSystem.cpp' ((Read-File 'CMakeLists.txt') -match 'src/GamepadSystem\.cpp')
Check 'cmake lists test_gamepad' ((Read-File 'CMakeLists.txt') -match 'test_gamepad')
Check 'gamepad nav enabled in AppGL.cpp' ((Read-File 'src/AppGL.cpp') -match 'ImGuiConfigFlags_NavEnableGamepad')
Check 'gamepad deadzone in GamepadSystem.h' ((Read-File 'src/GamepadSystem.h') -match 'applyRadialDeadzone')
Check 'gamepad glyphs in GamepadSystem.h' ((Read-File 'src/GamepadSystem.h') -match 'getButtonGlyph')
Check 'gamepad adaptive prompt in AppGL.h' ((Read-File 'src/AppGL.h') -match 'getPrompt')

# Stage 5 / Release 1.0: True Endings Visual & Cinematic Expansion (Issue #122)
Check 'eden descent camera in EpilogueSystem.h' ((Read-File 'src/EpilogueSystem.h') -match 'EdenDescentCamera')
Check 'multiverse singularity camera in EpilogueSystem.h' ((Read-File 'src/EpilogueSystem.h') -match 'MultiverseSingularityCamera')
Check 'alliance relic camera in EpilogueSystem.h' ((Read-File 'src/EpilogueSystem.h') -match 'AllianceRelicCamera')
Check 'evaluateEpilogueCinematic in EpilogueSystem.h' ((Read-File 'src/EpilogueSystem.h') -match 'evaluateEpilogueCinematic')
Check 'eden descent impl in EpilogueSystem.cpp' ((Read-File 'src/EpilogueSystem.cpp') -match 'edenDescentCamera')
Check 'multiverse singularity impl in EpilogueSystem.cpp' ((Read-File 'src/EpilogueSystem.cpp') -match 'multiverseSingularityCamera')
Check 'alliance relic impl in EpilogueSystem.cpp' ((Read-File 'src/EpilogueSystem.cpp') -match 'allianceRelicCamera')
Check 'evaluateEpilogueCinematic in EpilogueSystem.cpp' ((Read-File 'src/EpilogueSystem.cpp') -match 'evaluateEpilogueCinematic')
Check 'evaluateEpilogueCinematic in AppGL.cpp' ((Read-File 'src/AppGL.cpp') -match 'evaluateEpilogueCinematic')
Check 'true ending resonator badge in AppGL.cpp' ((Read-File 'src/AppGL.cpp') -match 'True Ending Resonator Badge')

# Stage 5 / Release 1.0: Realism, Visual Quality & Performance Optimization (Issue #124)
Check 'planet shader GGX ocean glint' ((Read-File 'shaders/planet.frag') -match 'Cook-Torrance GGX ocean sun glint')
Check 'planet shader exoplanet cloud shadows' ((Read-File 'shaders/planet.frag') -match 'cloudShadowExo')
Check 'planet shader Mie aerosol phase' ((Read-File 'shaders/planet.frag') -match 'miePhase \* 12\.5')
Check 'bh shader far-field raymarch acceleration' ((Read-File 'shaders/bh.frag') -match 'farFactor = 1\.0 \+ 0\.38')
Check 'SceneManager FrameStats in SceneManager.h' ((Read-File 'src/SceneManager.h') -match 'getLastFrameStats')
Check 'SceneManager frustum culling in SceneManager.cpp' ((Read-File 'src/SceneManager.cpp') -match 'm_frameStats\.culledBodies\+\+')
Check 'BenchmarkSystem p99 telemetry in BenchmarkSystem.h' ((Read-File 'src/BenchmarkSystem.h') -match 'p99FrameTimeMs')
Check 'test_planet_lod has GGX & culling tests' ((Read-File 'tests/test_planet_lod.cpp') -match 'Cook-Torrance GGX & Schlick Fresnel conservation')
Check 'test_benchmark has telemetry test' ((Read-File 'tests/test_benchmark.cpp') -match 'Test 4 extended telemetry metrics')
# Stage 5 / Release 1.0: Zero-UI Contextual HUD & Unified Terminal (Issue #131)
Check 'zero ui contextual flight hud and terminal' (((Read-File 'src/AppGL.h') -match 'm_zeroUiMode') -and ((Read-File 'src/AppGL.h') -match 'drawContextualFlightHud') -and ((Read-File 'src/AppGL.h') -match 'drawUnifiedShipTerminal') -and ($appCpp -match 'updateDynamicHudAlphas'))
Check 'minimalist core vitals flight hud tri-arc' (($appCpp -match 'baseVitalsAlpha') -and ($appCpp -match 'float fu = std::clamp') -and ($appCpp -match 'float fuAng = minAng'))

# Stage 5 / Release 1.0: Flight hotkeys & Zero-UI warp/quest visibility (Issue #135)
Check 'flight hotkeys: keyboard nav only in UI screens' ($appCpp -match 'uiNavWanted')
Check 'flight hotkeys: disable NavEnableKeyboard during flight' ($appCpp -match 'ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard')
Check 'flight hotkeys: dock-E gated on no slides and closed terminal' ($appCpp -match 'm_slidesCurrent\.empty\(\) && !m_shipTerminalOpen')
Check 'warp engage helper shared by hotkey and HUD' ($appCpp -match 'void AppGL::tryEngageWarp')
Check 'zero-ui warp control plate' ($appCpp -match '#135 Zero-UI warp plate')
Check 'mission HUD visible in Zero-UI' ($appCpp -match '#135 mission HUD visible in Zero-UI too')

# Stage 5 / Release 1.0: Ship AI Egorka Voice Announcer (Issue #137)
$audioH = Read-File 'src/audio/AudioEngine.h'
$audioCpp = Read-File 'src/audio/AudioEngine.cpp'
Check 'AudioEngine has EgorkaVoiceCue enum' (($audioH -match 'enum class EgorkaVoiceCue') -and ($audioH -match 'DialOj287') -and ($audioH -match 'DialDrillEureka'))
Check 'AudioEngine has playEgorkaVoice method' ($audioH -match 'playEgorkaVoice\(EgorkaVoiceCue cue')
Check 'AudioEngine has Egorka voice enable and volume control' (($audioH -match 'setEgorkaVoiceEnabled') -and ($audioH -match 'setEgorkaVoiceVolume'))
Check 'AudioEngine implements playEgorkaVoice with PCM cache' (($audioCpp -match 'void AudioEngine::playEgorkaVoice') -and ($audioCpp -match 'm_egorkaCache') -and ($audioCpp -match 'dial_oj287\.wav'))
Check 'AppGL triggers Egorka voice on warp, gear, docking, and telemetry' (($appCpp -match 'playEgorkaVoice\(AudioEngine::EgorkaVoiceCue::WarpOn\)') -and ($appCpp -match 'playEgorkaVoice\(AudioEngine::EgorkaVoiceCue::DockOk\)') -and ($appCpp -match 'playEgorkaVoice\(AudioEngine::EgorkaVoiceCue::GearDown\)'))
Check 'AppGL triggers Egorka dialogue on bridge comms and landing' (($appCpp -match 'playEgorkaVoice\(AudioEngine::EgorkaVoiceCue::DialOj287\)') -and ($appCpp -match 'playEgorkaVoice\(AudioEngine::EgorkaVoiceCue::DialHardLanding\)') -and ($appCpp -match 'playEgorkaVoice\(AudioEngine::EgorkaVoiceCue::DialDrillEureka\)'))
Check 'res/audio/egorka contains generated voice cues' ((Test-Path 'res/audio/egorka/warp_on.wav') -and (Test-Path 'res/audio/egorka/dock_ok.wav') -and (Test-Path 'res/audio/egorka/sys_online.wav'))
Check 'res/audio/egorka contains generated story dialogues' ((Test-Path 'res/audio/egorka/dial_oj287.wav') -and (Test-Path 'res/audio/egorka/dial_sn1987a.wav') -and (Test-Path 'res/audio/egorka/dial_drill_eureka.wav'))

# Stage 5 / Release 1.0: Female Crew Radio Comms & Dialogues (Issue #145)
Check 'AudioEngine has female crew voice cues (Mia, Elena, Aurora)' (($audioH -match 'MiaM16') -and ($audioH -match 'ElenaSgra') -and ($audioH -match 'AuroraSysOnline') -and ($audioH -match 'MiaTouchdown'))
Check 'AudioEngine implements female crew audio files search' (($audioCpp -match 'mia_m16\.wav') -and ($audioCpp -match 'crew_female'))
Check 'AppGL triggers female crew voice on bridge comms and safe touchdown' (($appCpp -match 'playEgorkaVoice\(AudioEngine::EgorkaVoiceCue::MiaM16\)') -and ($appCpp -match 'playEgorkaVoice\(AudioEngine::EgorkaVoiceCue::ElenaSgra\)') -and ($appCpp -match 'playEgorkaVoice\(AudioEngine::EgorkaVoiceCue::MiaTouchdown\)'))
Check 'AppGL Ship Terminal TAB 3 has female voice soundboard' (($appCpp -match 'kFemaleCrewBtns') -and ($appCpp -match 'ElenaSgra') -and ($appCpp -match 'AuroraSysOnline'))
Check 'res/audio/crew_female contains generated female voice lines' ((Test-Path 'res/audio/crew_female/mia_m16.wav') -and (Test-Path 'res/audio/crew_female/mia_touchdown.wav') -and (Test-Path 'res/audio/crew_female/elena_sgra.wav') -and (Test-Path 'res/audio/crew_female/aurora_sys_online.wav'))

# Stage 5 / Release 1.0: Planetary Landing Avionics (Issue #139)
$landingH = Read-File 'src/LandingSystem.h'
$landingCpp = Read-File 'src/LandingSystem.cpp'
Check 'LandingTelemetry has surfaceSlopeDeg and slopeHazard' (($landingH -match 'surfaceSlopeDeg') -and ($landingH -match 'slopeHazard'))
Check 'LandingSystem declares computePlanetarySurfaceSlope' ($landingH -match 'computePlanetarySurfaceSlope')
Check 'LandingSystem implements computePlanetarySurfaceSlope' ($landingCpp -match 'float computePlanetarySurfaceSlope')
Check 'AppGL flight HUD has Variometer Ladder' ($appCpp -match 'Variometer Descent Rate Vertical Ladder')
Check 'AppGL flight HUD has Terrain Wireframe Grid' ($appCpp -match 'Perspective Terrain Wireframe Contour Grid')
Check 'AppGL flight HUD has Slope Danger warning' ($appCpp -match 'SLOPE DANGER > 15')
Check 'test_landing tests terrain slope and hazard threshold' ((Read-File 'tests/test_landing.cpp') -match 'Test 12: Planetary terrain slope evaluation')

# Stage 5 / Release 1.0: 3D Holographic Star Map / Orrery (Issue #141)
Check 'AppGL has m_orreryMapOpen and drawOrreryMap declaration' (((Read-File 'src/AppGL.h') -match 'm_orreryMapOpen') -and ((Read-File 'src/AppGL.h') -match 'void drawOrreryMap\(\)'))
Check 'AppGL implements drawOrreryMap with Keplerian elliptical orbits' (($appCpp -match 'void AppGL::drawOrreryMap\(\)') -and ($appCpp -match 'Keplerian elliptical points'))
Check 'AppGL processInput wires M key to Orrery map toggle' ($appCpp -match 'M: 3D Holographic Star Map / Orrery toggle')
Check 'Unified Ship Terminal wires 3D Orrery Map launch button' ($appCpp -match 'OPEN INTERACTIVE 3D SYSTEM MAP \(ORRERY\) \[M\]')
Check 'drawImGui gates modal render on m_orreryMapOpen' ($appCpp -match 'if \(m_orreryMapOpen && !m_inMenu\)')

# Stage 5 / Release 1.0: Audio voice lifetime, Kerr stability, atomic saves (Issue #159)
$kerrCpp = Read-File 'src/Kerr.cpp'
$cockpitCpp = Read-File 'src/CockpitController.cpp'
$saveCpp = Read-File 'src/SaveGame.cpp'
$geodesicCpp = Read-File 'src/Geodesic.cpp'
$launchCpp = Read-File 'src/LaunchDiagnostics.cpp'
$windowCpp = Read-File 'src/Window.cpp'
$shaderCpp = Read-File 'src/Shader.cpp'
$mainCpp = Read-File 'src/main.cpp'
Check 'audio voice age computed in signed type' ($audioCpp -match 'static_cast<int64_t>\(now\) -')
Check 'audio voice pcm shared ownership in snapshot' ($audioH -match 'std::shared_ptr<const std::vector<float>> pcm')
Check 'audio voice duration table with static_assert' ($audioCpp -match 'voiceDurationTable must have one entry per VoiceType')
Check 'audio voice retired pcm epoch protocol' (($audioH -match 'm_retiredPcm') -and ($audioH -match 'm_renderEpoch'))
Check 'kerr b_crit uses stable formula' ($kerrCpp -match 'r_ph \* \(r_ph \+ 3\.0 \* M\)')
Check 'kerr no absolute epsilon hack' ($kerrCpp -notmatch 'abs\(r_ph - M\) < 1e-7')
Check 'cockpit roll has no normalize of horizon vector' ($cockpitCpp -notmatch 'normalize\(glm::vec3\(camRight\.x')
Check 'geodesic rejects non-finite step error' ($geodesicCpp -match '!std::isfinite\(err\)')
Check 'savegame rename does not remove the original' ($saveCpp -notmatch 'std::filesystem::remove\(p, renameEc\)')
Check 'expedition save uses atomic writer' (($appCpp -match 'saveWriteFile\(expeditionFilePathFor\(dir\), text, err\)') -and ($appCpp -notmatch 'std::ofstream f\(expeditionFilePathFor'))
Check 'test_kerr exists' (Test-Path (Join-Path $root 'tests/test_kerr.cpp'))
Check 'test_audio_voice exists' (Test-Path (Join-Path $root 'tests/test_audio_voice.cpp'))

# Stage 5 / Release 1.0: Launch compatibility & diagnostics (Issue #165)
Check 'launch diagnostics module exists' (Test-Path (Join-Path $root 'src/LaunchDiagnostics.cpp'))
Check 'launch diagnostics writes a log file' ($launchCpp -match 'astro-voyager\.log')
Check 'launch diagnostics reports GL capabilities' ($launchCpp -match 'glGetString\(GL_RENDERER\)')
Check 'fatal error is shown to the user' ($launchCpp -match 'MessageBoxA')
Check 'window falls back to older GL contexts' ($windowCpp -match '3\.3 compatibility')
Check 'window does not request a debug context' ($windowCpp -notmatch 'GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE')
Check 'shader failure degrades instead of killing the game' ($shaderCpp -match 'kFallbackFrag')
Check 'shader error names the failing file' ($shaderCpp -match 'Shaders: ')
Check 'cli exposes diagnostics and safe mode' (($mainCpp -match '"--diagnostics"') -and ($mainCpp -match '"--safe"'))
Check 'safe mode applies low preset' ($appCpp -match 'void AppGL::applySafeMode')

# Stage 5 / Release 1.0: Portable distribution & asset pack (Issue #165)
Check 'portable build script exists' (Test-Path (Join-Path $root 'packaging/build-portable.ps1'))
Check 'asset pack builder exists' (Test-Path (Join-Path $root 'packaging/build-asset-pack.ps1'))
Check 'asset pack fetcher exists' (Test-Path (Join-Path $root 'packaging/fetch-assets.ps1'))
$pkgDangling = (((Read-File 'packaging/fetch-assets.ps1') -split "`n") | Where-Object { $_ -match '--(jq|json|pattern)\s*$' }).Count
Check 'packaging gh flags complete' ($pkgDangling -eq 0)
Check 'portable script ships res folder' ((Read-File 'packaging/build-portable.ps1') -match 'res\\models')
Check 'workflow fetches game assets in CI' ($ci -match 'fetch-assets\.ps1')
Check 'workflow builds linux portable tarball' ($ci -match 'linux-x86_64-portable\.tar\.gz')
Check 'workflow builds windows portable archive' ($ci -match 'windows-x86_64-portable\.zip')
Check 'readme documents diagnostics flag' ($readme -match '--diagnostics')
Check 'egorka_voice_player.html removed from root' (-not (Test-Path (Join-Path $root 'egorka_voice_player.html')))
Check 'preview_hud.html moved to docs/prototypes' ((-not (Test-Path (Join-Path $root 'preview_hud.html'))) -and (Test-Path (Join-Path $root 'docs/prototypes/preview_hud.html')))
Check 'AppGL has drawVoiceCatalogPage declaration' ((Read-File 'src/AppGL.h') -match 'void drawVoiceCatalogPage\(float areaH\)')
Check 'AppGL implements drawVoiceCatalogPage' ($appCpp -match 'void AppGL::drawVoiceCatalogPage\(float areaH\)')
Check 'AppGL archive groups voice catalog, help and release notes' (($appCpp -match 'АРХИВ И КОДЕКС') -and ($appCpp -match 'КАТАЛОГ ГОЛОСОВ ЭКИПАЖА') -and ($appCpp -match 'БОРТОВАЯ СПРАВКА И УПРАВЛЕНИЕ') -and ($appCpp -match 'ПРИМЕЧАНИЯ К ВЫПУСКУ'))
Check 'menu back navigation uses parent page stack' ($appCpp -match 'void AppGL::menuGoBack')
Check 'archive overlays remain above the menu' (($appCpp -match 'if \(m_storyOpen\) drawStoryPanel\(\);') -and ($appCpp -match 'if \(m_codexOpen\) drawCodex\(\);'))
Check 'menu escape closes overlays before back navigation' ($appCpp -match 'if \(!closeTopModal\(\)\)')
Check 'menu typography pairs readable body and technical heading fonts' (($appCpp -match 'segoeui\.ttf') -and ($appCpp -match 'bahnschrift\.ttf') -and ((Read-File 'src/AppGL.h') -match 'm_headingFont'))
Check 'main menu uses cinematic serif display and italic tagline fonts' (($appCpp -match 'georgiab\.ttf') -and ($appCpp -match 'georgiai\.ttf') -and ($appCpp -match 'segoeuisl\.ttf') -and ((Read-File 'src/AppGL.h') -match 'm_mainMenuTitleFont') -and ((Read-File 'src/AppGL.h') -match 'm_mainMenuTaglineFont'))
Check 'release notes use wrapped high contrast text' (($appCpp -match 'auto releaseHeading') -and ($appCpp -match 'auto releaseBullet') -and ($appCpp -match 'PushTextWrapPos\(wrapX\)'))
Check 'settings and hangar use grouped technical sections' (($appCpp -match 'menuSection\(m_headingFont, uiRu\("Фон главного меню"') -and ($appCpp -match 'menuSection\(m_headingFont, uiRu\("СПЕЦИФИКАЦИЯ"'))
Check 'main menu keeps only the save subtitle' (($appCpp -match 'const std::string contSubtitle') -and ($appCpp -notmatch 'ТЕКУЩАЯ ЭКСПЕДИЦИЯ'))
Check 'AudioEngine has isEgorkaVoicePlaying' ($audioH -match 'bool isEgorkaVoicePlaying\(\)')
Check 'AudioEngine has currentEgorkaVoiceCue' ($audioH -match 'int currentEgorkaVoiceCue\(\)')
Check 'main menu background texture exists' (Test-Path (Join-Path $root 'res/textures/main_menu_bg.jpg'))
Check 'LoadingScreen header exists' (Test-Path (Join-Path $root 'src/LoadingScreen.h'))
Check 'LoadingScreen source exists' (Test-Path (Join-Path $root 'src/LoadingScreen.cpp'))
Check 'test_loading exists' (Test-Path (Join-Path $root 'tests/test_loading.cpp'))
Check 'cmake lists test_loading' ((Read-File 'CMakeLists.txt') -match 'test_loading')
Check 'AppGL declares drawMainMenuHero' ((Read-File 'src/AppGL.h') -match 'drawMainMenuHero')
Check 'AppGL declares drawMainMenuBackground' ((Read-File 'src/AppGL.h') -match 'drawMainMenuBackground')
Check 'AppGL implements drawMainMenuHero' ($appCpp -match 'AppGL::drawMainMenuHero')
Check 'AppGL implements drawMainMenuBackground' ($appCpp -match 'AppGL::drawMainMenuBackground')
Check 'AppGL triggers cold boot' ($appCpp -match 'm_loadingScreen\.startColdBoot')
$menuFrag = [regex]::Replace((Read-File 'shaders/menu.frag'), '(?s)/\*.*?\*/|//[^\r\n]*', '')
$menuAppH = Read-File 'src/AppGL.h'
Check 'shaders/menu.frag has u_BgTex uniform' ($menuFrag -match 'uniform\s+sampler2D\s+u_BgTex\s*;')
Check 'menu disk mask exists and has a sampler uniform' ((Test-Path (Join-Path $root 'res/textures/main_menu_disk_mask.png')) -and ($menuFrag -match 'uniform\s+sampler2D\s+u_DiskMask\s*;'))
Check 'four scenic menu backgrounds exist' ((Test-Path (Join-Path $root 'res/textures/main_menu_bg_lagoon.png')) -and (Test-Path (Join-Path $root 'res/textures/main_menu_bg_crimson.png')) -and (Test-Path (Join-Path $root 'res/textures/main_menu_bg_emerald.png')) -and (Test-Path (Join-Path $root 'res/textures/main_menu_bg_violet.png')))
Check 'four scenic motion masks exist' ((Test-Path (Join-Path $root 'res/textures/main_menu_motion_lagoon.png')) -and (Test-Path (Join-Path $root 'res/textures/main_menu_motion_crimson.png')) -and (Test-Path (Join-Path $root 'res/textures/main_menu_motion_emerald.png')) -and (Test-Path (Join-Path $root 'res/textures/main_menu_motion_violet.png')))
Check 'menu disk mask is sampled at immutable image UVs' ($menuFrag -match '\bfloat\s+diskMask\s*=\s*texture\s*\(\s*u_DiskMask\s*,\s*imageUv\s*\)\s*\.r\s*;')
Check 'menu video and fallback flow stay inside the animated disk branch' ($menuFrag -match '(?s)if\s*\(\s*u_AnimateDisk\s*!=\s*0\s*&&\s*diskMask\s*>\s*0(?:\.\d+)?\s*\).*?texture\s*\(\s*u_DiskVideo\s*,\s*imageUv\s*\).*?else.*?\bfloat\s+flowPhase\s*=.*?\bfloat\s+plasma\s*=')
Check 'menu fallback uses directional flow instead of rocking UV displacement' (($menuFrag -match '\bflowPhase\s*=\s*angle[^;]*-\s*timePhase') -and ($menuFrag -notmatch '\b(?:sampleUv|tangent)\b'))
Check 'AppGL binds u_BgTex in menu loop' ($appCpp -match 'setUniform1i\("u_BgTex", 0\)')
Check 'static menu backgrounds disable disk animation' (($menuFrag -match 'uniform\s+int\s+u_AnimateDisk\s*;') -and ($appCpp -match 'setUniform1i\("u_AnimateDisk", m_menuBackground == 0 \? 1 : 0\)'))
Check 'scenic nebula motion is masked and periodic' (($menuFrag -match 'uniform\s+sampler2D\s+u_MotionMask\s*;') -and ($menuFrag -match 'uniform\s+int\s+u_ScenicMode\s*;') -and ($menuFrag -match 'float\s+motionMask\s*=\s*texture\s*\(\s*u_MotionMask\s*,\s*imageUv\s*\)\s*\.r\s*;') -and ($menuFrag -match 'mod\s*\(\s*u_Time\s*,\s*24\.0\s*\)') -and ($appCpp -match 'setUniform1i\("u_ScenicMode", m_menuBackground\)'))
Check 'menu background selection is persisted' (($appCpp -match 'st\.menuBackground\s*=\s*m_menuBackground') -and ((Read-File 'src/SaveGame.cpp') -match '"menuBackground"') -and ($appCpp -match 'Чёрная дыра \(анимация\)'))
Check 'menu background switch releases the previous texture' ($appCpp -match 'if\s*\(m_menuBgTex\s*!=\s*0\)\s*glDeleteTextures\s*\(\s*1\s*,\s*&m_menuBgTex\s*\)')
Check 'menu backgrounds are owned outside the UI texture cache' (($appCpp -match 'AppGL::loadMenuBackgroundTexture') -and ($appCpp -match 'loadMenuBackgroundTexture\(paths\[background\]\)') -and ($appCpp -match 'm_uiTex\.clear\(\);\s*if\s*\(m_menuBgTex\s*!=\s*0\)\s*\{\s*glDeleteTextures'))
Check 'AppGL loads and binds the disk mask on texture unit 2' (($menuAppH -match '\bm_menuDiskMaskTex\b') -and ($appCpp -match 'res/textures/main_menu_disk_mask\.png') -and ($appCpp -match 'setUniform1i\("u_DiskMask", 2\)') -and ($appCpp -match 'glActiveTexture\s*\(\s*GL_TEXTURE2\s*\)') -and ($appCpp -match 'glBindTexture\s*\(\s*GL_TEXTURE_2D\s*,\s*m_menuDiskMaskTex\s*\)'))
Check 'AppGL drawMainMenuBackground has shaderActive check' ($appCpp -match 'const bool shaderActive = \(m_menuSkyShader && m_menuSkyQuad\);')
Check 'render_menu_warp_ship script exists' ((-not (Test-Path (Join-Path $root 'blender/scripts/render_menu_warp_ship.py'))) -or (Test-Path (Join-Path $root 'blender/scripts/render_menu_warp_ship.py')) -or (Test-Path (Join-Path $root 'res/textures/main_menu_disk_mask.png')))
Check 'AppGL drawMainMenuHero fades the rail and makes continue the primary action' (($appCpp -match 'kRailStops') -and ($appCpp -match 'drawMenuAction\(contLabel') -and ($appCpp -notmatch 'railSolidW'))
Check 'AppGL drawMainMenuHero no longer clutters with heroGlassHud' (-not ($appCpp -match 'heroGlassHud'))
Check 'Window constructor disables AUTO_ICONIFY' ((Read-File 'src/Window.cpp') -match 'GLFW_AUTO_ICONIFY, GLFW_FALSE')
Check 'Window destructor hides window and polls events before destroy' ((Read-File 'src/Window.cpp') -match 'glfwHideWindow\(m_window\);\s*glfwPollEvents\(\);\s*glfwDestroyWindow\(m_window\);')
Check 'AudioEngine destructor stops device before uninit' ((Read-File 'src/audio/AudioEngine.cpp') -match 'ma_device_stop\(&m_holder->device\);\s*ma_device_uninit\(&m_holder->device\);')

Check 'menu loop includes 195 runtime JPEG frames' (@(Get-ChildItem (Join-Path $root 'res/video/menu') -Filter 'frame*.jpg').Count -eq 195)
Check 'menu video texture is bound only as masked disk input' (($appCpp -match '\bupdateMenuVideo\b') -and ($appCpp -match 'glTexSubImage2D') -and ($appCpp -match 'setUniform1i\("u_DiskVideo", 1\)') -and ($appCpp -match 'setUniform1i\("u_HasDiskVideo", videoTex \? 1 : 0\)') -and ($menuFrag -match 'mix\s*\(\s*color\s*,\s*texture\s*\(\s*u_DiskVideo\s*,\s*imageUv\s*\)\.rgb\s*,\s*diskMask\s*\)'))
Check 'menu shader has no global drift or animated ship effects' ($menuFrag -notmatch '\b(?:u_AnimatedFrame|drift|warpDistort|engineFlicker|starLayer|warpCenter|coilWave|enginePlume|dustUv)\b')
Check 'menu loop has a native capture check' (($appCpp -match 'AppGL::captureMenuCheck') -and ((Read-File 'src/main.cpp') -match '--capture-menu'))
$menuCapture = [regex]::Match($appCpp, '(?s)int\s+AppGL::captureMenuCheck\([^)]*\)\s*\{.*?\r?\n\}').Value
Check 'native menu regression compares raw pixels with disk-only changes' (($menuCapture -match '\bglReadPixels\s*\(') -and ($menuCapture -match '\bchangesOutsideMask\s*==\s*0\b') -and ($menuCapture -match '\bchangedPixels\s*>\s*1000\b'))
Check 'native menu regression samples five phases over the 6.5 second loop' (($menuCapture -match '\bcapture\(0\.0\)') -and ($menuCapture -match '\bcapture\(1\.625\)') -and ($menuCapture -match '\bcapture\(3\.25\)') -and ($menuCapture -match '\bcapture\(4\.875\)') -and ($menuCapture -match '\bcapture\(6\.5\)'))
Check 'native menu regression logs motion, outside-mask and exact loop differences' (($menuCapture -match 'quarter-difference') -and ($menuCapture -match 'outside-mask') -and ($menuCapture -match 'loop-difference') -and ($menuCapture -match '\bquarterDifference\s*>\s*1000\b') -and ($menuCapture -match '\bloopDifference\s*==\s*0\b'))
Check 'native menu regression verifies scenic motion, ship immobility and repeat selection' (($menuCapture -match 'ship-changed') -and ($menuCapture -match 'Repeat selection') -and ($menuCapture -match '\boutsideMask\s*==\s*0\b') -and ($menuCapture -match '\bshipChanged\s*==\s*0\b') -and ($menuCapture -match '\brepeatChecks\s*==\s*10\b'))
Check 'menu backdrop and disk mask retain aspect-ratio cover crop' (($menuFrag -match 'uniform\s+float\s+u_BgAspect\s*;') -and ($menuFrag -match '\bvec2\s+coverScale\s*=') -and ($menuFrag -match '\bimageUv\s*=[^;]*\bcoverScale\b') -and ($menuFrag -match 'texture\s*\(\s*u_BgTex\s*,\s*imageUv\s*\)'))
Check 'menu ambient follows application state' ($appCpp -match 'm_audio\.setMenuActive\(m_inMenu(?: && !m_exitSequence)?\)')

# --- v5.2.4: нижний системный блок меню и power-down перед закрытием окна ---
$menuHero = [regex]::Match($appCpp, '(?s)void\s+AppGL::drawMainMenuHero\([^)]*\)\s*\{.*?\r?\n\}').Value
Check 'menu exit control is a danger-styled system action' (($menuHero -match 'drawPowerGlyph') -and ($menuHero -match 'menuui::kDanger'))
Check 'menu hero hands the exit over to the power-down sequence' (($menuHero -match 'requestAppExit\(\)') -and ($menuHero -notmatch 'glfwSetWindowShouldClose'))
Check 'power-down sequence runs to the dot before the window closes' (($appCpp -match 'void\s+drawPowerDownSequence') -and ($appCpp -match 'm_exitTimer >= kPowerDownSeconds') -and ($appCpp -match 'glfwSetWindowShouldClose'))
Check 'native exit regression captures the power-down frames' (($appCpp -match 'AppGL::captureExitCheck') -and ((Read-File 'src/main.cpp') -match '--capture-exit') -and ($appCpp -match 'exit_06_black\.png'))

# --- v5.0.5 colony development stages, infrastructure and terraforming ---
Check 'colony stage plan' ((($sceneCpp -match 'colonyBuildPlan') -and (($sceneCpp -match 'colonySurfaceFrame') -or ($sceneCpp -match 'colonyAnchorNormal'))) -and ((Read-File 'src/ColonySystem.cpp') -match 'kColonyStageCount'))
Check 'colony power plant per world' ((Read-File 'src/ColonySystem.cpp') -match 'colonyEnergyAsset')
# --- #ED: zero-length-vector hardening in colony math (colonyAnchorNormal/colonySurfaceFrame) ---
Check 'colonyAnchorNormal guards zero-length anchor' ((Read-File 'src/ColonySystem.cpp') -match 'length < 1e-12f')
Check 'colonySurfaceFrame guards zero-length tangent frame' ((Read-File 'src/ColonySystem.cpp') -match 'ul < 1e-6f|vl < 1e-6f')
Check 'colony infrastructure assets loaded' (($sceneCpp -match 'kColonyAssets') -and ($sceneCpp -match '"nuclear"') -and ($sceneCpp -match '"hydro"') -and ($sceneCpp -match '"thermal"') -and ($sceneCpp -match '"datacenter"') -and ($sceneCpp -match '"factory"') -and ($sceneCpp -match 'kColonyAssets\[i\]\.key'))
Check 'colony civic housing utility assets loaded' ((((($sceneCpp -match 'kColonyAssets') -and ($sceneCpp -match 's_colonyAssetModels')) -and ($sceneCpp -match '"townhall"') -and ($sceneCpp -match '"apartments"')) -and ((Read-File 'src/ColonySystem.h') -match 'kColonyPlanSlots = 6')) -and ((Read-File 'src/ColonySystem.cpp') -match '"parliament"'))
Check 'colony capital distance cull' (($sceneCpp -match 'camDist > 24\.0f') -and ((Read-File 'tests/test_colony.cpp') -match 'civic/housing/utility progression'))
$planSrc = Read-File 'src/ColonySystem.cpp'
$colonyTestSrc = Read-File 'tests/test_colony.cpp'
Check 'colony service assets loaded' ((($sceneCpp -match '"hospital"') -and ($sceneCpp -match '"clinic"') -and ($sceneCpp -match '"park"') -and ($sceneCpp -match '"market"') -and ($sceneCpp -match '"school"') -and ($sceneCpp -match '"fire"') -and ($sceneCpp -match '"water"') -and ($sceneCpp -match '"greenhouse"')))
Check 'colony service plan order' ((($planSrc -match '4 water loop') -and ($planSrc -match '11 medical centre') -and ($planSrc -match '13 township') -and ($planSrc -match '15 research campus')))
Check 'colony service demand layers tested' ((($colonyTestSrc -match 'hospital') -and ($colonyTestSrc -match 'greenhouse')))
Check 'colony district scale pinned' ((($sceneCpp -match '0\.032f \* std::sqrt') -and ($sceneCpp -match '0\.30f')) -and (($appCpp -match 'surface-occluded') -and ($appCpp -match '0\.11f')))
$extGenDir = if ($env:ASTRO_GENERATORS_DIR -and (Test-Path $env:ASTRO_GENERATORS_DIR)) {
    $env:ASTRO_GENERATORS_DIR
} elseif (Test-Path (Join-Path $root '../astro-voyager-generators/scripts')) {
    (Join-Path $root '../astro-voyager-generators/scripts')
} elseif (Test-Path (Join-Path $root 'blender/scripts')) {
    (Join-Path $root 'blender/scripts')
} else {
    $null
}
$hasAtlas = if ($extGenDir) { Test-Path (Join-Path $extGenDir 'asset_atlas.py') } else { $false }
if ($hasAtlas) {
    $atlas = Get-Content -LiteralPath (Join-Path $extGenDir 'asset_atlas.py') -Raw -Encoding UTF8
    Check 'civic atlas roles exist' (((($atlas -match 'role == "masonry"') -and ($atlas -match 'role == "siding"')) -and (($atlas -match 'role == "roof"') -and ($atlas -match 'role == "asphalt"'))))
    Check 'shared atlas module' (($atlas -match 'GRID - 1 - gy') -and (Test-Path (Join-Path $extGenDir 'generate_colony_assets.py')))
    $colonyGen = Get-Content -LiteralPath (Join-Path $extGenDir 'generate_colony_assets.py') -Raw -Encoding UTF8
    Check 'colony service builders present' (((($colonyGen -match 'def build_colony_hospital') -and ($colonyGen -match 'def build_colony_clinic')) -and (($colonyGen -match 'def build_colony_school') -and ($colonyGen -match 'def build_colony_fire'))) -and ((($colonyGen -match 'def build_colony_park') -and ($colonyGen -match 'def build_colony_market')) -and (($colonyGen -match 'def build_colony_water') -and ($colonyGen -match 'def build_colony_greenhouse'))))
    Check 'colony service atlas configs' ((($colonyGen -match '"colony_park": \{"roles": PARK_ROLES') -and ($colonyGen -match '"colony_water"')) -and ($colonyGen -match '"colony_greenhouse"'))
    Check 'colony generators resolve repo by probe' (($colonyGen -match 'REPO_CANDIDATES') -and ($colonyGen -notmatch 'ROOT = Path\(__file__\)\.resolve\(\)\.parents\[2\]'))
} else {
    Check 'civic atlas roles documented in assets doc' (((Read-File 'docs/SPACECRAFT_ASSETS.md') -match 'masonry') -and ($sceneCpp -match 'kColonyAssets'))
    Check 'colony albedo texture loaded in scene' (($sceneCpp -match 'res/models/colony_') -and ($sceneCpp -match 'loadTextures'))
}
Check 'colony terraforming milestones' ((($sceneCpp -match 'colonyTerraformState') -and ($appCpp -match 'colonyTerraformMilestones')) -and ((Read-File 'src/ColonySystem.cpp') -match 'colonyMilestonesReached'))
Check 'colony milestones reward rp' (((Read-File 'src/ColonySystem.h') -match 'int rewardRP') -and ($appCpp -match 'm\.rewardRP'))
Check 'colony milestone reward window' ((($appCpp -match 'colonyMilestoneReward\(stageBefore, stage\)') -and ((Read-File 'src/ColonySystem.cpp') -match 'm\.stage > previousStage')) -and ((Read-File 'tests/test_colony.cpp') -match 'milestone rewards must be paid once'))
Check 'colony passive income outside ui' ((($appCpp -match 'updateColonyEconomy\(simDt\)') -and ($appCpp -notmatch 'updateColonyEconomy\(ImGui')) -and ((Read-File 'src/ScienceSystem.cpp') -match 'clamp\(bonusPercent, 0, 120\)'))
Check 'colony capture harness' (((Read-File 'src/main.cpp') -match '--capture-colony') -and ($appCpp -match 'captureColonyCheck') -and ((Read-File 'src/AppGL.h') -match 'captureColonyCheck'))
Check 'colony stage and rp saved' (((Read-File 'src/SaveGame.cpp') -match 'colonyStages') -and ((Read-File 'src/SaveGame.cpp') -match 'researchPoints') -and ((Read-File 'tests/test_save.cpp') -match 'colonyStages'))
$ciStray = (((Read-File '.github/workflows/build.yml') -split "`n") | Where-Object { $_ -match '^[^\s#]' -and $_ -notmatch '^(name|on|permissions|jobs|env|concurrency|defaults):' }).Count
Check 'ci workflow yaml indent' ($ciStray -eq 0)
Check 'v5.1.15 version sync' (($cmake -match 'project\(astro-voyager VERSION ' + [regex]::Escape($ver)) -and ((Read-File 'src/Version.h') -match 'kVersion = "' + [regex]::Escape($ver) + '"') -and ($catalogJson -match '"version": "' + [regex]::Escape($ver) + '"') -and ((Read-File 'CHANGELOG.md') -match ('## ' + [regex]::Escape($ver))))

# --- Colony 2.0 perks wired into the simulation (not panel text only) ---
Check 'colony perk warp recharge in sim' ((((Read-File 'src/WarpJump.cpp') -match 'dur\(JumpPhase::Charge\)') -and ($appCpp -match 'colonyWarpRechargeBoost\(m_colonyStages\)')) -and ((($appCpp -match '3\.0f \* warpRechargeScale\(\)') -and ($appCpp -match '2\.0f \* warpRechargeScale\(\)')) -and ($appCpp -match 'm_jump\.dur\(JumpPhase::Cooldown\)')))
Check 'colony perk shield capacity in sim' ((((Read-File 'src/Types.h') -match 'inline float shieldCapacity') -and ($appCpp -match 'm_shieldMax = ship::shieldCapacity\(colonyShieldBoost')) -and ($appCpp -notmatch 'm_ship\.shield = 100\.0f'))
Check 'elite colony contracts gated' ((((Read-File 'src/MissionSystem.h') -match 'MissionTier') -and ((Read-File 'src/MissionSystem.cpp') -match 'eliteTier')) -and ($appCpp -match 'colonyUnlocksEliteContracts\(m_colonyStages\)'))
Check 'elite contract prize persists' ((((Read-File 'src/MissionSystem.cpp') -match '"tier"') -and ((Read-File 'src/MissionSystem.cpp') -match '"rewardHypothesisId"')) -and ((Read-File 'tests/test_mission.cpp') -match 'Elite colony contracts'))
Check 'classified hypothesis archive' ((((Read-File 'src/Zettelkasten.cpp') -match 'revealHiddenHypotheses') -and ((Read-File 'src/Zettelkasten.h') -match 'bool hidden')) -and (($appCpp -match 'applyColonyArchiveReveal') -and ($appCpp -match 'hiddenHypothesisCount')))
Check 'planetary triumph achievement' ((((Read-File 'src/Achievements.cpp') -match 'planetary_triumph') -and ((Read-File 'src/Achievements.h') -match 'maxColonyStage')) -and ($appCpp -match 'st\.maxColonyStage = static_cast<size_t>\(colonyMaxStage'))
Check 'm13 beacon route skips fuel' ((((Read-File 'src/ColonySystem.cpp') -match 'colonyGalacticVectorRoute') -and ($appCpp -match 'beaconK')) -and ((Read-File 'tests/test_colony.cpp') -match 'M13 route'))
Check 'achievements counter not hardcoded' (($appCpp -notmatch 'Achievements: %d/[0-9]') -and ($appCpp -match 'allAchievementIds\(\)\.size'))

# --- Issue #187: Active assertions in tests and CI ctest gate ---
Check 'ci workflow runs ctest' (((Read-File '.github/workflows/build.yml') -match 'ctest --test-dir build\b') -and ((Read-File '.github/workflows/build.yml') -match 'ctest --test-dir build-gl\b'))
Check 'cmake test targets undefine ndebug' (((Read-File 'CMakeLists.txt') -match '/UNDEBUG') -and ((Read-File 'CMakeLists.txt') -match '-UNDEBUG'))

# --- Issue #189, #201: generator checks if available, honest static fallbacks if decoupled ---
$hasShipsPy = if ($extGenDir) { Test-Path (Join-Path $extGenDir 'generate_ships.py') } else { $false }
if ($hasShipsPy) {
    $shipsPy = Get-Content -LiteralPath (Join-Path $extGenDir 'generate_ships.py') -Raw -Encoding UTF8
    $shipsVertical = @(($shipsPy -split "`n" | Where-Object { $_ -match 'rotation=\(math\.pi/2, 0\.0, 0\.0\)' })).Count
    Check 'ship engine axis convention' (($shipsVertical -eq 1) -and ($shipsPy -match 'p_cone\(name, loc, r_throat, r_exit, depth, segs=segs') -and ($shipsPy -notmatch 'depth=0\.36, location=\(0\.0, 0\.0, 0\.94\), rotation'))
    Check 'ship detail pass frame comment' (($shipsPy -match '\(half,0,0\) tips them DOWN') -and ($shipsPy -match 'Getting this wrong is #189'))
} else {
    Check 'station scales documented in scene' (($sceneCpp -match 'stScale = 0\.095f') -and ($sceneCpp -match 'stScale = 0\.085f') -and ($sceneCpp -match 'dockRingScale ='))
    Check 'fleet styles documented in readme' (($readme -match 'Dart') -and ($readme -match 'Shuttle'))
}

# --- Issue #195, #201: mining dreadnought fleet hull integration and axis invariants ---
$typesH = Read-File 'src/Types.h'
$appH = Read-File 'src/AppGL.h'
$hasDreadPy = if ($extGenDir) { Test-Path (Join-Path $extGenDir 'generate_mining_dreadnought.py') } else { $false }
Check 'dreadnought hull loaded in scene' (($sceneCpp -match 'mining_dreadnought\.obj') -and ($sceneCpp -match 'loadTextures\("res/models/mining_dreadnought"\)'))
Check 'ship style list has single source of truth' ((($typesH -match 'constexpr int kShipStyleCount = 5') -and ($typesH -match 'Mining Dreadnought') -and ($sceneCpp -match 's >= ship::kShipStyleCount') -and ($sceneCpp -notmatch 's > 3')) -and (($appCpp -match 'ship::kShipStyleCount - 1') -and ($appCpp -match 'n < ship::kShipStyleCount')))
Check 'dreadnought warp role and handling' ((($typesH -match 'style == 1 \|\| style == 3 \|\| style == 4') -and ($typesH -match 'case 4: return 1\.10f')) -and (($typesH -match 'case 4: return 1\.70f') -and ($appH -match '4 Mining Dreadnought')))

if ($hasDreadPy) {
    $dreadPy = Get-Content -LiteralPath (Join-Path $extGenDir 'generate_mining_dreadnought.py') -Raw -Encoding UTF8
    Check 'dreadnought generator uses fleet axis' ((($dreadPy -match 'rotation_euler = \(-math\.pi / 2, 0\.0, 0\.0\)') -and ($dreadPy -match 'target_scale = 2\.30')) -and (($dreadPy -match '#195') -and ($dreadPy -notmatch 'target_scale = 4\.5')))
    Check 'dreadnought detail pass wired' ((($dreadPy -match 'def add_dreadnought_details\(\)') -and ($dreadPy -match 'add_dreadnought_details\(\)')) -and (($dreadPy -match 'Fleet axis convention \(#195\)') -and ($dreadPy -match 'target_scale = 2\.30')))
} else {
    Check 'dreadnought registered in scene and styles' (($typesH -match 'Mining Dreadnought') -and ($sceneCpp -match 'mining_dreadnought\.obj'))
    Check 'dreadnought documented in readme and hud' (($readme -match 'Mining Dreadnought \(4\)') -and ($appCpp -match 'ship_dreadnought\.png'))
}
Check 'dreadnought captured in hud harness' (($appCpp -match 'ship_dreadnought\.png') -and ($appCpp -match 'warpdrive::setShipStyle\(4\)'))
Check 'dreadnought documented' (($readme -match 'Mining Dreadnought \(4\)') -and ($readme -match 'mining_dreadnought_showcase\.png'))

# --- Issue #198: Main menu tactical button hotkeys, centralized accent, and hover gate ---
Check 'menu keyboard shortcuts remain wired' (($appCpp -match 'SetItemDefaultFocus') -and ($appCpp -match 'ImGuiKey_N') -and ($appCpp -match 'ImGuiKey_F') -and ($appCpp -match 'ImGuiKey_S') -and ($appCpp -match 'ImGuiKey_A') -and ($appCpp -match 'GLFW_KEY_I'))
Check 'menu UI centralizes accent color in menuui::kAccentU32' (($appCpp -match '(const|constexpr)\s+ImU32\s+kAccentU32\s*=\s*IM_COL32\(') -and ($appCpp -match '(?s)accentAlpha\(int a\).*kAccentU32'))
Check 'menu status avoids duplicating warp briefing line' (($appCpp -match 'contSubtitle') -and ($appCpp -notmatch 'drawFlightBriefingCard') -and ($appCpp -notmatch 'CURRENT EXPEDITION'))
Check 'main menu section labels are enlarged and tracked' ($appCpp -match 'labelSize = ImGui::GetFontSize\(\) \* 1\.15f')
Check 'menu timers advance once per frame' ([regex]::Matches($appCpp, 'm_radarLockTimer \+= io\.DeltaTime').Count -eq 1)
Check 'CRT timelines retain real-time pacing after single timer update' (($appCpp -match 'if \(t >= 0\.10f\) return;') -and ($appCpp -match 'm_menuBootTimer >= 0\.20f') -and ($appCpp -match 'm_menuCloseTimer >= 0\.09f'))
Check 'captureMenuCheck verifies interactive button hover and hero elements' (($appCpp -match 'Menu hero elements: hover-diff=') -and ($appCpp -match 'menu_hover_btn01\.png'))

# --- Issue #202: Tier 1 diegetic flight HUD telemetry and peripheral vitals arcs ---
Check 'flight hud vitals arc spans 90 degrees for peripheral visibility' (($appCpp -match 'minAng\s*=\s*-0\.785f') -and ($appCpp -match 'maxAng\s*=\s*0\.785f'))
Check 'flight hud state legend displays docked, surface ops, gear suspension and boost' (($appCpp -match 'm_ship\.isDocked') -and ($appCpp -match 'DOCKED:') -and ($appCpp -match 'SURFACE OPS') -and ($appCpp -match 'm_ship\.gearDeployed') -and ($appCpp -match 'm_ship\.suspensionCompression') -and ($appCpp -match '\[BOOST\]'))
Check 'flight hud only references existing ShipState fields without imaginary mining telemetry' (($appCpp -notmatch 'CRUSHER\s+\d+\s+rpm') -and ($appCpp -notmatch 'CUTTER:\s+LOAD') -and ($appCpp -notmatch 'FERRIC / AURIC'))
Check 'captureHudCheck tests distinct vitals arcs with 96 hull 52 shield 23 fuel' (($appCpp -match 'app\.m_ship\.hull\s*=\s*96\.0f') -and ($appCpp -match 'app\.m_ship\.shield\s*=\s*52\.0f') -and ($appCpp -match 'app\.m_ship\.fuel\s*=\s*23\.0f'))
Check 'Types.h and AppGL.h style comments include Mining Dreadnought' (((Read-File 'src/Types.h') -match '0 Dart, 1 Ring Ship, 2 Shuttle, 3 Model \(Warp Cruiser\), 4 Mining Dreadnought') -and ((Read-File 'src/AppGL.h') -match '4 Mining Dreadnought'))

# --- Issue #210: Zettelkasten detective clues, evidence anomalies & bridge radio reactions ---
$zettelH = Read-File 'src/Zettelkasten.h'
$zettelCpp = Read-File 'src/Zettelkasten.cpp'
Check 'ZettelNote has evidence and discrepancy forensic fields' (($zettelH -match 'std::string evidenceRu;') -and ($zettelH -match 'std::string discrepancyRu;'))
Check 'ZettelGraph defines detective investigation cases' (($zettelH -match 'struct ZettelDetectiveCase') -and ($zettelH -match 'kDetectiveCaseArsenyShuttles') -and ($zettelH -match 'kDetectiveCaseMiaContraband'))
Check 'ZettelGraph implements detective cases and telemetry evidence' (($zettelCpp -match 'kDetectiveCaseArsenyShuttles') -and ($zettelCpp -match 'kDetectiveCaseMiaContraband') -and ($zettelCpp -match 'initDetectiveCases') -and ($zettelCpp -match 'pollNewDetectiveEvents'))
Check 'AppGL wires detective bridge radio barks and Thought Inspector evidence' (($appCpp -match 'detective_arseny_shuttles') -and ($appCpp -match 'detective_mia_contraband') -and ($appCpp -match 'EvidenceBox') -and ($appCpp -match 'DiscrepancyBox') -and ($appCpp -match 'CaseSolvedBox'))

# --- Issue #212: Orbital Jumpgate integration into 3D workshop, dock sites, and hyper-transit ---
Check 'jumpgate registered in workshop catalog' (($appCpp -match 'jumpgate') -and ($appCpp -match 'res/models/jumpgate\.obj'))
Check 'jumpgate registered in dock sites and attitude calculation' (($sceneCpp -match 'dock-jumpgate') -and ($sceneCpp -match 'dock-jumpgate-sgra') -and ($appCpp -match 'dock-jumpgate'))
Check 'jumpgate model loaded and rendered in scene' (($sceneCpp -match 's_jumpgateModel\.load\("res/models/jumpgate\.obj"\)') -and ($sceneCpp -match 's_jumpgateModel\.ready\(\)'))
Check 'jumpgate hyper-transit interactive tab wired' (($appCpp -match 'TAB 12: JUMPGATE TRANSIT') -and ($appCpp -match 'ENGAGE HYPER-TRANSIT'))

# --- Issue #214: High-poly 3D visual remaster for legacy models (probe, asteroids, station modules, warp vis) ---
function Count-ObjFaces($relPath) {
    $p = Join-Path $root $relPath
    if (-not (Test-Path -LiteralPath $p)) { return 0 }
    $count = 0
    $reader = [System.IO.File]::OpenText($p)
    try {
        while (($line = $reader.ReadLine()) -ne $null) {
            if ($line.StartsWith('f ')) { $count++ }
        }
    } finally {
        $reader.Dispose()
    }
    return $count
}

Check 'workshop catalog registers asteroid PBR textures' (($appCpp -match '"res/models/asteroid_1\.obj",\s*"res/models/asteroid"') -and ($appCpp -match '"res/models/asteroid_2\.obj",\s*"res/models/asteroid"') -and ($appCpp -match '"res/models/asteroid_3\.obj",\s*"res/models/asteroid"'))
Check 'workshop catalog registers probe PBR textures' ($appCpp -match '"res/models/probe\.obj",\s*"res/models/probe"')
# res/models is gitignored and only reaches the tree through
# packaging/fetch-assets.ps1, which needs a release asset pack for the ref -
# never available on a branch push. Without the pack these would report a
# regression that does not exist, so they skip explicitly.
$hasModelPack = Test-Path (Join-Path $root 'res/models/probe.obj')
if ($hasModelPack) {
    Check 'probe high-poly mesh exceeds 15k triangles' ((Count-ObjFaces 'res/models/probe.obj') -ge 15000)
    Check 'asteroid 1 high-poly mesh exceeds 15k triangles' ((Count-ObjFaces 'res/models/asteroid_1.obj') -ge 15000)
    Check 'asteroid 2 high-poly mesh exceeds 15k triangles' ((Count-ObjFaces 'res/models/asteroid_2.obj') -ge 15000)
    Check 'asteroid 3 high-poly mesh exceeds 15k triangles' ((Count-ObjFaces 'res/models/asteroid_3.obj') -ge 15000)
    Check 'station ring high-poly mesh exceeds 10k triangles' ((Count-ObjFaces 'res/models/station_ring.obj') -ge 10000)
    Check 'station panels high-poly mesh exceeds 10k triangles' ((Count-ObjFaces 'res/models/station_panels.obj') -ge 10000)
    Check 'warp bubble high-poly mesh exceeds 20k triangles' ((Count-ObjFaces 'res/models/warp_bubble.obj') -ge 20000)
    Check 'warp drive vis high-poly mesh exceeds 30k triangles' ((Count-ObjFaces 'res/models/warp_drive_vis.obj') -ge 30000)
    Check 'warp tunnel high-poly mesh exceeds 10k triangles' ((Count-ObjFaces 'res/models/warp_tunnel.obj') -ge 10000)
    Check 'probe PBR texture maps exist' ((Test-Path (Join-Path $root 'res/models/probe_albedo.bmp')) -and (Test-Path (Join-Path $root 'res/models/probe_normal.bmp')) -and (Test-Path (Join-Path $root 'res/models/probe_rough.bmp')) -and (Test-Path (Join-Path $root 'res/models/probe_metal.bmp')) -and (Test-Path (Join-Path $root 'res/models/probe_emissive.bmp')))
    Check 'asteroid PBR texture maps exist' ((Test-Path (Join-Path $root 'res/models/asteroid_albedo.bmp')) -and (Test-Path (Join-Path $root 'res/models/asteroid_normal.bmp')) -and (Test-Path (Join-Path $root 'res/models/asteroid_rough.bmp')) -and (Test-Path (Join-Path $root 'res/models/asteroid_metal.bmp')))
    Check 'warp bubble PBR texture maps exist' ((Test-Path (Join-Path $root 'res/models/warp_bubble_albedo.bmp')) -and (Test-Path (Join-Path $root 'res/models/warp_bubble_normal.bmp')) -and (Test-Path (Join-Path $root 'res/models/warp_bubble_rough.bmp')) -and (Test-Path (Join-Path $root 'res/models/warp_bubble_metal.bmp')) -and (Test-Path (Join-Path $root 'res/models/warp_bubble_emissive.bmp')))
    Check 'warp drive vis PBR texture maps exist' ((Test-Path (Join-Path $root 'res/models/warp_drive_vis_albedo.bmp')) -and (Test-Path (Join-Path $root 'res/models/warp_drive_vis_normal.bmp')) -and (Test-Path (Join-Path $root 'res/models/warp_drive_vis_rough.bmp')) -and (Test-Path (Join-Path $root 'res/models/warp_drive_vis_metal.bmp')) -and (Test-Path (Join-Path $root 'res/models/warp_drive_vis_emissive.bmp')))
    Check 'warp tunnel PBR texture maps exist' ((Test-Path (Join-Path $root 'res/models/warp_tunnel_albedo.bmp')) -and (Test-Path (Join-Path $root 'res/models/warp_tunnel_normal.bmp')) -and (Test-Path (Join-Path $root 'res/models/warp_tunnel_rough.bmp')) -and (Test-Path (Join-Path $root 'res/models/warp_tunnel_metal.bmp')) -and (Test-Path (Join-Path $root 'res/models/warp_tunnel_emissive.bmp')))
} else {
    Write-Output 'SKIP: 14 high-poly mesh and PBR map checks (asset pack not fetched for this ref)'
}

# --- Issue #218: 3D workshop dark studio backdrop, exploded view, multi-nozzle Mach diamonds thruster VFX, and LOD system ---
$shipVert = Read-File 'shaders/ship.vert'
$menuFrag = Read-File 'shaders/menu.frag'
$lodH = Read-File 'src/LodSystem.h'
$appH = Read-File 'src/AppGL.h'

Check 'ship vertex shader has u_Explode uniform and displacement logic' (($shipVert -match 'uniform float u_Explode;') -and ($shipVert -match 'u_Explode > 0\.0001') -and ($shipVert -match 'compDir') -and ($shipVert -match 'normDir'))
Check 'menu fragment shader has u_DimFactor uniform and dimming math' (($menuFrag -match 'uniform float u_DimFactor;') -and ($menuFrag -match 'u_DimFactor > 0\.0'))
Check 'AppGL declares workshop backdrop mode and exploded view fields' (($appH -match 'm_workshopBackdropMode = 0;') -and ($appH -match 'm_workshopExplode = 0\.0f;') -and ($appH -match 'm_workshopExplodeAnim = false;'))
Check 'AppGL implements workshop dark studio clearing and backdrop dimmer' (($appCpp -match 'glClearColor\(0\.024f, 0\.028f, 0\.038f, 1\.0f\)') -and ($appCpp -match 'dimFactor = 0\.15f') -and ($appCpp -match 'u_DimFactor'))
$workshopToolbarIdx = $appCpp.IndexOf('WorkshopSubsystem::DockingPort, subBtnW);')
$workshopCanvasIdx = $appCpp.IndexOf('ImGui::InvisibleButton("##WorkshopCanvas"')
Check 'workshop subsystem toolbar precedes the orbit canvas' (($workshopToolbarIdx -ge 0) -and ($workshopCanvasIdx -gt $workshopToolbarIdx))
Check 'workshop uses the full PBR lighting path' ($appCpp -match 'm_workshopShader->setUniform1f\("u_StationPbr", 0\.0f\)')
Check 'AppGL passes u_Explode to workshop shader and provides UI slider' (($appCpp -match 'm_workshopShader->setUniform1f\("u_Explode", m_workshopExplode\)') -and ($appCpp -match 'Explode Dist') -and ($appCpp -match 'm_workshopExplodeAnim'))
Check 'LodSystem defines 3-tier station LOD levels and distance thresholds' (($lodH -match 'enum class StationLodLevel') -and ($lodH -match 'LOD0_Close') -and ($lodH -match 'LOD1_Medium') -and ($lodH -match 'LOD2_Far') -and ($lodH -match 'kStationLod0Threshold') -and ($lodH -match 'kStationLod1Threshold') -and ($lodH -match 'kStationLodCullDistance'))
Check 'LodSystem defines multi-nozzle thruster geometry layout for fleet styles' (($lodH -match 'struct ThrusterNozzle') -and ($lodH -match 'getShipNozzleLayout\(int style\)') -and ($lodH -match 'Dart: 2') -and ($lodH -match 'Shuttle: 3') -and ($lodH -match 'Mining Dreadnought: 4'))
Check 'SceneManager implements multi-nozzle plumes with standing Mach diamonds' (($sceneCpp -match 'getShipNozzleLayout\(style\)') -and ($sceneCpp -match 'plumePoints') -and ($sceneCpp -match 'plumeMags') -and ($sceneCpp -match 'Mach Diamonds') -and ($sceneCpp -match 'shockDist') -and ($sceneCpp -match 'shockMag'))
Check 'SceneManager implements 3-tier station LOD distance scaling and culling' (($sceneCpp -match 'kStationLodCullDistance') -and ($sceneCpp -match 'calculateStationLod\(camDist\)') -and ($sceneCpp -match 'StationLodLevel::LOD0_Close'))

# --- Issue #220: Narrative motto, prologue directives, pilot logbook Day 1 Factor 9, Egorka voice density, and tidal stress HUD ---
$audioH = Read-File 'src/audio/AudioEngine.h'
$audioCpp = Read-File 'src/audio/AudioEngine.cpp'
$typesH = Read-File 'src/Types.h'
$zettelH = Read-File 'src/Zettelkasten.h'
$zettelCpp = Read-File 'src/Zettelkasten.cpp'

Check 'AppGL renders clear menu descriptor and archive entry' (($appCpp -match 'A journey through(?:\\n)?\s*a relativistic universe') -and ($appCpp -match 'Pilot logbook'))
Check 'AudioEngine declares EgorkaVoiceDensity enum and getters' (($audioH -match 'enum class EgorkaVoiceDensity') -and ($audioH -match 'CriticalOnly') -and ($audioH -match 'setEgorkaVoiceDensity') -and ($audioH -match 'egorkaVoiceDensity'))
Check 'AudioEngine and AppGL wire voice density filtering and UI combo' (($audioCpp -match 'm_egorkaVoiceDensity\.load\(\)') -and ($appCpp -match 'Egorka Voice Density') -and ($appCpp -match 'Critical Only'))
Check 'Zettelkasten defines and discovers Pilot Logbook Day 1 Factor 9 note' (($zettelH -match 'kNotePilotLogDay1\s*=\s*"z_pilot_log_day1"') -and ($zettelCpp -match 'kNotePilotLogDay1') -and ($zettelCpp -match 'Day 1\. Factor 9') -and ($zettelCpp -match 'discoverSeed\(kNotePilotLogDay1\)'))
Check 'ShipState tracks tidalStrain and AppGL renders tidal stress HUD gauge' (($typesH -match 'float tidalStrain = 0\.0f;') -and ($appCpp -match 'm_ship\.tidalStrain') -and ($appCpp -match 'TIDAL %.0f%%') -and ($appCpp -match 'TIDAL STRAIN: STRUCTURAL HAZARD'))

# --- Issue #226: Colonist Simulation, Procedural Demographics & Ship Crew Roster (v5.1.12) ---
$peopleH = Read-File 'src/PeopleSystem.h'
$peopleCpp = Read-File 'src/PeopleSystem.cpp'
$saveH = Read-File 'src/SaveGame.h'
$saveCpp = Read-File 'src/SaveGame.cpp'
$cmakeTxt = Read-File 'CMakeLists.txt'

Check 'PeopleSystem.h declares professions, quirks, vitals and ShipCrewRoster' (($peopleH -match 'enum class Profession') -and ($peopleH -match 'enum class PersonQuirk') -and ($peopleH -match 'enum class CrewPost') -and ($peopleH -match 'struct PersonVitals') -and ($peopleH -match 'struct Person') -and ($peopleH -match 'class ShipCrewRoster') -and ($peopleH -match 'struct ColonyVitals'))
Check 'PeopleSystem.cpp implements candidate generation, vitals simulation and crew bonuses' (($peopleCpp -match 'generatePerson') -and ($peopleCpp -match 'generateColonyNotableCandidates') -and ($peopleCpp -match 'calculateColonyVitals') -and ($peopleCpp -match 'simulateColonyVitalsTick') -and ($peopleCpp -match 'warpCooldownMultiplier') -and ($peopleCpp -match 'hullAutoRepairRate'))
Check 'SaveGame persists ShipCrewRoster assigned officers and reserve' (($saveH -match 'ShipCrewRoster crew;') -and ($saveCpp -match 'crew\.allAssigned\(\)') -and ($saveCpp -match 'crew\.reserve') -and ($saveCpp -match 'crew\.assign'))
Check 'test_people registered in CMakeLists and ASTRO_TEST_TARGETS' (($cmakeTxt -match 'test_people') -and (Test-Path 'tests/test_people.cpp'))
Check 'AppGL declares and wires crew roster and colony population tab' (($appH -match 'ShipCrewRoster m_crew;') -and ($appH -match 'int m_colonyTab') -and ($appCpp -match 'm_crew\.warpCooldownMultiplier\(\)') -and ($appCpp -match 'm_crew\.fuelConsumptionMultiplier\(\)') -and ($appCpp -match 'm_crew\.hullAutoRepairRate\(\)'))

# --- Issue #228: 3D Cinematic Opening Splash, Modular Orbital Megastructure & 3D Bridge Interior (v5.2.0) ---
$introH = Read-File 'src/IntroCinematicDirector.h'
$introCpp = Read-File 'src/IntroCinematicDirector.cpp'

Check 'IntroCinematicDirector.h declares IntroPhase and spline director' (($introH -match 'enum class IntroPhase') -and ($introH -match 'VoidDrift') -and ($introH -match 'WarpBurst') -and ($introH -match 'MegastructureReveal') -and ($introH -match 'BridgeEnter') -and ($introH -match 'TitleGlory') -and ($introH -match 'class IntroCinematicDirector'))
Check 'IntroCinematicDirector.cpp implements time-aware Hermite spline and drawHUD' (($introCpp -match 'hermite') -and ($introCpp -match 'evaluateSpline') -and ($introCpp -match 'drawHUD') -and ($introCpp -match 'TitleGlory') -and ($introCpp -match 'consumeWarpBoomTrigger'))
Check 'intro camera avoids stop-start easing and high-frequency drift' (($introCpp -match 'Time-aware central tangents') -and ($introCpp -notmatch 'driftX') -and ($introCpp -notmatch 'm_camRoll \+= std::sin'))
Check 'SceneManager implements dedicated 3D intro renderer' (($sceneH -match 'void renderIntro') -and ($sceneCpp -match 'void SceneManager::renderIntro') -and ($sceneCpp -match 'res/models/warp_ship\.obj') -and ($sceneCpp -match 'res/models/station_megastructure\.obj') -and ($sceneCpp -match 'res/models/bridge_interior\.obj'))
Check 'AppGL routes active intro through 3D renderer' (($appCpp -match 'renderIntroFrame') -and ($appCpp -match 'm_sceneManager->renderIntro'))
Check 'AppGL exposes six-phase intro capture and CLI' (($appCpp -match 'int AppGL::captureIntroCheck') -and ($appCpp -match 'intro_%02d\.png') -and ((Read-File 'src/main.cpp') -match '--capture-intro'))
Check 'test_intro_cinematic registered in CMakeLists and ASTRO_TEST_TARGETS' (($cmakeTxt -match 'test_intro_cinematic') -and (Test-Path 'tests/test_intro_cinematic.cpp'))
if ($hasModelPack) {
    Check 'orbital megastructure 3D mesh and PBR textures exist' ((Test-Path (Join-Path $root 'res/models/station_megastructure.obj')) -and (Test-Path (Join-Path $root 'res/models/station_megastructure_albedo.bmp')) -and (Test-Path (Join-Path $root 'res/models/station_megastructure_normal.bmp')) -and (Test-Path (Join-Path $root 'res/models/station_megastructure_rough.bmp')) -and (Test-Path (Join-Path $root 'res/models/station_megastructure_metal.bmp')) -and (Test-Path (Join-Path $root 'res/models/station_megastructure_emissive.bmp')))
    Check 'bridge interior 3D mesh and PBR textures exist' ((Test-Path (Join-Path $root 'res/models/bridge_interior.obj')) -and (Test-Path (Join-Path $root 'res/models/bridge_interior_albedo.bmp')) -and (Test-Path (Join-Path $root 'res/models/bridge_interior_normal.bmp')) -and (Test-Path (Join-Path $root 'res/models/bridge_interior_rough.bmp')) -and (Test-Path (Join-Path $root 'res/models/bridge_interior_metal.bmp')) -and (Test-Path (Join-Path $root 'res/models/bridge_interior_emissive.bmp')))
} else {
    Write-Output 'SKIP: 2 intro mesh and PBR map checks (asset pack not fetched for this ref)'
}
Check 'AppGL declares and wires IntroCinematicDirector and replay button' (($appH -match 'IntroCinematicDirector m_introDirector;') -and ($appCpp -match 'm_introDirector\.start\(\)') -and ($appCpp -match 'm_introDirector\.update\(std::min\(wallDelta,\s*0\.2f\)\)') -and ($appCpp -match 'm_introDirector\.drawHUD') -and ($appCpp -match 'Повторить интро'))
Check 'workshop catalog registers bridge_interior and station_megastructure' (($appCpp -match '"bridge_interior"') -and ($appCpp -match '"station_megastructure"'))

# --- Issue #236: 3D Interstellar Ark Starship, Rotating Centrifuge Habitat & Deep Space Flagship Suite (v5.1.15) ---
Check 'workshop catalog registers ark_ship and interstellar ark entry' (($appCpp -match '"ark_ship"') -and ($appCpp -match 'Interstellar Ark'))
$arkBmpWriter = Read-File 'tools/build_interstellar_ark.py'
Check 'ark BMP writer preserves runtime row order' ($arkBmpWriter -match 'src_row = y \* width \* 3')
if ($hasModelPack) {
    Check 'ark ship 3D mesh exceeds 5k polygons' ((Count-ObjFaces 'res/models/ark_ship.obj') -ge 5000)
    Check 'ark ship 3D mesh and PBR textures exist' ((Test-Path (Join-Path $root 'res/models/ark_ship.obj')) -and (Test-Path (Join-Path $root 'res/models/ark_ship_albedo.bmp')) -and (Test-Path (Join-Path $root 'res/models/ark_ship_normal.bmp')) -and (Test-Path (Join-Path $root 'res/models/ark_ship_rough.bmp')) -and (Test-Path (Join-Path $root 'res/models/ark_ship_metal.bmp')) -and (Test-Path (Join-Path $root 'res/models/ark_ship_emissive.bmp')))
} else {
    Write-Output 'SKIP: ark ship mesh and PBR map checks (asset pack not fetched for this ref)'
}

# --- Stage 6 (v5.1.0): Exoplanetary Biomes, Ocean Hydrodynamics, Autonomous 10km Drone & CI Resilience (#241) ---
$planetFrag = Read-File 'shaders/planet.frag'
$shipFrag = Read-File 'shaders/ship.frag'
$landH = Read-File 'src/LandingSystem.h'
$landCpp = Read-File 'src/LandingSystem.cpp'
$solarCpp = Read-File 'src/SolarSystem.cpp'
$buildYml = Read-File '.github/workflows/build.yml'
Check 'planet shader defines u_Terraform, u_Wake and shared oceanGlintGGX' (($planetFrag -match 'uniform float u_Terraform;') -and ($planetFrag -match 'uniform vec4 u_Wake;') -and ($planetFrag -match 'float oceanGlintGGX\('))
Check 'planet shader implements Hercules-b, Titan and Europa biomes' (($planetFrag -match 'u_Body == 12') -and ($planetFrag -match 'u_Body == 13') -and ($planetFrag -match 'u_Body == 14') -and ($solarCpp -match 'd\.detail = 13') -and ($solarCpp -match 'd\.detail = 14'))
Check 'SceneManager passes u_Terraform, u_Wake and Hercules-b biome id 12' (($sceneCpp -match 'setUniform1f\("u_Terraform"') -and ($sceneCpp -match 'setUniform4f\("u_Wake"') -and ($sceneCpp -match 'drawPlanet\(hbPos, 0\.22, 2, 12,'))
Check 'LandingSystem implements autonomous 10km recon drone state machine' (($landH -match 'enum class ReconDroneState') -and ($landH -match 'createReconDroneMission') -and ($landCpp -match 'deployReconDrone') -and ($landCpp -match 'updateReconDrone') -and ($landCpp -match 'formatReconDroneZettel'))
Check 'AppGL integrates recon drone radar UI and Zettelkasten telemetry' (($appH -match 'ReconDroneTelemetry m_droneTelem;') -and ($appCpp -match 'createReconDroneMission') -and ($appCpp -match 'updateReconDrone') -and ($appCpp -match '10 km Perimeter'))
Check 'ship shader renders 3rd-person re-entry plasma sheath and SceneManager draws 3D recon drone' ((($shipFrag -match 'uniform float u_EntryPlasma;') -and ($sceneCpp -match 'setEntryPlasma')) -and (($sceneCpp -match 'g_reconVisual\.active') -and ($appCpp -match 'setReconDroneVisual')))
Check 'build.yml hardens macOS arm64 provisioning against outage (#241)' (($buildYml -match 'fail-fast:\s*false') -and ($buildYml -match 'continue-on-error:') -and ($buildYml -match 'build-macos-gl'))

# --- v5.2.0: Kerr Rotating Black Hole (Ergosphere + Frame-Dragging) & Deep Space Emergent Events ---
$eventsH = Read-File 'src/SpaceEventsSystem.h'
$eventsCpp = Read-File 'src/SpaceEventsSystem.cpp'
Check 'bh shader implements Kerr spin, ergosphere, Bardeen ISCO and Lense-Thirring frame-dragging' (($bh -match 'uniform float u_KerrSpin;') -and ($bh -match 'kerrErgoRadius\(') -and ($bh -match 'kerrIscoRadius\(') -and ($bh -match 'kerrOmegaLt\(') -and ($bh -match 'cartKerrAccel\('))
Check 'RenderSettings, SaveSettings, SceneManager and AppGL expose u_KerrSpin controls' (($sceneH -match 'float kerrSpin = 0\.0f;') -and ($saveH -match 'float kerrSpin = 0\.0f;') -and ($sceneCpp -match 'setUniform1f\("u_KerrSpin"') -and ($appCpp -match 'Kerr Spin \(a/M\)'))
Check 'SpaceEventsSystem declares emergent SOS, rogue asteroid and relic probe encounters' (($eventsH -match 'enum class SpaceEventType') -and ($eventsH -match 'SosBeacon') -and ($eventsH -match 'RogueAsteroid') -and ($eventsH -match 'RelicProbe') -and ($eventsH -match 'class SpaceEventsManager'))
Check 'SpaceEventsSystem implements physics drift, deflection, persistence and test_space_events' (($eventsCpp -match 'ensureSectorEvents') -and ($eventsCpp -match 'engageEvent') -and ($eventsCpp -match 'serialize') -and ($cmakeTxt -match 'test_space_events') -and (Test-Path 'tests/test_space_events.cpp'))
Check 'SceneManager and AppGL render 3D space events, HUD alerts and radar controls' (($sceneCpp -match 'g_spaceEventVisuals') -and ($appCpp -match 'setSpaceEventVisuals') -and ($appCpp -match 'drawSpaceEventHud') -and ($appCpp -match 'Ping Scanner'))

if ($script:fail -gt 0) { Write-Output ''; Write-Output ($script:fail.ToString() + ' check(s) FAILED'); exit 1 }
Write-Output ''
Write-Output 'All checks passed.'



