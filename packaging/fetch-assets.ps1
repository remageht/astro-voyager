<#
.SYNOPSIS
  Подтягивает пакет игровых ассетов (3D-модели + текстуры) в рабочее дерево.

.DESCRIPTION
  Модели генерируются скриптами Blender и не лежат в git (около 850 МБ),
  поэтому сборки CI выходят без 3D-моделей. Пакет публикуется один раз как
  asset релиза, а этот скрипт забирает его и распаковывает в res/.

  Если пакета нет (первый запуск или старый тег) - сборка продолжается без
  моделей: игра это переживает (модели опциональны, см. ObjModel::load).

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File packaging/fetch-assets.ps1 -Tag v5.1.0
#>
[CmdletBinding()]
param(
  [string]$Tag = "",
  [string]$Repo = "remageht/astro-voyager",
  [string]$DestRoot = ""
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
if (-not $DestRoot) { $DestRoot = $repoRoot }
if (-not $Tag) {
  $version = (Get-Content (Join-Path $repoRoot 'VERSION') -Raw).Trim()
  $Tag = "v$version"
}

$tmpRoot = $env:RUNNER_TEMP
if (-not $tmpRoot) { $tmpRoot = $env:TEMP }
if (-not $tmpRoot) { $tmpRoot = [System.IO.Path]::GetTempPath() }
$tmp = Join-Path $tmpRoot "av-assets-$Tag"
New-Item -ItemType Directory -Force -Path $tmp | Out-Null

$pattern = "astro-voyager-assets-$Tag.zip"
Write-Host "==> fetching asset pack $pattern from $Repo"

$ok = $false
try {
  gh release download $Tag --pattern $pattern --dir $tmp -R $Repo 2>&1 | Out-Null
  $ok = Test-Path (Join-Path $tmp $pattern)
} catch {
  Write-Host "    asset pack for $Tag not available: $($_.Exception.Message)"
}

if (-not $ok) {
  # Пробуем самый свежий релиз с пакетом — модели одинаковые для всех версий.
  try {
    $latest = gh release list -R $Repo --limit 20 --json tagName,assets --jq
      '[.[] | select(any(.assets[]?; .name | startswith("astro-voyager-assets-"))) | .tagName] | first // empty'
    if ($latest) {
      $latestPattern = "astro-voyager-assets-$latest.zip"
      Write-Host "    trying latest release with assets: $latest"
      gh release download $latest --pattern $latestPattern --dir $tmp -R $Repo 2>&1 | Out-Null
      $ok = Test-Path (Join-Path $tmp $latestPattern)
      if ($ok) { $pattern = $latestPattern }
    }
  } catch {
    Write-Host "    no release asset pack found: $($_.Exception.Message)"
  }
}

if (-not $ok) {
  Write-Warning 'No asset pack available - building without 3D models (game still runs).'
  exit 0
}

Write-Host '    extracting...'
Expand-Archive -LiteralPath (Join-Path $tmp $pattern) -DestinationPath $DestRoot -Force
$modelCount = (Get-ChildItem (Join-Path $DestRoot 'res\models') -Filter *.obj -ErrorAction SilentlyContinue).Count
Write-Host "    done: $modelCount models unpacked into $DestRoot\res\models"
