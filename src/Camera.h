#pragma once

#include "Types.h"

namespace astro {

struct Camera {
  Vec3 position{0.0, 0.0, 10.0};
  double yaw = 0.0;    // radians, around Y
  double pitch = 0.0;  // radians
  double moveSpeed = 5.0;

  // Basis vectors (right-handed, Y up).
  Vec3 forward() const;
  Vec3 right() const;
  Vec3 up() const;

  void moveForward(double dt, bool forward);
  void moveRight(double dt, bool right);
  void clampNearHorizon(double rs);

  // === Ship survival === (v3.3.0): gravitational drift.
  // Integrates a damped, capped drift velocity driven by accel and moves the
  // camera by driftVel * dt. Free-flight controls stay untouched.
  void applyGravityStep(Vec3& driftVel, const Vec3& accel, double dt);
};

}  // namespace astro
