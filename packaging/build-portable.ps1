<#
.SYNOPSIS
  Собирает портативную сборку Astro-Voyager: папку, которую можно
  распаковать и запустить без установки.

.DESCRIPTION
  Зачем это нужно:
   * друзьям проще — распаковал архив и запустил exe, ничего ставить не надо;
   * в обычный zip релиза попадает только exe (без res/), поэтому игра
     запускается, но без 3D-моделей, текстур и звука;
   * этот скрипт кладёт рядом с exe папку res/ (модели, текстуры, звук),
     шейдеры и памятку PORTABLE-README.txt с подсказками на случай
     белого экрана (--diagnostics / --safe).

.PARAMETER OutputDir
  Куда сложить портативную сборку (по умолчанию dist/portable).

.PARAMETER ExePath
  Путь к собранному exe. По умолчанию ищется build-gl/Release/astro-voyager.exe.

.PARAMETER SkipModels
  Не класть res/models (модели весят ~850 МБ) — лёгкий вариант.

.PARAMETER Zip
  Дополнительно упаковать папку в .zip.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File packaging/build-portable.ps1 -Zip
#>
[CmdletBinding()]
param(
  [string]$OutputDir = "",
  [string]$ExePath = "",
  [switch]$SkipModels,
  [switch]$Zip
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $OutputDir) { $OutputDir = Join-Path $repoRoot 'dist\portable' }
if (-not $ExePath) {
  $candidates = @(
    (Join-Path $repoRoot 'build-gl\Release\astro-voyager.exe'),
    (Join-Path $repoRoot 'build-gl\astro-voyager.exe')
  )
  $ExePath = $candidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
}
if (-not $ExePath -or -not (Test-Path -LiteralPath $ExePath)) {
  throw "astro-voyager.exe not found. Build it first: cmake --build build-gl --config Release"
}

Write-Host "==> Portable build"
Write-Host "    repo:  $repoRoot"
Write-Host "    exe:   $ExePath"
Write-Host "    out:   $OutputDir"

if (Test-Path -LiteralPath $OutputDir) {
  Remove-Item -LiteralPath $OutputDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

Copy-Item -LiteralPath $ExePath -Destination (Join-Path $OutputDir 'astro-voyager.exe')

# Шейдеры вшиты в бинарник (EmbeddedShaders), но копия рядом позволяет
# подменить шейдер без пересборки и служит подсказкой для пользователя.
if (Test-Path (Join-Path $repoRoot 'shaders')) {
  Copy-Item -LiteralPath (Join-Path $repoRoot 'shaders') -Destination (Join-Path $OutputDir 'shaders') -Recurse
}

# Ресурсы: звук (лёгкий) + текстуры + модели (тяжёлые).
foreach ($sub in @('audio', 'textures')) {
  $src = Join-Path $repoRoot "res\$sub"
  if (Test-Path $src) {
    Copy-Item -LiteralPath $src -Destination (Join-Path $OutputDir "res\$sub") -Recurse
  }
}
if (-not $SkipModels) {
  $models = Join-Path $repoRoot 'res\models'
  if (Test-Path $models) {
    Write-Host "    copying res/models (this can take a while)..."
    Copy-Item -LiteralPath $models -Destination (Join-Path $OutputDir 'res\models') -Recurse
  } else {
    Write-Warning "res/models not found - the build will run without 3D models."
  }
}

foreach ($doc in @('README.md', 'LICENSE')) {
  $p = Join-Path $repoRoot $doc
  if (Test-Path $p) { Copy-Item -LiteralPath $p -Destination (Join-Path $OutputDir $doc) }
}

$readme = @"
ASTRO-VOYAGER - PORTABLE BUILD
==============================

Установка НЕ требуется: распакуйте архив в любую папку и запустите
astro-voyager.exe.

Если игра не запускается или показывает белый экран:

  1) Проверьте видеокарту и драйвер: запустите
         astro-voyager.exe --diagnostics
     и пришлите вывод (он покажет версию OpenGL и видеокарту).

  2) Лог запуска пишется рядом с exe: astro-voyager.log
     Пришлите его - по нему видно, что именно не загрузилось.

  3) Если сцена идёт с артефактами или совсем чёрная, попробуйте
     облегчённый режим:
         astro-voyager.exe --safe

Требования: OpenGL 3.3+, любой актуальный видеодрайвер.
Папка res/ содержит 3D-модели, текстуры и звук - не удаляйте её.
"@
Set-Content -LiteralPath (Join-Path $OutputDir 'PORTABLE-README.txt') -Value $readme -Encoding UTF8

$hash = (Get-FileHash -LiteralPath (Join-Path $OutputDir 'astro-voyager.exe') -Algorithm SHA256).Hash
"astro-voyager.exe SHA256 = $hash" | Set-Content -LiteralPath (Join-Path $OutputDir 'CHECKSUMS.txt') -Encoding UTF8

$sizeMb = [math]::Round((Get-ChildItem -LiteralPath $OutputDir -Recurse -File | Measure-Object -Property Length -Sum).Sum / 1MB, 1)
Write-Host "    ready: $OutputDir ($sizeMb MB)"

if ($Zip) {
  $zip = "$OutputDir.zip"
  if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
  Compress-Archive -Path (Join-Path $OutputDir '*') -DestinationPath $zip -Force
  $zipMb = [math]::Round((Get-Item -LiteralPath $zip).Length / 1MB, 1)
  Write-Host "    zip:   $zip ($zipMb MB)"
}
