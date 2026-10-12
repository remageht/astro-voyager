#include "Interaction.h"

#include <cmath>

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

Vec3 pushOutOfSphere(Vec3 pos, double cx, double cy, double cz, double minDist) {
  if (minDist <= 0.0) return pos;
  double dx = pos.x - cx;
  double dy = pos.y - cy;
  double dz = pos.z - cz;
  double len = std::sqrt(dx * dx + dy * dy + dz * dz);
  if (len >= minDist) return pos;
  if (len < 1e-9) {
    pos.x = cx + minDist;
    pos.y = cy;
    pos.z = cz;
    return pos;
  }
  double k = minDist / len;
  pos.x = cx + dx * k;
  pos.y = cy + dy * k;
  pos.z = cz + dz * k;
  return pos;
}

}  // namespace interaction
}  // namespace astro
