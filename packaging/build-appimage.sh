#!/usr/bin/env bash
# Build an AppImage (type 2) for astro-voyager from an already built GL binary.
#
# Usage (from the repository root):
#   cmake -S . -B build-gl -DCMAKE_BUILD_TYPE=Release -DASTROVOYAGER_ENABLE_GL=ON
#   cmake --build build-gl -j"$(nproc)"
#   packaging/build-appimage.sh [outdir]
#
# The script downloads linuxdeploy and appimagetool into a temporary directory
# (nothing is committed). The GL build needs GLFW/GLM/ImGui/stb_image sources,
# which live in Dependencies/ and src/vendor/ locally and are gitignored; when
# they are missing they are fetched from upstream releases.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
outdir="${1:-${repo_root}/dist}"
version="$(tr -d '[:space:]' < "${repo_root}/VERSION")"
app="astro-voyager"
appimage_name="${app}-v${version}-x86_64.AppImage"

# Pinned upstream versions (kept in sync with the local, gitignored sources).
glfw_version="3.4.0"
glm_version="1.0.1"
imgui_version="1.91.8"
stb_commit="2c980bb59875b0d32144a71867fbdebb2f77cd20"

workdir="$(mktemp -d)"
trap 'rm -rf "${workdir}"' EXIT

log() { printf '==> %s\n' "$*"; }

# ---------------------------------------------------------------------------
# 1. Sources that CMake expects (Dependencies/ + src/vendor/)
# ---------------------------------------------------------------------------
fetch_gl_sources() {
  mkdir -p "${repo_root}/Dependencies" "${repo_root}/src/vendor"

  if [ ! -f "${repo_root}/Dependencies/GLFW/CMakeLists.txt" ]; then
    log "fetching GLFW ${glfw_version}"
    curl -fsSL -o "${workdir}/glfw.tar.gz" \
      "https://github.com/glfw/glfw/archive/refs/tags/${glfw_version}.tar.gz"
    mkdir -p "${workdir}/glfw"
    tar -xzf "${workdir}/glfw.tar.gz" -C "${workdir}/glfw" --strip-components=1
    mkdir -p "${repo_root}/Dependencies/GLFW"
    cp -a "${workdir}/glfw/." "${repo_root}/Dependencies/GLFW/"
  fi

  if [ ! -f "${repo_root}/src/vendor/glm/CMakeLists.txt" ] && \
     [ ! -f "${repo_root}/src/vendor/glm/glm/glm.hpp" ]; then
    log "fetching GLM ${glm_version}"
    curl -fsSL -o "${workdir}/glm.tar.gz" \
      "https://github.com/g-truc/glm/archive/refs/tags/${glm_version}.tar.gz"
    mkdir -p "${workdir}/glm"
    tar -xzf "${workdir}/glm.tar.gz" -C "${workdir}/glm" --strip-components=1
    mkdir -p "${repo_root}/src/vendor/glm"
    cp -a "${workdir}/glm/." "${repo_root}/src/vendor/glm/"
  fi

  if [ ! -f "${repo_root}/src/vendor/imgui/imgui.cpp" ]; then
    log "fetching ImGui ${imgui_version}"
    curl -fsSL -o "${workdir}/imgui.tar.gz" \
      "https://github.com/ocornut/imgui/archive/refs/tags/v${imgui_version}.tar.gz"
    mkdir -p "${workdir}/imgui"
    tar -xzf "${workdir}/imgui.tar.gz" -C "${workdir}/imgui" --strip-components=1
    mkdir -p "${repo_root}/src/vendor/imgui"
    cp -a "${workdir}/imgui/." "${repo_root}/src/vendor/imgui/"
  fi

  if [ ! -f "${repo_root}/src/vendor/stb_image/stb_image.h" ]; then
    log "fetching stb_image headers (${stb_commit})"
    mkdir -p "${repo_root}/src/vendor/stb_image"
    curl -fsSL -o "${repo_root}/src/vendor/stb_image/stb_image.h" \
      "https://raw.githubusercontent.com/nothings/stb/${stb_commit}/stb_image.h"
    curl -fsSL -o "${repo_root}/src/vendor/stb_image/stb_image_write.h" \
      "https://raw.githubusercontent.com/nothings/stb/${stb_commit}/stb_image_write.h"
  fi
}

