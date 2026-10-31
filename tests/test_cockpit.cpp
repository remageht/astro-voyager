// test_cockpit.cpp (v4.2.0 Alpha): Unit test for CockpitController dynamics and HUD telemetry
#include "CockpitController.h"
#include <iostream>
#include <cmath>
#include <cassert>

using namespace astro;

int main() {
  CockpitController ctrl;

  // Test 1: Baseline reset state
  {
    ctrl.reset();
    glm::vec3 off = ctrl.getHeadOffset();
    glm::vec3 tilt = ctrl.getHeadAngularTilt();
    if (std::abs(off.x) > 1e-4f || std::abs(off.y) > 1e-4f || std::abs(off.z) > 1e-4f) {
      std::cerr << "Test 1 failed: Initial head offset is non-zero\n";
      return 1;
    }
    if (std::abs(tilt.x) > 1e-4f || std::abs(tilt.y) > 1e-4f || std::abs(tilt.z) > 1e-4f) {
      std::cerr << "Test 1 failed: Initial head tilt is non-zero\n";
      return 1;
    }
    std::cout << "PASS: Cockpit Test 1 (reset baseline)\n";
  }

  // Test 2: Inertial turn reaction (yaw turning left causes head shift right)
  {
    ctrl.reset();
    glm::vec3 pos(0.0f);
    glm::vec3 right(1.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 fwd(0.0f, 0.0f, -1.0f);

    // Initial update to seed previous vectors
    ctrl.update(0.016f, pos, fwd, right, up, 10.0f, 0.5f, false, 0.0f, 1.0f, 1.0f, "", glm::vec3(0.0f), 0.0f);

    // Turn yaw sharply
    glm::vec3 turnedFwd = glm::normalize(glm::vec3(0.3f, 0.0f, -0.95f));
    glm::vec3 turnedRight = glm::normalize(glm::cross(turnedFwd, up));
    for (int i = 0; i < 10; ++i) {
      ctrl.update(0.016f, pos, turnedFwd, turnedRight, up, 10.0f, 0.5f, false, 0.0f, 1.0f, 1.0f, "", glm::vec3(0.0f), 0.0f);
    }

    glm::vec3 headOff = ctrl.getHeadOffset();
    // Offset should be constrained to realistic neck limits <= 0.08m
    if (std::abs(headOff.x) > 0.08f || std::abs(headOff.y) > 0.08f || std::abs(headOff.z) > 0.08f) {
      std::cerr << "Test 2 failed: Head offset out of physical bounds: " << headOff.x << ", " << headOff.y << "\n";
      return 1;
    }
    std::cout << "PASS: Cockpit Test 2 (inertial turn bounded)\n";
  }

  // Test 3: Telemetry HUD pitch and roll calculation
  {
    ctrl.reset();
    glm::vec3 pos(0.0f);
    // 30 degree pitch up: fwd.y = sin(30) = 0.5
    glm::vec3 fwd(0.0f, 0.5f, -std::sqrt(0.75f));
    glm::vec3 right(1.0f, 0.0f, 0.0f);
    glm::vec3 up = glm::normalize(glm::cross(right, fwd));

    ctrl.update(0.016f, pos, fwd, right, up, 45.0f, 0.8f, false, 0.0f, 1.0f, 1.0f, "", glm::vec3(0.0f), 0.0f);
    const CockpitHUDData& hud = ctrl.getHUDData();

    if (std::abs(hud.pitchDeg - 30.0f) > 1.5f) {
      std::cerr << "Test 3 failed: Pitch expected ~30 deg, got " << hud.pitchDeg << "\n";
      return 1;
    }
    if (std::abs(hud.speed - 45.0f) > 0.1f) {
      std::cerr << "Test 3 failed: Speed expected 45, got " << hud.speed << "\n";
      return 1;
    }
    std::cout << "PASS: Cockpit Test 3 (pitch/speed telemetry)\n";
  }

  // Test 4: Target tracking bearing and distance
  {
    ctrl.reset();
    glm::vec3 pos(0.0f, 0.0f, 10.0f);
    glm::vec3 fwd(0.0f, 0.0f, -1.0f);
    glm::vec3 right(1.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 tgtPos(0.0f, 0.0f, -90.0f); // directly ahead at 100 units

    ctrl.update(0.016f, pos, fwd, right, up, 0.0f, 0.0f, false, 0.0f, 1.0f, 1.0f, "Sgr A*", tgtPos, 0.0f);
    const CockpitHUDData& hud = ctrl.getHUDData();

    if (hud.targetName != "Sgr A*") {
      std::cerr << "Test 4 failed: Target name mismatch\n";
      return 1;
    }
    if (std::abs(hud.targetDistLy - 100.0f) > 0.1f) {
      std::cerr << "Test 4 failed: Target distance expected 100, got " << hud.targetDistLy << "\n";
      return 1;
    }
    if (hud.targetAngleDeg > 1.0f || !hud.targetLocked) {
      std::cerr << "Test 4 failed: Target should be locked directly ahead, got angle " << hud.targetAngleDeg << "\n";
      return 1;
    }
    std::cout << "PASS: Cockpit Test 4 (target tracking lock)\n";
  }

  // Test 5: Photochromic auto-tint & rumble
  {
    ctrl.reset();
    glm::vec3 pos(0.0f);
    glm::vec3 fwd(0.0f, 0.0f, -1.0f);
    glm::vec3 right(1.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    // Feed high solar flux and warp drive active
    for (int i = 0; i < 30; ++i) {
      ctrl.update(0.016f, pos, fwd, right, up, 250.0f, 1.5f, true, 0.8f, 0.9f, 0.85f, "", glm::vec3(0.0f), 1.0f);
    }
    const CockpitHUDData& hud = ctrl.getHUDData();

    if (hud.autoTint < 0.5f || hud.autoTint > 1.0f) {
      std::cerr << "Test 5 failed: Auto-tint expected > 0.5 under direct sunlight, got " << hud.autoTint << "\n";
      return 1;
    }
    if (ctrl.getCanopyRumble() < 0.2f) {
      std::cerr << "Test 5 failed: Rumble expected > 0.2 under warp, got " << ctrl.getCanopyRumble() << "\n";
      return 1;
    }
    std::cout << "PASS: Cockpit Test 5 (auto-tint & warp rumble)\n";
  }

  std::cout << "ALL COCKPIT CONTROLLER TESTS PASSED\n";
  return 0;
}
