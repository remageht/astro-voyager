#include "Interaction.h"

namespace astro {
namespace interaction {

bool shouldCaptureMouseForCamera(bool mousePressed, bool uiWantsMouse) {
  return mousePressed && !uiWantsMouse;
}

bool shouldProcessKeyboardMovement(bool uiWantsKeyboard) {
  return !uiWantsKeyboard;
}

float clampCameraRadius(float radius, float minRadius) {
  if (radius < minRadius) {
    return minRadius;
  }
  return radius;
}

}  // namespace interaction
}  // namespace astro
