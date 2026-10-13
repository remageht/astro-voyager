#pragma once

#include "Camera.h"

namespace astro {
namespace interaction {

bool shouldCaptureMouseForCamera(bool mousePressed, bool uiWantsMouse);
bool shouldProcessKeyboardMovement(bool uiWantsKeyboard);
float clampCameraRadius(float radius, float minRadius);
Vec3 pushOutOfSphere(Vec3 pos, double cx, double cy, double cz, double minDist);
// PH slide: grazes a solid sphere instead of entering it. Removes only the
// inward normal component of the step prev->cur (tangential part kept, so
// no sticking), then projects to the surface if still inside (no popping).
// Returns cur unchanged when already outside or radius <= 0.
Vec3 slideOnSphere(Vec3 prevPos, Vec3 curPos, double cx, double cy, double cz,
                   double radius);

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

inline astro::Vec3 SlideOnSphere(astro::Vec3 prevPos, astro::Vec3 curPos, double cx, double cy,
                          double cz, double radius) {
  return astro::interaction::slideOnSphere(prevPos, curPos, cx, cy, cz, radius);
}

}  // namespace blackhole
