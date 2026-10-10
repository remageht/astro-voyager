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
Check 'catalog types' ($catalogH -match 'BlackHole')

Check 'camera clamp' ($cameraCpp -match 'clampNearHorizon')
Check 'camera forward' ($cameraH -match 'forward\(\)')
Check 'geodesic accel' ($geoCpp -match 'geodesicAcceleration')
Check 'geodesic rk4' ($geoCpp -match 'rk4Step')
Check 'geodesic trace' ($geoH -match 'traceRay')
Check 'renormalizeTime decl' ($geoH -match 'renormalizeTime')
Check 'renormalizeTime impl' ($geoCpp -match 'void renormalizeTime\(SchwState& s, double rs\)')
Check 'rk4Step renormalizes' ($geoCpp -match '(?s)renormalizeTime\(s, rs\);\s*\r?\n\}')
Check 'glsl renormalizeTime decl' ($bh -match 'void renormalizeTime\(inout vec4 p, inout vec4 dp\)')
Check 'glsl renormalize after rk4' ($bh -match '(?s)rk4\(hAdapt, p, dp\);\s*\r?\n\s*renormalizeTime\(p, dp\);')
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

if ($script:fail -gt 0) { Write-Output ''; Write-Output ($script:fail.ToString() + ' check(s) FAILED'); exit 1 }
Write-Output ''
Write-Output 'All checks passed.'
