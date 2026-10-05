# Static checks for astro-voyager v0.1.0 (ASCII only).
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

Check 'cmake project astro-voyager' ($cmake -match 'project\(astro-voyager')
Check 'cmake cxx17' ($cmake -match 'CMAKE_CXX_STANDARD 17')
Check 'cmake lists Catalog.cpp' ($cmake -match 'src/Catalog\.cpp')
Check 'cmake lists Camera.cpp' ($cmake -match 'src/Camera\.cpp')
Check 'cmake lists Geodesic.cpp' ($cmake -match 'src/Geodesic\.cpp')
Check 'cmake gl option' ($cmake -match 'ASTROVOYAGER_ENABLE_GL')
Check 'version header' ((Read-File 'src/Version.h') -match '0\.[12]\.0')

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

Check 'cli list' ($main -match '--list')
Check 'cli info' ($main -match '--info')
Check 'cli demo-geodesic' ($main -match 'demo-geodesic')

Check 'json has stations' ($catalogJson -match '"stations"')
Check 'json sgra params' ($catalogJson -match 'shellRadius')

Check 'bh shader has RK4' ($bh -match 'rk4')
Check 'bh shader capture' ($bh -match '1\.001')
Check 'bh shader shell' ($bh -match 'u_ShellRadius')

Check 'readme name' ($readme -match 'astro-voyager')
Check 'readme quickstart' ($readme -match 'cmake -S')

Check 'CI matrix' (($ci -match 'windows-latest') -and ($ci -match 'ubuntu-latest'))
Check 'CI docker' ($ci -match 'docker build')
Check 'docker healthcheck-free build image' ($dockerfile -match 'FROM debian')

Check 'catalog doc exists' (Test-Path (Join-Path $root 'docs/CATALOG.md'))
Check 'arch doc exists' (Test-Path (Join-Path $root 'docs/ARCHITECTURE.md'))
Check 'build doc exists' (Test-Path (Join-Path $root 'docs/BUILD.md'))
Check 'roadmap exists' (Test-Path (Join-Path $root 'docs/ROADMAP.md'))
Check 'env example no secret' ((Read-File '.env.example') -notmatch 'sk-|secret123|password')

if ($script:fail -gt 0) { Write-Output ''; Write-Output ($script:fail.ToString() + ' check(s) FAILED'); exit 1 }
Write-Output ''
Write-Output 'All checks passed.'
