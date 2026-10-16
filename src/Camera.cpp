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

// === Ship survival === (v3.3.0): gravitational drift.
// Drift velocity integrates the pull, is damped and capped, and moves the
// camera: a gentle "being pulled" feel without fighting the free-flight
// controls (they keep modifying position directly, as before).
void Camera::applyGravityStep(Vec3& driftVel, const Vec3& accel, double dt) {
  driftVel = Vec3(driftVel.x + accel.x * dt, driftVel.y + accel.y * dt,
                  driftVel.z + accel.z * dt);
  const double damp = 1.0 - ship::kGravDampPerSec * dt;
  const double k = damp < 0.0 ? 0.0 : damp;
  double vx = driftVel.x * k, vy = driftVel.y * k, vz = driftVel.z * k;
  const double v =
      std::sqrt(vx * vx + vy * vy + vz * vz);
  if (v > ship::kGravVelMax && v > 1e-9) {
    const double s = ship::kGravVelMax / v;
    vx *= s;
    vy *= s;
    vz *= s;
  }
  driftVel = Vec3(vx, vy, vz);
  position = Vec3(position.x + vx * dt, position.y + vy * dt,
                  position.z + vz * dt);
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
