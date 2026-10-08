#!/usr/bin/env bash
# Fetch the GL sources that CMake expects but that are not committed:
#   Dependencies/GLFW, src/vendor/glm, src/vendor/imgui, src/vendor/stb_image
# (see .gitignore). Pinned to the versions used by the local Windows build.
#
# Usage (from the repository root):
#   bash packaging/fetch-gl-deps.sh
#
# Nothing is committed; sources are downloaded from upstream releases only.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

glfw_version="3.4.0"
glm_version="1.0.1"
imgui_version="1.91.8"
stb_commit="2c980bb59875b0d32144a71867fbdebb2f77cd20"

workdir="$(mktemp -d)"
trap 'rm -rf "${workdir}"' EXIT

log() { printf '==> %s\n' "$*"; }

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

log "GL sources ready"
