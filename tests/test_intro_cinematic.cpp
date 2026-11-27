#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <glm/glm.hpp>
#include "IntroCinematicDirector.h"
#include "StorySlides.h"

int main() {
  std::cout << "[test_intro_cinematic] Starting unit tests...\n";

  StorySlide legacyVideo;
  assert(legacyVideo.videoFps == 12.0);
  StorySlide tourVideo{"", "", 0, "", "res/story/worlds_tour", 30.0};
  assert(tourVideo.videoDir == "res/story/worlds_tour");
  assert(tourVideo.videoFps == 30.0);
  std::cout << "  PASS: Video slide fps defaults and tour override\n";

  astro::IntroCinematicDirector dir;
  assert(!dir.isActive());
  assert(!dir.isFinished());

  // 1. Start test
  dir.start();
  assert(dir.isActive());
  assert(!dir.isFinished());
  assert(dir.phase() == astro::IntroPhase::VoidDrift);
  assert(dir.totalProgress() == 0.0f);

  // Initial camera sanity
  glm::vec3 cPos = dir.cameraPosition();
  glm::vec3 cUp = dir.cameraUp();
  assert(!std::isnan(cPos.x) && !std::isnan(cPos.y) && !std::isnan(cPos.z));
  assert(std::abs(glm::length(cUp) - 1.0f) < 0.01f);
  std::cout << "  PASS: Initialization and start state\n";

  // Smooth camera path: no stop-start easing at keyframes and no harsh flash spike.
  dir.start();
  const float smoothDt = 1.0f / 120.0f;
  float maxFlash = 0.0f;
  float maxStep = 0.0f;
  glm::vec3 prevPos = dir.cameraPosition();
  for (int i = 0; i < static_cast<int>(22.0f / smoothDt); ++i) {
    dir.update(smoothDt);
    const glm::vec3 pos = dir.cameraPosition();
    maxStep = std::max(maxStep, glm::length(pos - prevPos));
    maxFlash = std::max(maxFlash, dir.flashIntensity());
    prevPos = pos;
  }
  assert(maxStep < 0.75f);
  assert(maxFlash <= 0.221f);

  const float keyframeTimes[] = {3.5f, 7.0f, 10.5f, 14.5f, 18.0f};
  for (float boundary : keyframeTimes) {
    dir.start();
    while (dir.currentTime() < boundary - 0.6f) dir.update(smoothDt);
    float minSpeed = std::numeric_limits<float>::max();
    float maxSpeed = 0.0f;
    prevPos = dir.cameraPosition();
    while (dir.currentTime() < boundary + 0.6f) {
      dir.update(smoothDt);
      const glm::vec3 pos = dir.cameraPosition();
      const float speed = glm::length(pos - prevPos) / smoothDt;
      if (std::abs(dir.currentTime() - boundary) < 0.15f) {
        minSpeed = std::min(minSpeed, speed);
      }
      maxSpeed = std::max(maxSpeed, speed);
      prevPos = pos;
    }
    assert(minSpeed > maxSpeed * 0.15f);
  }
  std::cout << "  PASS: Continuous camera velocity and soft flash envelope\n";

  dir.start();

  // 2. Progression into Warp Burst
  dir.update(4.0f); // Advance to t=4.0s
  assert(dir.phase() == astro::IntroPhase::WarpBurst);
  assert(dir.warpDistortion() > 0.0f);
  assert(dir.flashIntensity() > 0.0f);
  assert(dir.consumeWarpBoomTrigger());
  assert(!dir.consumeWarpBoomTrigger()); // consumed once
  std::cout << "  PASS: Phase 1 WarpBurst & sound trigger\n";

  // 3. Progression into ShipPass
  dir.update(4.0f); // Advance to t=8.0s
  assert(dir.phase() == astro::IntroPhase::ShipPass);
  assert(dir.consumeEngineWhooshTrigger());
  std::cout << "  PASS: Phase 2 ShipPass & engine whoosh trigger\n";

  // 4. Progression into MegastructureReveal
  dir.update(4.0f); // Advance to t=12.0s
  assert(dir.phase() == astro::IntroPhase::MegastructureReveal);
  glm::mat4 megaMat = dir.megastructureModelMatrix();
  assert(!std::isnan(megaMat[3][0]) && !std::isnan(megaMat[3][1]));
  std::cout << "  PASS: Phase 3 MegastructureReveal & transform\n";

  // 5. Progression into BridgeEnter
  dir.update(4.0f); // Advance to t=16.0s
  assert(dir.phase() == astro::IntroPhase::BridgeEnter);
  assert(dir.consumeBridgeOnlineTrigger());
  std::cout << "  PASS: Phase 4 BridgeEnter & bridge online trigger\n";

  // 6. Progression into TitleGlory
  dir.update(3.0f); // Advance to t=19.0s
  assert(dir.phase() == astro::IntroPhase::TitleGlory);
  assert(dir.consumeTitleChimeTrigger());
  std::cout << "  PASS: Phase 5 TitleGlory & title chime trigger\n";

  // 7. Full completion
  dir.update(4.0f); // Advance to t=23.0s
  assert(dir.phase() == astro::IntroPhase::Finished);
  assert(dir.isFinished());
  assert(!dir.isActive());
  assert(dir.totalProgress() >= 1.0f);
  std::cout << "  PASS: Natural completion after total duration\n";

  // 8. Skip test
  dir.start();
  assert(dir.isActive());
  dir.update(1.0f);
  assert(dir.phase() == astro::IntroPhase::VoidDrift);
  dir.skip();
  assert(!dir.isFinished()); // Fading out
  dir.update(0.4f); // Exceed skip fade duration (0.35s)
  assert(dir.isFinished());
  assert(!dir.isActive());
  std::cout << "  PASS: Instant skip and soft fadeout\n";

  std::cout << "[test_intro_cinematic] ALL CHECKS PASSED!\n";
  return 0;
}
