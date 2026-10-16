#include "../src/Interaction.h"
#include "../src/Types.h"  // v3.3.0: ShipState / astro::ship balance logic

#include <cmath>
#include <iostream>
#include <cstdlib>

namespace {

void AssertNear(float actual, float expected, float tolerance, const char* label) {
  if (std::abs(actual - expected) > tolerance) {
    std::cerr << label << " expected " << expected << " but got " << actual << '\n';
    std::exit(1);
  }
}

}  // namespace

int main() {
  // Test 1: mouse capture when UI does not need mouse
  if (!blackhole::ShouldCaptureMouseForCamera(true, false)) {
    std::cerr << "left mouse button should enable camera look when UI does not need mouse\n";
    return 1;
  }

  // Test 2: mouse capture off when button is released
  if (blackhole::ShouldCaptureMouseForCamera(false, false)) {
    std::cerr << "camera look should be off while left mouse button is released\n";
    return 1;
  }

  // Test 3: mouse capture off when UI wants mouse
  if (blackhole::ShouldCaptureMouseForCamera(true, true)) {
    std::cerr << "camera look should stay off while UI needs mouse input\n";
    return 1;
  }

  // Test 4: keyboard movement when UI does not need keyboard
  if (!blackhole::ShouldProcessKeyboardMovement(false)) {
    std::cerr << "keyboard movement should work when UI does not need keyboard\n";
    return 1;
  }

  // Test 5: keyboard movement blocked when UI needs keyboard
  if (blackhole::ShouldProcessKeyboardMovement(true)) {
    std::cerr << "keyboard movement should pause while UI needs keyboard input\n";
    return 1;
  }

  // Test 6: clamp radius
  AssertNear(blackhole::ClampCameraRadius(0.4f, 1.05f), 1.05f, 0.0001f, "radius below horizon");
  AssertNear(blackhole::ClampCameraRadius(8.0f, 1.05f), 8.0f, 0.0001f, "radius outside horizon");
  AssertNear(blackhole::ClampCameraRadius(31.0f, 1.05f), 31.0f, 0.0001f, "radius beyond shell");

  // Test 7 (v3.3.0): warp jump costs 5 fuel and is blocked under 5.
  astro::ShipState jumpShip;  // 100/100 defaults
  AssertNear(jumpShip.fuel, 100.0f, 0.0001f, "fresh ship fuel");
  AssertNear(jumpShip.shield, 100.0f, 0.0001f, "fresh ship shield");
  if (!astro::ship::canJump(jumpShip)) {
    std::cerr << "jump should be allowed at full fuel\n";
    return 1;
  }
  if (!astro::ship::spendJumpFuel(jumpShip)) {
    std::cerr << "jump fuel spend should succeed at full fuel\n";
    return 1;
  }
  AssertNear(jumpShip.fuel, 95.0f, 0.0001f, "fuel after one jump");
  jumpShip.fuel = 4.0f;
  if (astro::ship::canJump(jumpShip)) {
    std::cerr << "jump must be blocked below 5 fuel\n";
    return 1;
  }
  if (astro::ship::spendJumpFuel(jumpShip)) {
    std::cerr << "blocked jump must not spend fuel\n";
    return 1;
  }
  AssertNear(jumpShip.fuel, 4.0f, 0.0001f, "fuel after blocked jump");

  // Test 8 (v3.3.0): pulsar radiation drains 10/s, star proximity refuels 2/s.
  astro::ShipState zoneShip;
  astro::ship::applyRadiation(zoneShip, 2.5f, true);
  AssertNear(zoneShip.shield, 75.0f, 0.0001f, "shield after 2.5s in pulsar zone");
  astro::ship::applyRadiation(zoneShip, 1.0f, false);
  AssertNear(zoneShip.shield, 75.0f, 0.0001f, "shield unchanged outside zone");
  zoneShip.fuel = 50.0f;
  astro::ship::updateFuel(zoneShip, 3.0f, true);
  AssertNear(zoneShip.fuel, 56.0f, 0.0001f, "fuel regenerated near a star");
  astro::ship::updateFuel(zoneShip, 3.0f, false);
  AssertNear(zoneShip.fuel, 56.0f, 0.0001f, "no regen away from a star");
  astro::ship::updateFuel(zoneShip, 100.0f, true);
  AssertNear(zoneShip.fuel, 100.0f, 0.0001f, "fuel capped at 100");
  zoneShip.shield = 2.0f;
  astro::ship::applyRadiation(zoneShip, 5.0f, true);
  AssertNear(zoneShip.shield, 0.0f, 0.0001f, "shield floors at zero");

  // Test 9 (v3.3.0): death triggers the respawn balance (50 shield / 30 fuel).
  astro::ShipState deadShip;
  deadShip.shield = 0.0f;
  if (!astro::ship::isDead(deadShip)) {
    std::cerr << "zero shield must count as death\n";
    return 1;
  }
  deadShip.gravVel = astro::Vec3(1.0, 2.0, 3.0);
  astro::ship::respawn(deadShip);
  AssertNear(deadShip.shield, 50.0f, 0.0001f, "respawn shield");
  AssertNear(deadShip.fuel, 30.0f, 0.0001f, "respawn fuel");
  AssertNear(static_cast<float>(deadShip.gravVel.x), 0.0f, 0.0001f,
             "respawn drift reset");
  if (astro::ship::isDead(deadShip)) {
    std::cerr << "respawned ship must be alive\n";
    return 1;
  }

  // Test 10 (v3.3.0): low-shield postfx gates and Newtonian falloff.
  AssertNear(astro::ship::chromaK(100.0f), 0.0f, 0.0001f, "no chroma at full shield");
  AssertNear(astro::ship::chromaK(30.0f), 0.0f, 0.0001f, "chroma gate at 30");
  AssertNear(astro::ship::chromaK(15.0f), 0.5f, 0.0001f, "chroma half at 15");
  AssertNear(astro::ship::staticK(10.0f), 0.0f, 0.0001f, "static gate at 10");
  AssertNear(astro::ship::staticK(5.0f), 0.5f, 0.0001f, "static half at 5");
  AssertNear(static_cast<float>(astro::ship::gravAccel(40.0, 2.0)), 10.0f,
             0.0001f, "GM/r^2 at 2 units");
  AssertNear(static_cast<float>(astro::ship::gravAccel(40.0, 0.1)), 160.0f,
             0.0001f, "GM/r^2 clamps singular distance");

  std::cout << "All BlackHoleInteraction tests passed!\n";
  return 0;
}
