#include <iostream>
#include <cassert>
#include <cmath>
#include "GamepadSystem.h"

using namespace astro;

static int g_failed = 0;

#define TEST_CHECK(desc, cond) \
  do { \
    if (cond) { \
      std::cout << "[PASS] " << desc << "\n"; \
    } else { \
      std::cerr << "[FAIL] " << desc << " (line " << __LINE__ << ")\n"; \
      g_failed++; \
    } \
  } while (0)

void testDeadzone() {
  std::cout << "--- Test 1: Radial Deadzone Filtering ---\n";
  float ox = 0.0f, oy = 0.0f;

  // Inside deadzone (0.05, 0.05) with deadzone 0.15
  GamepadSystem::applyRadialDeadzone(0.05f, 0.05f, 0.15f, ox, oy);
  TEST_CHECK("Stick inside deadzone produces 0.0", ox == 0.0f && oy == 0.0f);

  // Zero input
  GamepadSystem::applyRadialDeadzone(0.0f, 0.0f, 0.15f, ox, oy);
  TEST_CHECK("Zero input produces 0.0", ox == 0.0f && oy == 0.0f);

  // Maximum deflection along X
  GamepadSystem::applyRadialDeadzone(1.0f, 0.0f, 0.15f, ox, oy);
  TEST_CHECK("Full deflection along X gives 1.0", std::abs(ox - 1.0f) < 1e-4f && oy == 0.0f);

  // Partial deflection: length = 0.5 with deadzone 0.2
  // expected normalized = (0.5 - 0.2) / (1.0 - 0.2) = 0.3 / 0.8 = 0.375
  GamepadSystem::applyRadialDeadzone(0.5f, 0.0f, 0.2f, ox, oy);
  TEST_CHECK("Rescaled partial deflection", std::abs(ox - 0.375f) < 1e-4f);
}

void testTriggerNormalization() {
  std::cout << "--- Test 2: Trigger Normalization ---\n";

  // Raw unpressed is -1.0
  float t0 = GamepadSystem::normalizeTrigger(-1.0f, 0.05f);
  TEST_CHECK("Raw -1.0 maps to 0.0", t0 == 0.0f);

  // Raw slightly noisy around rest (-0.95)
  float tRest = GamepadSystem::normalizeTrigger(-0.95f, 0.05f);
  TEST_CHECK("Near-rest trigger within deadzone maps to 0.0", tRest == 0.0f);

  // Raw fully pressed is +1.0
  float tMax = GamepadSystem::normalizeTrigger(1.0f, 0.05f);
  TEST_CHECK("Raw 1.0 maps to 1.0", std::abs(tMax - 1.0f) < 1e-4f);

  // Raw halfway (0.0) -> (0.0 + 1.0)*0.5 = 0.5, normalized = (0.5 - 0.05)/0.95 = ~0.4736
  float tHalf = GamepadSystem::normalizeTrigger(0.0f, 0.05f);
  TEST_CHECK("Raw 0.0 maps to mid-range throttle", tHalf > 0.45f && tHalf < 0.55f);
}

void testButtonTransitions() {
  std::cout << "--- Test 3: Button Transitions (Down / JustPressed / JustReleased) ---\n";
  GamepadSystem pad;
  pad.setConnected(true);

  // Initial state: not down
  TEST_CHECK("Button A initially not down", !pad.isDown(GamepadButton::A));
  TEST_CHECK("Button A initially not just pressed", !pad.isJustPressed(GamepadButton::A));

  // Frame 1: Press A
  GamepadRawState raw1 = {};
  raw1.buttons[static_cast<int>(GamepadButton::A)] = true;
  pad.injectRawState(raw1);

  TEST_CHECK("Frame 1: Button A is down", pad.isDown(GamepadButton::A));
  TEST_CHECK("Frame 1: Button A is just pressed", pad.isJustPressed(GamepadButton::A));
  TEST_CHECK("Frame 1: Button A is not just released", !pad.isJustReleased(GamepadButton::A));

  // Frame 2: Hold A
  pad.injectRawState(raw1);
  TEST_CHECK("Frame 2: Button A is still down", pad.isDown(GamepadButton::A));
  TEST_CHECK("Frame 2: Button A is NOT just pressed", !pad.isJustPressed(GamepadButton::A));
  TEST_CHECK("Frame 2: Button A is not just released", !pad.isJustReleased(GamepadButton::A));

  // Frame 3: Release A
  GamepadRawState raw2 = {};
  pad.injectRawState(raw2);
  TEST_CHECK("Frame 3: Button A is NOT down", !pad.isDown(GamepadButton::A));
  TEST_CHECK("Frame 3: Button A is NOT just pressed", !pad.isJustPressed(GamepadButton::A));
  TEST_CHECK("Frame 3: Button A IS just released", pad.isJustReleased(GamepadButton::A));
}

