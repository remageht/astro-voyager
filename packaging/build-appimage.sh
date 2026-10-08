#!/usr/bin/env bash
# Build an AppImage (type 2) for astro-voyager from an already built GL binary.
#
# Usage (from the repository root):
#   bash packaging/fetch-gl-deps.sh
#   cmake -S . -B build-gl -DCMAKE_BUILD_TYPE=Release -DASTROVOYAGER_ENABLE_GL=ON
#   cmake --build build-gl -j"$(nproc)"
#   bash packaging/build-appimage.sh [outdir]
#
# linuxdeploy and appimagetool are downloaded into a temporary directory; the
# GL sources themselves come from packaging/fetch-gl-deps.sh (nothing committed).
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
outdir="${1:-${repo_root}/dist}"
version="$(tr -d '[:space:]' < "${repo_root}/VERSION")"
app="astro-voyager"
appimage_name="${app}-v${version}-x86_64.AppImage"

workdir="$(mktemp -d)"
trap 'rm -rf "${workdir}"' EXIT

log() { printf '==> %s\n' "$*"; }

# GL sources are not committed (see .gitignore): fetch them when missing so the
# script works both locally and from scratch in CI.
bash "${repo_root}/packaging/fetch-gl-deps.sh"

if [ ! -x "${repo_root}/build-gl/${app}" ]; then
  echo "error: ${repo_root}/build-gl/${app} not found; build with -DASTROVOYAGER_ENABLE_GL=ON first" >&2
  exit 1
fi

# ---------------------------------------------------------------------------
# AppDir (AppRun, .desktop, icon, binary, shaders, res, docs)
# ---------------------------------------------------------------------------
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

# ---------------------------------------------------------------------------
# Bundle the runtime libraries with linuxdeploy, then pack with appimagetool
# ---------------------------------------------------------------------------
curl -fsSL -o "${workdir}/linuxdeploy" \
  "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
chmod +x "${workdir}/linuxdeploy"

curl -fsSL -o "${workdir}/appimagetool" \
  "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"
chmod +x "${workdir}/appimagetool"

# The AppImage tools themselves need a FUSE-free self-extract inside containers.
export APPIMAGE_EXTRACT_AND_RUN=1
export ARCH=x86_64
export VERSION="${version}"
# Keep the host GL stack out of the bundle (official AppImage guidance);
# linuxdeploy reads this semicolon-separated glob list.
export LINUXDEPLOY_EXCLUDED_LIBRARIES='libGL.so*;libEGL.so*;libGLX.so*;libdrm.so*;libgbm.so*'

log "linuxdeploy: bundling runtime libraries"
"${workdir}/linuxdeploy" \
  --appdir "${appdir}" \
  --desktop-file "${appdir}/usr/share/applications/${app}.desktop" \
  --icon-file "${appdir}/usr/share/icons/hicolor/256x256/apps/${app}.png" \
  --executable "${appdir}/usr/bin/${app}"

# linuxdeploy wraps usr/bin/<app> in a shell script that re-resolves its own
# directory with `readlink -f "$0"`, and deploys AppRun as a symlink to that
# wrapper. Through the symlink $0 resolves to the wrapper itself and linuxdeploy
# doubles the path (AppDir/usr/bin/usr/bin/<app>). Replace the wrapper body with
# a resolution that works both for the mounted AppImage (APPDIR) and for an
# --appimage-extract'ed AppDir.
elf="${appdir}/usr/bin/${app}"
if head -1 "${elf}" | grep -q '^#!/bin/sh'; then
  log "rewriting the linuxdeploy exec wrapper (${app})"
  dest="${workdir}/wrapper.${app}"
  {
    printf '#!/bin/sh\n'
    printf '# written by packaging/build-appimage.sh: linuxdeploy wrapper, path-safe\n'
    printf 'here="${APPDIR:-}"\n'
    printf '[ -n "$here" ] || here="$(cd "$(dirname -- "$0")" && pwd)"\n'
    printf 'exec "$here/usr/bin/%s" "$@"\n' "${app}"
  } > "${dest}"
  cat "${dest}" > "${elf}"
  chmod +x "${elf}"
fi

# Replace the AppRun symlink with a real script: the runtime exports APPDIR and
# the CLI must run with the working directory set to the bundled assets.
rm -f "${appdir}/AppRun"
cat > "${appdir}/AppRun" <<'APPRUN'
#!/bin/sh
# Run astro-voyager from the mounted AppDir so that shaders/ and res/ resolve.
self="$0"
here="$(cd "$(dirname "${self}")" && pwd)"
if [ -n "${APPDIR:-}" ] && [ -d "${APPDIR}/usr/bin" ]; then
  here="${APPDIR}"
fi
if [ -d "${here}/usr/share/astro-voyager" ]; then
  cd "${here}/usr/share/astro-voyager"
fi
exec "${here}/usr/bin/astro-voyager" "$@"
APPRUN
chmod +x "${appdir}/AppRun"

mkdir -p "${outdir}"
log "appimagetool: packing ${appimage_name}"
"${workdir}/appimagetool" "${appdir}" "${outdir}/${appimage_name}"

log "result"
ls -l "${outdir}/${appimage_name}"
sha256sum "${outdir}/${appimage_name}"
