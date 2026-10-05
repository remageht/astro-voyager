#include "Camera.h"
#include <cmath>

namespace astro {

Vec3 Camera::forward() const {
  const double cy = std::cos(yaw), sy = std::sin(yaw);
  const double cp = std::cos(pitch), sp = std::sin(pitch);
  return Vec3(-sy * cp, sp, -cy * cp);
}

Vec3 Camera::right() const {
  const double cy = std::cos(yaw), sy = std::sin(yaw);
  return Vec3(cy, 0.0, -sy);
}

Vec3 Camera::up() const {
  Vec3 f = forward();
  Vec3 r = right();
  // up = right x forward (right-handed)
  return Vec3(r.y * f.z - r.z * f.y, r.z * f.x - r.x * f.z,
              r.x * f.y - r.y * f.x);
}

void Camera::moveForward(double dt, bool fwd) {
  Vec3 f = forward();
  const double s = (fwd ? 1.0 : -1.0) * moveSpeed * dt;
  position = Vec3(position.x + f.x * s, position.y + f.y * s,
                  position.z + f.z * s);
}

void Camera::moveRight(double dt, bool rgt) {
  Vec3 r = right();
  const double s = (rgt ? 1.0 : -1.0) * moveSpeed * dt;
  position = Vec3(position.x + r.x * s, position.y + r.y * s,
                  position.z + r.z * s);
}

void Camera::clampNearHorizon(double rs) {
  const double minR = rs * 1.05;
  const double r =
      std::sqrt(position.x * position.x + position.y * position.y +
                position.z * position.z);
  if (r < minR && r > 1e-9) {
    const double k = minR / r;
    position = Vec3(position.x * k, position.y * k, position.z * k);
  }
}

}  // namespace astro