void testFlightControl() {
  std::cout << "--- Test 4: Analog Flight Control Mapping ---\n";
  GamepadSystem pad;
  pad.setConnected(true);

  GamepadRawState raw = {};
  // Pull left stick back (LeftY = 0.8) -> Pitch up
  raw.axes[static_cast<int>(GamepadAxis::LeftY)] = 0.8f;
  // Push right stick right (RightX = 0.6) -> Yaw right
  raw.axes[static_cast<int>(GamepadAxis::RightX)] = 0.6f;
  // Full throttle on RightTrigger (1.0)
  raw.axes[static_cast<int>(GamepadAxis::RightTrigger)] = 1.0f;
  // Zero brake on LeftTrigger (-1.0)
  raw.axes[static_cast<int>(GamepadAxis::LeftTrigger)] = -1.0f;

  pad.injectRawState(raw);
  const auto& flight = pad.getFlightControl();

  TEST_CHECK("Pitch responds to stick pull back", flight.pitch < -0.5f); // pull back is negative standard flight pitch down/up
  TEST_CHECK("Yaw responds to right stick", flight.yaw > 0.4f);
  TEST_CHECK("Throttle responds to right trigger", std::abs(flight.throttle - 1.0f) < 1e-4f);
  TEST_CHECK("Reverse is 0.0 when left trigger unpressed", flight.reverse == 0.0f);

  // Test pitch inversion
  pad.setInvertPitch(true);
  pad.injectRawState(raw);
  TEST_CHECK("Pitch inverts when setInvertPitch is active", pad.getFlightControl().pitch > 0.5f);
}

void testGlyphsAndAdaptivePrompts() {
  std::cout << "--- Test 5: Controller Glyphs and Adaptive Prompts ---\n";
  // Xbox glyphs
  TEST_CHECK("Xbox A glyph is [A]", std::string(GamepadSystem::getButtonGlyph(GamepadButton::A, GamepadStyle::Xbox)) == "[A]");
  TEST_CHECK("Xbox Y glyph is [Y]", std::string(GamepadSystem::getButtonGlyph(GamepadButton::Y, GamepadStyle::Xbox)) == "[Y]");
  TEST_CHECK("Xbox LB glyph is [LB]", std::string(GamepadSystem::getButtonGlyph(GamepadButton::LB, GamepadStyle::Xbox)) == "[LB]");

  // PlayStation glyphs
  TEST_CHECK("PlayStation A glyph is (X)", std::string(GamepadSystem::getButtonGlyph(GamepadButton::A, GamepadStyle::PlayStation)) == "(X)");
  TEST_CHECK("PlayStation B glyph is (O)", std::string(GamepadSystem::getButtonGlyph(GamepadButton::B, GamepadStyle::PlayStation)) == "(O)");
  TEST_CHECK("PlayStation LB glyph is [L1]", std::string(GamepadSystem::getButtonGlyph(GamepadButton::LB, GamepadStyle::PlayStation)) == "[L1]");

  // Steam Deck glyphs
  TEST_CHECK("SteamDeck Back is [View]", std::string(GamepadSystem::getButtonGlyph(GamepadButton::Back, GamepadStyle::SteamDeck)) == "[View]");
  TEST_CHECK("SteamDeck Guide is [Steam]", std::string(GamepadSystem::getButtonGlyph(GamepadButton::Guide, GamepadStyle::SteamDeck)) == "[Steam]");

  // Adaptive prompts
  GamepadSystem pad;
  pad.setConnected(true);
  pad.setGamepadActive(false);

  // When gamepad inactive: returns keyboard prompt
  std::string p1 = pad.getAdaptivePrompt("[E] Scan Target", GamepadButton::A, "Scan Target");
  TEST_CHECK("Adaptive prompt returns keyboard prompt when inactive", p1 == "[E] Scan Target");

  // When gamepad active: returns controller prompt
  pad.setGamepadActive(true);
  std::string p2 = pad.getAdaptivePrompt("[E] Scan Target", GamepadButton::A, "Scan Target");
  TEST_CHECK("Adaptive prompt returns gamepad prompt when active", p2 == "[A] Scan Target");

  // Mouse/Keyboard used notification switches back
  pad.notifyKeyboardMouseUsed();
  TEST_CHECK("notifyKeyboardMouseUsed deactivates gamepad prompt", !pad.isGamepadActive());
}

int main() {
  std::cout << "=== Astro-Voyager GamepadSystem Test Suite ===\n";
  testDeadzone();
  testTriggerNormalization();
  testButtonTransitions();
  testFlightControl();
  testGlyphsAndAdaptivePrompts();

  if (g_failed == 0) {
    std::cout << "\nAll GamepadSystem tests passed successfully (0 failures)!\n";
    return 0;
  } else {
    std::cerr << "\n" << g_failed << " test(s) failed!\n";
    return 1;
  }
}
