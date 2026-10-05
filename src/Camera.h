#pragma once

namespace astro {

// Minimal 3D vector to keep core buildable without GLM.
// GL renderer will use glm::vec3; conversion is 1:1.
struct Vec3 {
  double x = 0.0, y = 0.0, z = 0.0;
  Vec3() = default;
  Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
};

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
};

}  // namespace astro
