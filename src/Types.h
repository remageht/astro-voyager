#pragma once

#include <cmath>

namespace astro {

// Minimal 3D vector to keep core buildable without GLM.
// GL renderer will use glm::vec3; conversion is 1:1.
// (v3.3.0: moved here from Camera.h so the ship state stays GL-free.)
struct Vec3 {
  double x = 0.0, y = 0.0, z = 0.0;
  Vec3() = default;
  Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
};

// === Ship survival mode === (v3.3.0)
// Core-safe ship state (no GL): used by the headless core, the interaction
// tests and the GL app. Units are game units, fuel/shield are 0..100.
struct ShipState {
  float fuel = 100.0f;    // warp jumps cost kJumpFuelCost; regenerates near a star
  float shield = 100.0f;  // pulsar radiation drains it; death at 0
  Vec3 gravVel;           // gravitational drift velocity (world units/s)
  bool fuelNoticeShown = false;  // one-shot "drift to star" notice flag
};

namespace ship {

// Balance constants (single source of truth for app + tests).
constexpr float kJumpFuelCost = 5.0f;
constexpr float kPulsarRadius = 1.0f;       // pulsar "radius" in world units
constexpr float kPulsarDamageRadii = 5.0f;  // radiation zone = 5 radii
constexpr float kPulsarDamagePerSec = 10.0f;
constexpr float kStarFuelRange = 10.0f;     // star proximity counted from center
constexpr float kStarFuelPerSec = 2.0f;
constexpr float kLowShieldChroma = 30.0f;   // chromatic aberration gate
constexpr float kLowShieldStatic = 10.0f;   // interference noise gate
constexpr float kRespawnShield = 50.0f;     // after death, back at sol
constexpr float kRespawnFuel = 30.0f;
constexpr float kGravDampPerSec = 0.35f;    // drift velocity damping
constexpr float kGravVelMax = 2.0f;         // drift speed cap (units/s)
constexpr float kGravMinDist = 0.5f;        // avoids the r->0 singularity

inline bool canJump(const ShipState& s) { return s.fuel >= kJumpFuelCost; }

inline bool spendJumpFuel(ShipState& s) {
  if (!canJump(s)) return false;
  s.fuel -= kJumpFuelCost;
  if (s.fuel < 0.0f) s.fuel = 0.0f;
  return true;
}

// Fuel regenerates while the camera stays near a star (sun at the origin).
inline void updateFuel(ShipState& s, float dt, bool nearStar) {
  if (nearStar && s.fuel < 100.0f) {
    s.fuel += kStarFuelPerSec * dt;
    if (s.fuel > 100.0f) s.fuel = 100.0f;
  }
}

inline void applyRadiation(ShipState& s, float dt, bool inZone) {
  if (!inZone) return;
  s.shield -= kPulsarDamagePerSec * dt;
  if (s.shield < 0.0f) s.shield = 0.0f;
}

inline bool isDead(const ShipState& s) { return s.shield <= 0.0f; }

inline void respawn(ShipState& s) {
  s.shield = kRespawnShield;
  s.fuel = kRespawnFuel;
  s.gravVel = Vec3();
  s.fuelNoticeShown = false;
}

// Newtonian pull of a point mass: a = GM / r^2 (GM = catalogue mass field).
inline double gravAccel(double gm, double dist) {
  if (dist < kGravMinDist) dist = kGravMinDist;
  return gm / (dist * dist);
}

inline Vec3 gravAccelToward(double gm, const Vec3& pos, const Vec3& bodyPos) {
  const double dx = bodyPos.x - pos.x;
  const double dy = bodyPos.y - pos.y;
  const double dz = bodyPos.z - pos.z;
  const double d = std::sqrt(dx * dx + dy * dy + dz * dz);
  if (d < 1e-9) return Vec3();
  const double a = gravAccel(gm, d);
  return Vec3(dx / d * a, dy / d * a, dz / d * a);
}

// Low-shield screen effect gates (0 when the shield is healthy).
inline float chromaK(float shield) {
  if (shield >= kLowShieldChroma) return 0.0f;
  return (kLowShieldChroma - shield) / kLowShieldChroma;
}

inline float staticK(float shield) {
  if (shield >= kLowShieldStatic) return 0.0f;
  return (kLowShieldStatic - shield) / kLowShieldStatic;
}

// Shield impact flash (v3.3.0): вспышка при уроне щиту — импульс и затухание.
constexpr float kShieldFlashDecayPerSec = 3.0f;
inline float shieldFlashHit(float f, float dmg) {
  const float v = f + dmg * 0.35f;
  return v > 1.0f ? 1.0f : v;
}
inline float shieldFlashDecay(float f, float dt) {
  return f * std::exp(-dt * kShieldFlashDecayPerSec);
}

}  // namespace ship

// === Camera views === (v3.3.0): cockpit / behind the ship / far overview.
// Toggled with V; pure helpers live here so the core tests cover them too.
namespace view {
enum class ViewMode { Cockpit = 0, Chase = 1, Overview = 2 };

inline ViewMode nextView(ViewMode v) {
  return static_cast<ViewMode>((static_cast<int>(v) + 1) % 3);
}

constexpr double kChaseBack = 4.5;  // world units behind the ship
constexpr double kChaseUp = 1.1;
constexpr double kFarBack = 9.0;
constexpr double kFarUp = 2.2;

// Render-camera position for a view mode (Cockpit = the camera itself).
inline Vec3 offsetCameraPos(const Vec3& pos, const Vec3& fwd, ViewMode v) {
  if (v == ViewMode::Cockpit) return pos;
  const double back = (v == ViewMode::Chase) ? kChaseBack : kFarBack;
  const double up = (v == ViewMode::Chase) ? kChaseUp : kFarUp;
  return Vec3(pos.x - fwd.x * back, pos.y - fwd.y * back + up,
              pos.z - fwd.z * back);
}

// Speed FX amount 0..1 from a speed (units/s) against a reference speed.
inline float speedFx(float speed, float ref) {
  if (ref <= 0.0f) return 0.0f;
  const float k = speed / ref;
  return k < 0.0f ? 0.0f : (k > 1.0f ? 1.0f : k);
}
}  // namespace view

}  // namespace astro
