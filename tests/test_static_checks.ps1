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
Check 'ISCO 3 Rs' ($bh -match 'rIn = u_Rs \* 3\.0')
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

Check 'CI matrix' (($ci -match 'windows-latest') -and ($ci -match 'ubuntu-latest'))
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
Check 'ship hull switchable 3 styles' ((($appCpp) -match 'Ring Ship \(IXS\)') -and ($sceneCpp -match 'setShipStyle') -and ($sceneCpp -match 'shipStyle'))
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
Check 'V key camera views' (($appCpp -match 'GLFW_KEY_V') -and ($appCpp -match 'view::nextView') -and ($appCpp -match 'resetRelativity'))
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
Check 'story journal HUD panel' (($appCpp -match 'drawStoryPanel') -and ($appCpp -match 'm_storyOpen') -and ($appCpp -match 'GLFW_KEY_J') -and ($appCpp -match 'm_prevJPressed'))
Check 'story journal refresh hook' (($appCpp -match 'StoryJournal::Progress') -and ($appCpp -match 'm_story\.refresh\(sp\)'))
Check 'story journal test wired' (($cmake -match 'tests/test_story\.cpp') -and ($cmake -match 'add_test\(NAME test_story'))

# --- v3.5.0 expedition pass ---
Check 'expedition core module' (((Read-File 'src/Expedition.h') -match 'class ExpeditionState') -and ((Read-File 'src/Expedition.cpp') -match 'expeditionChapters') -and ($cmake -match 'src/Expedition\.cpp'))
Check 'expedition slides module' (((Read-File 'src/StorySlides.h') -match 'storyIntroSlides') -and ((Read-File 'src/StorySlides.cpp') -match 'drawStoryArt') -and ($cmake -match 'src/StorySlides\.cpp'))
Check 'expedition play hook' (($appCpp -match 'startExpeditionIntro') -and ($appCpp -match 'Begin the expedition'))
Check 'expedition map key N' (($appCpp -match 'GLFW_KEY_N') -and ($appCpp -match 'm_expeditionMapOpen'))
Check 'expedition test wired' (($cmake -match 'tests/test_expedition\.cpp') -and ($cmake -match 'add_test\(NAME test_expedition'))
Check 'expedition intro shot' (($appCpp -match 'intro\.png') -and ($appCpp -match 'startExpeditionIntro\(\);'))

if ($script:fail -gt 0) { Write-Output ''; Write-Output ($script:fail.ToString() + ' check(s) FAILED'); exit 1 }
Write-Output ''
Write-Output 'All checks passed.'
