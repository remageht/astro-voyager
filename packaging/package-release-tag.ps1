param(
  [Parameter(Mandatory=$true)]
  [string]$Tag
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Get-Item $PSScriptRoot).Parent.FullName
$worktree = Join-Path (Split-Path -Parent $repoRoot) 'astro-voyager-worktree'

Write-Host "===> Packaging Portable Release for $Tag"
Write-Host "Repo root: $repoRoot"
Write-Host "Worktree:  $worktree"

# 1. Checkout tag in worktree
Write-Host "--- Checking out $Tag in worktree..."
git -C $worktree checkout -f $Tag

# 2. Ensure junctions for Dependencies and src/vendor
$depJunction = Join-Path $worktree 'Dependencies'
if (-not (Test-Path $depJunction)) {
  cmd /c mklink /J $depJunction (Join-Path $repoRoot 'Dependencies')
}
$vendorJunction = Join-Path $worktree 'src\vendor'
if (-not (Test-Path $vendorJunction)) {
  cmd /c mklink /J $vendorJunction (Join-Path $repoRoot 'src\vendor')
}

# 3. Patch Window.cpp for AMD compatibility and instant dark clear
Write-Host "--- Patching Window.cpp for AMD GPU compatibility..."
$winCppPath = Join-Path $worktree 'src\Window.cpp'
$winCpp = Get-Content $winCppPath -Raw -Encoding UTF8

# Replace context hint block if not already patched
if ($winCpp -notmatch 'kGLConfigs') {
  $target = 'glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);'
  $patch = @'
  glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);
  static const struct { int maj; int min; int prof; } kGLConfigs[] = {
    {3, 3, GLFW_OPENGL_CORE_PROFILE},
    {3, 3, GLFW_OPENGL_COMPAT_PROFILE},
    {3, 2, GLFW_OPENGL_CORE_PROFILE},
    {3, 2, GLFW_OPENGL_COMPAT_PROFILE},
    {3, 0, GLFW_OPENGL_ANY_PROFILE}
  };
  bool created = false;
  for (const auto& cfg : kGLConfigs) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, cfg.maj);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, cfg.min);
    glfwWindowHint(GLFW_OPENGL_PROFILE, cfg.prof);
#ifdef __APPLE__
    if (cfg.maj > 3 || (cfg.maj == 3 && cfg.min >= 2)) glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);
    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (m_window) { created = true; break; }
  }
  if (!created) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window: OpenGL context creation failed");
  }
'@
  # Find m_window = glfwCreateWindow
  $idx = $winCpp.IndexOf('m_window = glfwCreateWindow')
  if ($idx -gt 0) {
    # Replace from glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR to after m_window creation check
    $startIdx = $winCpp.LastIndexOf('glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR', $idx)
    $endIdx = $winCpp.IndexOf('}', $winCpp.IndexOf('throw std::runtime_error', $idx)) + 1
    $winCpp = $winCpp.Substring(0, $startIdx) + $patch + $winCpp.Substring($endIdx)
  }
}

