#pragma once

namespace astro {
namespace interaction {

bool shouldCaptureMouseForCamera(bool mousePressed, bool uiWantsMouse);
bool shouldProcessKeyboardMovement(bool uiWantsKeyboard);
float clampCameraRadius(float radius, float minRadius);

}  // namespace interaction
}  // namespace astro

// BlackHole compatibility namespace
namespace blackhole {

inline bool ShouldCaptureMouseForCamera(bool mousePressed, bool uiWantsMouse) {
  return astro::interaction::shouldCaptureMouseForCamera(mousePressed, uiWantsMouse);
}

inline bool ShouldProcessKeyboardMovement(bool uiWantsKeyboard) {
  return astro::interaction::shouldProcessKeyboardMovement(uiWantsKeyboard);
}

inline float ClampCameraRadius(float radius, float minRadius) {
  return astro::interaction::clampCameraRadius(radius, minRadius);
}

}  // namespace blackhole
