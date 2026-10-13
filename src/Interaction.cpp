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

Vec3 slideOnSphere(Vec3 prevPos, Vec3 curPos, double cx, double cy, double cz,
                   double radius) {
  if (radius <= 0.0) return curPos;
  double dx = curPos.x - cx;
  double dy = curPos.y - cy;
  double dz = curPos.z - cz;
  double len = std::sqrt(dx * dx + dy * dy + dz * dz);
  if (len >= radius) return curPos;
  double nx, ny, nz;
  if (len < 1e-9) {
    // Degenerate: exactly at the center, pick +X as the normal.
    nx = 1.0;
    ny = 0.0;
    nz = 0.0;
  } else {
    nx = dx / len;
    ny = dy / len;
    nz = dz / len;
  }
  // Kill only the inward normal part of the step; tangential motion (slide)
  // survives, so the camera grazes instead of sticking or bouncing.
  double sx = curPos.x - prevPos.x;
  double sy = curPos.y - prevPos.y;
  double sz = curPos.z - prevPos.z;
  double vn = sx * nx + sy * ny + sz * nz;
  if (vn < 0.0) {
    curPos.x -= nx * vn;
    curPos.y -= ny * vn;
    curPos.z -= nz * vn;
  }
  // Numerical safety: after the slide the point can still sit inside
  // (deep step, grazing angle) — project to the surface, no overshoot.
  dx = curPos.x - cx;
  dy = curPos.y - cy;
  dz = curPos.z - cz;
  len = std::sqrt(dx * dx + dy * dy + dz * dz);
  if (len < radius) {
    if (len < 1e-9) {
      curPos.x = cx + radius;
      curPos.y = cy;
      curPos.z = cz;
    } else {
      double k = radius / len;
      curPos.x = cx + dx * k;
      curPos.y = cy + dy * k;
      curPos.z = cz + dz * k;
    }
  }
  return curPos;
}

}  // namespace interaction
}  // namespace astro
