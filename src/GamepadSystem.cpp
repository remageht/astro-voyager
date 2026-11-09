#include "GamepadSystem.h"

#ifdef ASTROVOYAGER_ENABLE_GL
#include <GLFW/glfw3.h>
#endif

namespace astro {

GamepadSystem::GamepadSystem() {
  m_curRaw = {};
  m_prevRaw = {};
  m_flight = {};
}

void GamepadSystem::applyRadialDeadzone(float inX, float inY, float deadzone, float& outX, float& outY) {
  const float len = std::sqrt(inX * inX + inY * inY);
  if (len <= deadzone || len < 1e-5f) {
    outX = 0.0f;
    outY = 0.0f;
    return;
  }
  const float normalizedLen = std::min(1.0f, (len - deadzone) / (1.0f - deadzone));
  outX = (inX / len) * normalizedLen;
  outY = (inY / len) * normalizedLen;
}

float GamepadSystem::normalizeTrigger(float rawAxis, float deadzone) {
  const float clamped = std::max(-1.0f, std::min(1.0f, rawAxis));
  const float val = (clamped + 1.0f) * 0.5f; // [-1.0, 1.0] -> [0.0, 1.0]
  if (val <= deadzone) {
    return 0.0f;
  }
  return std::min(1.0f, (val - deadzone) / (1.0f - deadzone));
}

void GamepadSystem::injectRawState(const GamepadRawState& raw) {
  m_prevRaw = m_curRaw;
  m_curRaw = raw;

  // Process axes through deadzones
  float leftX = 0.0f;
  float leftY = 0.0f;
  applyRadialDeadzone(
    m_curRaw.axes[static_cast<int>(GamepadAxis::LeftX)],
    m_curRaw.axes[static_cast<int>(GamepadAxis::LeftY)],
    m_stickDeadzone,
    leftX, leftY
  );

  float rightX = 0.0f;
  float rightY = 0.0f;
  applyRadialDeadzone(
    m_curRaw.axes[static_cast<int>(GamepadAxis::RightX)],
    m_curRaw.axes[static_cast<int>(GamepadAxis::RightY)],
    m_stickDeadzone,
    rightX, rightY
  );

  const float rawLT = m_curRaw.axes[static_cast<int>(GamepadAxis::LeftTrigger)];
  const float rawRT = m_curRaw.axes[static_cast<int>(GamepadAxis::RightTrigger)];
  const float normLT = normalizeTrigger(rawLT, m_triggerDeadzone);
  const float normRT = normalizeTrigger(rawRT, m_triggerDeadzone);

  // Flight controls:
  // In GLFW: stick push forward is negative Y, pulling backward is positive Y
  // Standard flight model: pulling back pitches up (+), pushing forward pitches down (-)
  float pitchInput = -leftY;
  if (m_invertPitch) {
    pitchInput = -pitchInput;
  }

  m_flight.pitch = pitchInput;
  m_flight.roll = leftX;
  m_flight.yaw = rightX;
  m_flight.lookPitch = -rightY;
  m_flight.lookYaw = rightX;
  m_flight.throttle = normRT;
  m_flight.reverse = normLT;
  m_flight.strafeX = leftX;
  m_flight.strafeY = -leftY;

  // Activity detection: activate gamepad mode if sticks, triggers, or buttons are actively used
  bool hasInput = false;
  if (std::abs(leftX) > 0.01f || std::abs(leftY) > 0.01f ||
      std::abs(rightX) > 0.01f || std::abs(rightY) > 0.01f ||
      normLT > 0.01f || normRT > 0.01f) {
    hasInput = true;
  } else {
    for (int i = 0; i < static_cast<int>(GamepadButton::Count); ++i) {
      if (m_curRaw.buttons[i]) {
        hasInput = true;
        break;
      }
    }
  }

  if (hasInput) {
    m_gamepadActive = true;
    m_connected = true;
  }
}

void GamepadSystem::update(float dt) {
  (void)dt;
  // Re-evaluates state decay if needed
}

#ifdef ASTROVOYAGER_ENABLE_GL
bool GamepadSystem::pollGlfw(int joystickId) {
  if (!glfwJoystickPresent(joystickId) || !glfwJoystickIsGamepad(joystickId)) {
    m_connected = false;
    return false;
  }

  m_connected = true;
  GLFWgamepadstate state;
  if (!glfwGetGamepadState(joystickId, &state)) {
    return false;
  }

  GamepadRawState raw = {};
  for (int i = 0; i < static_cast<int>(GamepadButton::Count); ++i) {
    raw.buttons[i] = (state.buttons[i] == GLFW_PRESS);
  }
  for (int i = 0; i < static_cast<int>(GamepadAxis::Count); ++i) {
    raw.axes[i] = state.axes[i];
  }

  injectRawState(raw);
  return true;
}
#endif

bool GamepadSystem::isDown(GamepadButton btn) const {
  const int idx = static_cast<int>(btn);
  if (idx < 0 || idx >= static_cast<int>(GamepadButton::Count)) return false;
  return m_curRaw.buttons[idx];
}

bool GamepadSystem::isJustPressed(GamepadButton btn) const {
  const int idx = static_cast<int>(btn);
  if (idx < 0 || idx >= static_cast<int>(GamepadButton::Count)) return false;
  return m_curRaw.buttons[idx] && !m_prevRaw.buttons[idx];
}

bool GamepadSystem::isJustReleased(GamepadButton btn) const {
  const int idx = static_cast<int>(btn);
  if (idx < 0 || idx >= static_cast<int>(GamepadButton::Count)) return false;
  return !m_curRaw.buttons[idx] && m_prevRaw.buttons[idx];
}

const char* GamepadSystem::getButtonGlyph(GamepadButton btn, GamepadStyle style) {
  if (style == GamepadStyle::PlayStation) {
    switch (btn) {
      case GamepadButton::A: return "(X)";
      case GamepadButton::B: return "(O)";
      case GamepadButton::X: return "([ ])";
      case GamepadButton::Y: return "(^)";
      case GamepadButton::LB: return "[L1]";
      case GamepadButton::RB: return "[R1]";
      case GamepadButton::Back: return "[Share]";
      case GamepadButton::Start: return "[Options]";
      case GamepadButton::Guide: return "[PS]";
      case GamepadButton::LThumb: return "[L3]";
      case GamepadButton::RThumb: return "[R3]";
      case GamepadButton::DpadUp: return "[D-Up]";
      case GamepadButton::DpadRight: return "[D-Right]";
      case GamepadButton::DpadDown: return "[D-Down]";
      case GamepadButton::DpadLeft: return "[D-Left]";
      default: return "[?]";
    }
  }

  // Xbox / SteamDeck default
  switch (btn) {
    case GamepadButton::A: return "[A]";
    case GamepadButton::B: return "[B]";
    case GamepadButton::X: return "[X]";
    case GamepadButton::Y: return "[Y]";
    case GamepadButton::LB: return "[LB]";
    case GamepadButton::RB: return "[RB]";
    case GamepadButton::Back: return (style == GamepadStyle::SteamDeck ? "[View]" : "[Back]");
    case GamepadButton::Start: return (style == GamepadStyle::SteamDeck ? "[Menu]" : "[Start]");
    case GamepadButton::Guide: return (style == GamepadStyle::SteamDeck ? "[Steam]" : "[Guide]");
    case GamepadButton::LThumb: return "[LS]";
    case GamepadButton::RThumb: return "[RS]";
    case GamepadButton::DpadUp: return "[D-Up]";
    case GamepadButton::DpadRight: return "[D-Right]";
    case GamepadButton::DpadDown: return "[D-Down]";
    case GamepadButton::DpadLeft: return "[D-Left]";
    default: return "[?]";
  }
}

std::string GamepadSystem::getAdaptivePrompt(
    const std::string& kbPrompt, GamepadButton btn, const std::string& action) const {
  if (isGamepadActive()) {
    return std::string(getButtonGlyph(btn, m_style)) + " " + action;
  }
  return kbPrompt;
}

}  // namespace astro
