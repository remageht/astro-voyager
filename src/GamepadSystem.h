#pragma once

#include <string>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace astro {

enum class GamepadButton : int {
  A = 0,
  B = 1,
  X = 2,
  Y = 3,
  LB = 4,
  RB = 5,
  Back = 6,
  Start = 7,
  Guide = 8,
  LThumb = 9,
  RThumb = 10,
  DpadUp = 11,
  DpadRight = 12,
  DpadDown = 13,
  DpadLeft = 14,
  Count = 15
};

enum class GamepadAxis : int {
  LeftX = 0,
  LeftY = 1,
  RightX = 2,
  RightY = 3,
  LeftTrigger = 4,
  RightTrigger = 5,
  Count = 6
};

enum class GamepadStyle {
  Xbox,
  PlayStation,
  SteamDeck,
  Generic
};

struct GamepadRawState {
  bool buttons[static_cast<int>(GamepadButton::Count)] = {false};
  float axes[static_cast<int>(GamepadAxis::Count)] = {0.0f};
};

struct GamepadFlightControl {
  float pitch = 0.0f;     // [-1.0, 1.0]: positive = pitch up, negative = pitch down
  float yaw = 0.0f;       // [-1.0, 1.0]: positive = yaw right, negative = yaw left
  float roll = 0.0f;      // [-1.0, 1.0]: positive = roll right, negative = roll left
  float throttle = 0.0f;  // [0.0, 1.0]: right trigger forward thrust
  float reverse = 0.0f;   // [0.0, 1.0]: left trigger reverse thrust / brake
  float lookPitch = 0.0f; // [-1.0, 1.0]: camera free-look pitch
  float lookYaw = 0.0f;   // [-1.0, 1.0]: camera free-look yaw
  float strafeX = 0.0f;   // [-1.0, 1.0]: lateral strafe (left/right)
  float strafeY = 0.0f;   // [-1.0, 1.0]: vertical strafe (up/down)
};

class GamepadSystem {
public:
  GamepadSystem();

  // Configuration
  void setStickDeadzone(float dz) { m_stickDeadzone = std::max(0.01f, std::min(0.5f, dz)); }
  float getStickDeadzone() const { return m_stickDeadzone; }

  void setTriggerDeadzone(float dz) { m_triggerDeadzone = std::max(0.01f, std::min(0.5f, dz)); }
  float getTriggerDeadzone() const { return m_triggerDeadzone; }

  void setInvertPitch(bool inv) { m_invertPitch = inv; }
  bool getInvertPitch() const { return m_invertPitch; }

  void setGamepadStyle(GamepadStyle style) { m_style = style; }
  GamepadStyle getGamepadStyle() const { return m_style; }

  // State Updates
  void update(float dt);
  void injectRawState(const GamepadRawState& raw);

#ifdef ASTROVOYAGER_ENABLE_GL
  // Polls standard GLFW gamepad state for joystick index 0..15 (defaults to GLFW_JOYSTICK_1)
  bool pollGlfw(int joystickId = 0);
#endif

  // Connection & Activity
  bool isConnected() const { return m_connected; }
  void setConnected(bool conn) { m_connected = conn; }

  bool isGamepadActive() const { return m_gamepadActive && m_connected; }
  void setGamepadActive(bool active) { m_gamepadActive = active; }
  void notifyKeyboardMouseUsed() { m_gamepadActive = false; }

  // Button Queries
  bool isDown(GamepadButton btn) const;
  bool isJustPressed(GamepadButton btn) const;
  bool isJustReleased(GamepadButton btn) const;

  // Normalized Axis & Flight Math
  static void applyRadialDeadzone(float inX, float inY, float deadzone, float& outX, float& outY);
  static float normalizeTrigger(float rawAxis, float deadzone = 0.05f);

  const GamepadFlightControl& getFlightControl() const { return m_flight; }

  // Glyphs & Adaptive Prompts
  static const char* getButtonGlyph(GamepadButton btn, GamepadStyle style = GamepadStyle::Xbox);
  std::string getAdaptivePrompt(const std::string& kbPrompt, GamepadButton btn, const std::string& action) const;
  std::string formatPrompt(const std::string& token) const;
  std::string formatPrompt(GamepadButton btn) const;

private:
  float m_stickDeadzone = 0.15f;
  float m_triggerDeadzone = 0.05f;
  bool m_invertPitch = false;
  bool m_connected = false;
  bool m_gamepadActive = false;
  GamepadStyle m_style = GamepadStyle::Xbox;

  GamepadRawState m_curRaw;
  GamepadRawState m_prevRaw;
  GamepadFlightControl m_flight;
};

}  // namespace astro