# ---------------------------------------------------------------------------
# 2. AppDir (AppRun, .desktop, icon, binary, shaders, res, docs)
# ---------------------------------------------------------------------------
build_appdir() {
  appdir="${workdir}/AppDir"
  mkdir -p "${appdir}/usr/bin" \
           "${appdir}/usr/share/applications" \
           "${appdir}/usr/share/icons/hicolor/256x256/apps" \
           "${appdir}/usr/share/astro-voyager/shaders" \
           "${appdir}/usr/share/astro-voyager/res"

  cp "${repo_root}/build-gl/${app}" "${appdir}/usr/bin/${app}"
  chmod +x "${appdir}/usr/bin/${app}"

  cp "${repo_root}"/shaders/* "${appdir}/usr/share/astro-voyager/shaders/"
  cp "${repo_root}"/res/* "${appdir}/usr/share/astro-voyager/res/"
  cp "${repo_root}/LICENSE" "${repo_root}/README.md" "${appdir}/usr/share/astro-voyager/"

  cp "${repo_root}/packaging/${app}.desktop" \
     "${appdir}/usr/share/applications/${app}.desktop"
  cp "${repo_root}/packaging/icon.png" \
     "${appdir}/usr/share/icons/hicolor/256x256/apps/${app}.png"
}

# ---------------------------------------------------------------------------
# 3. Bundle the runtime libraries with linuxdeploy, then pack with appimagetool
# ---------------------------------------------------------------------------
make_appimage() {
  curl -fsSL -o "${workdir}/linuxdeploy" \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
  chmod +x "${workdir}/linuxdeploy"

  curl -fsSL -o "${workdir}/appimagetool" \
    "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"
  chmod +x "${workdir}/appimagetool"

  # AppImage tools themselves need a FUSE-free self-extract inside containers.
  export APPIMAGE_EXTRACT_AND_RUN=1
  export ARCH=x86_64
  export VERSION="${version}"

  log "linuxdeploy: bundling runtime libraries"
  "${workdir}/linuxdeploy" \
    --appdir "${appdir}" \
    --desktop-file "${appdir}/usr/share/applications/${app}.desktop" \
    --icon-file "${appdir}/usr/share/icons/hicolor/256x256/apps/${app}.png" \
    --executable "${appdir}/usr/bin/${app}" \
    --exclude-library 'libGL.so*' \
    --exclude-library 'libEGL.so*' \
    --exclude-library 'libGLX.so*' \
    --exclude-library 'libdrm.so*' \
    --exclude-library 'libgbm.so*' \
    --no-appstream

  log "AppRun wrapper uses the bundled assets"
  cat > "${appdir}/AppRun" <<'APPRUN'
#!/bin/sh
# Run astro-voyager from the mounted AppDir so that shaders/ and res/ resolve.
self="$(readlink -f "$0")"
here="$(dirname "$self")"
if [ -d "${here}/usr/share/astro-voyager" ]; then
  cd "${here}/usr/share/astro-voyager"
fi
exec "${here}/usr/bin/astro-voyager" "$@"
APPRUN
  chmod +x "${appdir}/AppRun"

  mkdir -p "${outdir}"
  log "appimagetool: packing ${appimage_name}"
  "${workdir}/appimagetool" --no-appstream "${appdir}" "${outdir}/${appimage_name}"

  log "result"
  ls -l "${outdir}/${appimage_name}"
  sha256sum "${outdir}/${appimage_name}"
}

fetch_gl_sources
build_appdir
make_appimage
