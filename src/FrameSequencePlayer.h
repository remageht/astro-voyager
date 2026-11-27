#pragma once

#include <functional>
#include <string>

namespace astro {

// Reusable player state & texture manager for 30 fps frame_%04d.jpg + audio.wav sequences
// (intro video, menu->gameplay transition, exit destruction sequence).
class FrameSequencePlayer {
 public:
  enum class StepResult {
    Idle = 0,
    Playing = 1,
    Finished = 2
  };

  // Scan the sequence directory once (via frame_0001 probe path) and cache resolved directory & count.
  int ensureScanned(const std::string& resolvedProbePath);

  // Start playback from frame 1 (requires prior or explicit scan path). Returns false if no frames exist.
  bool start(const std::string& resolvedProbePath, double fps = 30.0);

  // Stop playback and reset audio state (texture kept cached for instant restart until releaseTexture()).
  void stop();

  // Advance playback clock, returning Idle / Playing / Finished and setting outStartAudio on frame 0.
  StepResult step(double wallDelta, bool& outStartAudio, int& outTargetFrame);

  // Load frame1Based JPG into OpenGL texture if needed via caller-provided RGBA texture uploader.
  bool uploadFrame(
      int frame1Based,
      const std::function<unsigned int(const unsigned char*, int, int)>& uploader);

  void releaseTexture();

  bool isActive() const { return m_active; }
  int totalFrames() const { return m_totalFrames; }
  int currentFrameIdx() const { return m_frameIdx; }
  double elapsed() const { return m_timeSec; }
  double fps() const { return m_fps; }
  double durationSec() const { return (m_fps > 0.0) ? (m_totalFrames / m_fps) : 0.0; }
  const std::string& resolvedDir() const { return m_resolvedDir; }
  std::string audioPath() const { return m_resolvedDir.empty() ? std::string() : (m_resolvedDir + "/audio.wav"); }
  unsigned int textureId() const { return m_tex; }
  int width() const { return m_width; }
  int height() const { return m_height; }

 private:
  bool m_active = false;
  bool m_scanned = false;
  bool m_audioPlayed = false;
  double m_timeSec = 0.0;
  double m_fps = 30.0;
  int m_totalFrames = 0;
  int m_frameIdx = -1;
  std::string m_scannedProbe;
  std::string m_resolvedDir;
  unsigned int m_tex = 0;
  int m_width = 0;
  int m_height = 0;
};

}  // namespace astro
