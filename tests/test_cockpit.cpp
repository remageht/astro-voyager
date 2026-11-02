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

  // Test 6: Prograde & Retrograde velocity vector projections (v4.6.0)
  {
    ctrl.reset();
    glm::vec3 pos(0.0f);
    glm::vec3 fwd(0.0f, 0.0f, -1.0f);
    glm::vec3 right(1.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    // Ship moving forward and drifting to the right: vx = +6.0, vz = -12.0
    glm::vec3 driftVel(6.0f, 0.0f, -12.0f);
    ctrl.update(0.016f, pos, fwd, right, up, 13.4f, 0.5f, false, 0.0f, 1.0f, 1.0f, "", glm::vec3(0.0f), 0.0f,
                driftVel);
    const CockpitHUDData& hud = ctrl.getHUDData();

    if (!hud.progradeVisible) {
      std::cerr << "Test 6 failed: Prograde marker should be visible when drifting forward\n";
      return 1;
    }
    if (hud.progradeScreenPos.x <= 0.0f) {
      std::cerr << "Test 6 failed: Prograde marker should project to the right (x > 0), got " << hud.progradeScreenPos.x << "\n";
      return 1;
    }

    // Ship burning in reverse (moving backward: vz = +15.0)
    glm::vec3 reverseVel(0.0f, 0.0f, 15.0f);
    ctrl.update(0.016f, pos, fwd, right, up, 15.0f, 0.5f, false, 0.0f, 1.0f, 1.0f, "", glm::vec3(0.0f), 0.0f,
                reverseVel);
    const CockpitHUDData& hudRetro = ctrl.getHUDData();

    if (!hudRetro.retrogradeVisible) {
      std::cerr << "Test 6 failed: Retrograde marker should be visible when moving backwards\n";
      return 1;
    }
    if (std::abs(hudRetro.retrogradeScreenPos.x) > 0.1f || std::abs(hudRetro.retrogradeScreenPos.y) > 0.1f) {
      std::cerr << "Test 6 failed: Retrograde marker should be centered ahead during reverse burn\n";
      return 1;
    }
    std::cout << "PASS: Cockpit Test 6 (prograde & retrograde vectors)\n";
  }

  // Test 7: G-force tracking & physiological blackout/redout (v4.6.0)
  {
    ctrl.reset();
    glm::vec3 pos(0.0f);
    glm::vec3 right(1.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 fwd(0.0f, 0.0f, -1.0f);

    // Initial seed
    ctrl.update(0.016f, pos, fwd, right, up, 50.0f, 0.5f, false, 0.0f, 1.0f, 1.0f, "", glm::vec3(0.0f), 0.0f);

    // High-G turn with boost throttle
    glm::vec3 hardTurnFwd = glm::normalize(glm::vec3(0.6f, 0.0f, -0.8f));
    glm::vec3 hardTurnRight = glm::normalize(glm::cross(hardTurnFwd, up));
    for (int i = 0; i < 20; ++i) {
      ctrl.update(0.016f, pos, hardTurnFwd, hardTurnRight, up, 100.0f, 1.8f, false, 0.0f, 1.0f, 1.0f, "", glm::vec3(0.0f), 0.0f);
    }
    const CockpitHUDData& hudHighG = ctrl.getHUDData();

    if (hudHighG.gForce < 2.5f) {
      std::cerr << "Test 7 failed: G-force expected > 2.5G under high-speed hard turn, got " << hudHighG.gForce << "\n";
      return 1;
    }
    if (hudHighG.blackoutK < 0.0f || hudHighG.blackoutK > 1.0f) {
      std::cerr << "Test 7 failed: Blackout factor out of bounds: " << hudHighG.blackoutK << "\n";
      return 1;
    }

    // Violent pitch down for negative-G redout
    glm::vec3 pitchDownFwd = glm::normalize(glm::vec3(0.0f, -0.7f, -0.7f));
    glm::vec3 pitchDownUp = glm::normalize(glm::cross(right, pitchDownFwd));
    for (int i = 0; i < 20; ++i) {
      ctrl.update(0.016f, pos, pitchDownFwd, right, pitchDownUp, 80.0f, 0.5f, false, 0.0f, 1.0f, 1.0f, "", glm::vec3(0.0f), 0.0f);
    }
    const CockpitHUDData& hudRedout = ctrl.getHUDData();
    if (hudRedout.redoutK < 0.0f || hudRedout.redoutK > 1.0f) {
      std::cerr << "Test 7 failed: Redout factor out of bounds: " << hudRedout.redoutK << "\n";
      return 1;
    }
    std::cout << "PASS: Cockpit Test 7 (g-force & physiological strain)\n";
  }

  // Test 8: Radar altimeter AGL & terrain clearance alert (v4.6.0)
  {
    ctrl.reset();
    glm::vec3 bodyPos(0.0f);
    float bodyRadius = 10.0f; // planet radius = 10

    // Ship at altitude 1.2 units above surface (distance = 11.2)
    glm::vec3 shipPos(0.0f, 0.0f, 11.2f);
    glm::vec3 fwd(0.0f, 0.0f, -1.0f); // nose toward surface
    glm::vec3 right(1.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    // Rapid descent toward surface: vy = 0, vz = -6.0
    glm::vec3 descentVel(0.0f, 0.0f, -6.0f);

    ctrl.update(0.016f, shipPos, fwd, right, up, 6.0f, 0.5f, false, 0.0f, 1.0f, 1.0f, "Earth", glm::vec3(0.0f), 0.0f,
                descentVel, bodyPos, bodyRadius, true);
    const CockpitHUDData& hud = ctrl.getHUDData();

    if (hud.radarAltitudeKm < 0.0f) {
      std::cerr << "Test 8 failed: Radar altimeter should be active near planetary surface\n";
      return 1;
    }
    if (hud.verticalSpeedMs >= 0.0f) {
      std::cerr << "Test 8 failed: Vertical speed should be negative during descent\n";
      return 1;
    }
    if (!hud.terrainAlert) {
      std::cerr << "Test 8 failed: Terrain clearance alert should trigger on dangerous low-altitude descent\n";
      return 1;
    }
    std::cout << "PASS: Cockpit Test 8 (radar altimeter & terrain alert)\n";
  }

  // Test 9: Atmospheric hypersonic entry plasma ionization (v4.6.0)
  {
    ctrl.reset();
    glm::vec3 bodyPos(0.0f);
    float bodyRadius = 20.0f;
    // Inside atmospheric layer at altitude 1.0 (atm thickness ~4.4)
    glm::vec3 entryPos(0.0f, 0.0f, 21.0f);
    glm::vec3 fwd(0.0f, 0.0f, -1.0f);
    glm::vec3 right(1.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    // Hypersonic speed: 30 units/s
    glm::vec3 hypersonicVel(0.0f, 0.0f, -30.0f);

    for (int i = 0; i < 25; ++i) {
      ctrl.update(0.016f, entryPos, fwd, right, up, 30.0f, 1.0f, false, 0.0f, 1.0f, 1.0f, "Jupiter", glm::vec3(0.0f), 0.0f,
                  hypersonicVel, bodyPos, bodyRadius, true);
    }
    const CockpitHUDData& hud = ctrl.getHUDData();

    if (hud.entryPlasma < 0.15f || hud.entryPlasma > 1.0f) {
      std::cerr << "Test 9 failed: Entry plasma expected > 0.15 under hypersonic entry, got " << hud.entryPlasma << "\n";
      return 1;
    }
    std::cout << "PASS: Cockpit Test 9 (hypersonic atmospheric entry plasma)\n";
  }

  std::cout << "ALL COCKPIT CONTROLLER TESTS PASSED\n";
  return 0;
}
