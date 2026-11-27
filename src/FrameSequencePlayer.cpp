#include "FrameSequencePlayer.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <system_error>

#ifdef ASTROVOYAGER_ENABLE_GL
#include "GLCompat.h"
#include <stb_image.h>
#endif

namespace astro {

int FrameSequencePlayer::ensureScanned(const std::string& resolvedProbePath) {
  if (m_scanned && m_scannedProbe == resolvedProbePath) return m_totalFrames;
  m_scanned = true;
  m_scannedProbe = resolvedProbePath;
  std::error_code ec;
  if (resolvedProbePath.empty() || !std::filesystem::exists(resolvedProbePath, ec)) {
    m_resolvedDir.clear();
    m_totalFrames = 0;
    return 0;
  }
  m_resolvedDir = std::filesystem::path(resolvedProbePath).parent_path().string();
  int count = 0;
  for (int i = 1; i <= 3600; ++i) {
    char fname[32];
    std::snprintf(fname, sizeof(fname), "frame_%04d.jpg", i);
    if (!std::filesystem::exists(std::filesystem::path(m_resolvedDir) / fname, ec)) break;
    count = i;
  }
  m_totalFrames = count;
  return m_totalFrames;
}

bool FrameSequencePlayer::start(const std::string& resolvedProbePath, double fps) {
  ensureScanned(resolvedProbePath);
  if (m_totalFrames <= 0) return false;
  m_fps = (fps > 1.0) ? fps : 30.0;
  m_active = true;
  m_timeSec = 0.0;
  m_frameIdx = -1;
  m_audioPlayed = false;
  return true;
}

void FrameSequencePlayer::stop() {
  m_active = false;
  m_audioPlayed = false;
}

FrameSequencePlayer::StepResult FrameSequencePlayer::step(double wallDelta,
                                                           bool& outStartAudio,
                                                           int& outTargetFrame) {
  outStartAudio = false;
  outTargetFrame = 0;
  if (!m_active) return StepResult::Idle;

  if (!m_audioPlayed) {
    m_audioPlayed = true;
    m_timeSec = 0.0;
    outStartAudio = true;
  } else {
    m_timeSec += std::clamp(wallDelta, 0.0, 0.1);
  }

  outTargetFrame = static_cast<int>(m_timeSec * m_fps) + 1;
  if (outTargetFrame > m_totalFrames) {
    m_active = false;
    m_audioPlayed = false;
    return StepResult::Finished;
  }
  return StepResult::Playing;
}

bool FrameSequencePlayer::uploadFrame(
    int frame1Based,
    const std::function<unsigned int(const unsigned char*, int, int)>& uploader) {
#ifdef ASTROVOYAGER_ENABLE_GL
  if (frame1Based <= 0 || frame1Based > m_totalFrames || m_resolvedDir.empty()) return false;
  if (frame1Based == m_frameIdx && m_tex != 0) return true;
  char fname[32];
  std::snprintf(fname, sizeof(fname), "frame_%04d.jpg", frame1Based);
  const std::string path = (std::filesystem::path(m_resolvedDir) / fname).string();
  int w = 0, h = 0;
  unsigned char* px = stbi_load(path.c_str(), &w, &h, nullptr, 4);
  if (!px) return false;
  if (m_tex == 0 || w != m_width || h != m_height) {
    if (m_tex != 0) glDeleteTextures(1, &m_tex);
    m_tex = uploader ? uploader(px, w, h) : 0;
    m_width = w;
    m_height = h;
  } else {
    glBindTexture(GL_TEXTURE_2D, m_tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px);
  }
  stbi_image_free(px);
  m_frameIdx = frame1Based;
  return m_tex != 0;
#else
  (void)frame1Based;
  (void)uploader;
  return false;
#endif
}

void FrameSequencePlayer::releaseTexture() {
#ifdef ASTROVOYAGER_ENABLE_GL
  if (m_tex != 0) {
    glDeleteTextures(1, &m_tex);
    m_tex = 0;
  }
#else
  m_tex = 0;
#endif
  m_frameIdx = -1;
  m_width = 0;
  m_height = 0;
}

}  // namespace astro