# Add immediate clear after glewInit if not already present
if ($winCpp -notmatch 'glClearColor\(0.01f') {
  $clearBlock = @'
  glGetError();
  glClearColor(0.01f, 0.02f, 0.04f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glfwSwapBuffers(m_window);
'@
  $winCpp = $winCpp -replace 'glGetError\(\);', $clearBlock
}

# Add graceful hide to destructor if not already present
if ($winCpp -notmatch 'glfwHideWindow') {
  $hideBlock = @'
  if (m_window) {
    glfwHideWindow(m_window);
    glfwPollEvents();
    glfwDestroyWindow(m_window);
'@
  $winCpp = $winCpp -replace 'if\s*\(\s*m_window\s*\)\s*\{\s*glfwDestroyWindow\(m_window\);', $hideBlock
}

Set-Content $winCppPath -Value $winCpp -Encoding UTF8

# 4. Configure & Build
Write-Host "--- Configuring CMake..."
$buildDir = Join-Path $worktree 'build-gl'
cmake -S $worktree -B $buildDir -DASTROVOYAGER_ENABLE_GL=ON | Out-Null
Write-Host "--- Compiling Release binary..."
cmake --build $buildDir --config Release --target astro-voyager

$exeBuilt = Join-Path $buildDir 'Release\astro-voyager.exe'
if (-not (Test-Path $exeBuilt)) {
  throw "Build failed: $exeBuilt does not exist!"
}

# 5. Assemble portable distribution folder
$stageDir = Join-Path $repoRoot "dist\portable-$Tag"
if (Test-Path $stageDir) { Remove-Item $stageDir -Recurse -Force }
New-Item -ItemType Directory -Force $stageDir | Out-Null

Copy-Item $exeBuilt (Join-Path $stageDir 'astro-voyager.exe')
Copy-Item (Join-Path $worktree 'shaders') (Join-Path $stageDir 'shaders') -Recurse

$stageRes = Join-Path $stageDir 'res'
New-Item -ItemType Directory -Force $stageRes | Out-Null
Copy-Item (Join-Path $repoRoot 'res\textures') (Join-Path $stageRes 'textures') -Recurse
if (Test-Path (Join-Path $repoRoot 'res\models')) {
  Copy-Item (Join-Path $repoRoot 'res\models') (Join-Path $stageRes 'models') -Recurse
}
if (Test-Path (Join-Path $repoRoot 'res\audio')) {
  Copy-Item (Join-Path $repoRoot 'res\audio') (Join-Path $stageRes 'audio') -Recurse
}
if (Test-Path (Join-Path $repoRoot 'res\story')) {
  Copy-Item (Join-Path $repoRoot 'res\story') (Join-Path $stageRes 'story') -Recurse
}
if (Test-Path (Join-Path $repoRoot 'res\video')) {
  Copy-Item (Join-Path $repoRoot 'res\video') (Join-Path $stageRes 'video') -Recurse
}
foreach ($f in @('catalog.json', 'constellations.json', 'icon.png')) {
  $srcFile = Join-Path $repoRoot "res\$f"
  if (Test-Path $srcFile) {
    Copy-Item $srcFile (Join-Path $stageRes $f)
  }
}
Copy-Item (Join-Path $repoRoot 'README.md') $stageDir
Copy-Item (Join-Path $repoRoot 'LICENSE') $stageDir

$readme = @"
ASTRO-VOYAGER $Tag - PORTABLE BUILD
====================================
Установка НЕ требуется: распакуйте архив в любую папку и запустите astro-voyager.exe.

В этой сборке применены исправления для старых видеокарт AMD (устранён белый экран)
и включен полный пакет 3D-моделей, текстур и аудио.
"@
Set-Content (Join-Path $stageDir 'PORTABLE-README.txt') -Value $readme -Encoding UTF8

# 6. Compress into portable zip
$zipName = "astro-voyager-$Tag-windows-x86_64-portable.zip"
$zipPath = Join-Path (Join-Path $repoRoot 'dist') $zipName
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Write-Host "--- Compressing $zipName..."
Compress-Archive -Path (Join-Path $stageDir '*') -DestinationPath $zipPath -Force
$zipSizeMb = [math]::Round((Get-Item $zipPath).Length / 1MB, 1)
Write-Host "Created portable archive: $zipPath ($zipSizeMb MB)"

# 7. Clean up non-working bare assets from GitHub Release
Write-Host "--- Cleaning up non-working bare assets from GitHub release $Tag..."
$bareZip = "astro-voyager-$Tag-windows-x86_64.zip"
$bareSetup = "AstroVoyager-$Tag-setup.exe"
$bareExe = "astro-voyager.exe"
foreach ($asset in @($bareZip, $bareSetup, $bareExe)) {
  try {
    gh release delete-asset $Tag $asset -y 2>$null
  } catch {}
}

# 8. Upload portable package to release
Write-Host "--- Uploading $zipName to GitHub release $Tag..."
gh release upload $Tag $zipPath --clobber

Write-Host "===> SUCCESS: $Tag packaged and published!"
