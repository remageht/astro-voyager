#include "../src/FrameSequencePlayer.h"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int main() {
  namespace fs = std::filesystem;

  // Create isolated temp directory inside current test working directory.
  const fs::path tempDir = fs::current_path() / "astro_voyager_test_seq";
  std::error_code ec;
  fs::remove_all(tempDir, ec);
  assert(fs::create_directories(tempDir, ec));

  for (int i = 1; i <= 3; ++i) {
    char fname[32];
    std::snprintf(fname, sizeof(fname), "frame_%04d.jpg", i);
    std::ofstream f(tempDir / fname, std::ios::binary);
    f << "dummy";
  }
  {
    std::ofstream a(tempDir / "audio.wav", std::ios::binary);
    a << "wav";
  }

  astro::FrameSequencePlayer player;
  assert(!player.isActive());

  // 1. Missing path scans to 0 and fails start().
  assert(!player.start((tempDir / "missing.jpg").string(), 30.0));
  assert(!player.isActive());

  // 2. Valid probe scans 3 frames and starts cleanly at 10 fps (0.3s total).
  const std::string probe = (tempDir / "frame_0001.jpg").string();
  assert(player.start(probe, 10.0));
  assert(player.isActive());
  assert(player.totalFrames() == 3);
  assert(std::abs(player.durationSec() - 0.3) < 1e-6);
  assert(fs::equivalent(fs::path(player.audioPath()), tempDir / "audio.wav", ec) && !ec);

  // 3. First step triggers audio start without advancing elapsed clock beyond 0.
  bool startAudio = false;
  int targetFrame = 0;
  auto res = player.step(0.05, startAudio, targetFrame);
  assert(res == astro::FrameSequencePlayer::StepResult::Playing);
  assert(startAudio);
  assert(targetFrame == 1);
  assert(std::abs(player.elapsed()) < 1e-9);

  // 4. Subsequent steps clamp wallDelta to 0.1s and advance frame index cleanly.
  res = player.step(0.5, startAudio, targetFrame);  // clamped to +0.1s -> frame 2
  assert(res == astro::FrameSequencePlayer::StepResult::Playing);
  assert(!startAudio);
  assert(targetFrame == 2);
  assert(std::abs(player.elapsed() - 0.1) < 1e-6);

  res = player.step(0.1, startAudio, targetFrame);  // 0.2s -> frame 3
  assert(res == astro::FrameSequencePlayer::StepResult::Playing);
  assert(targetFrame == 3);

  res = player.step(0.1, startAudio, targetFrame);  // 0.3s -> frame 4 > 3 -> Finished
  assert(res == astro::FrameSequencePlayer::StepResult::Finished);

  player.stop();
  assert(!player.isActive());

  fs::remove_all(tempDir, ec);
  std::cout << "test_frame_sequence PASSED\n";
  return 0;
}
