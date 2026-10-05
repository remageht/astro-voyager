#include "../src/Interaction.h"

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

  std::cout << "All BlackHoleInteraction tests passed!\n";
  return 0;
}
