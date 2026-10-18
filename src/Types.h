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

inline bool canJump(const ShipState& s, float costK = 1.0f) {
  return s.fuel >= kJumpFuelCost * costK;
}

// v3.6.0: варп-роли корпусов. Dart (0) и Ring Ship (1) несут варп-двигатель,
// Shuttle (2) — локальный корабль: прыжок недоступен.
inline bool styleCanWarp(int style) { return style != 2; }

inline bool spendJumpFuel(ShipState& s, float costK = 1.0f) {
  const float cost = kJumpFuelCost * costK;
  if (s.fuel < cost) return false;
  s.fuel -= cost;
  if (s.fuel < 0.0f) s.fuel = 0.0f;
  return true;
}

// Fuel regenerates while the camera stays near a star (sun at the origin).
inline void updateFuel(ShipState& s, float dt, bool nearStar, float rateK = 1.0f) {
  if (nearStar && s.fuel < 100.0f) {
    s.fuel += kStarFuelPerSec * rateK * dt;
    if (s.fuel > 100.0f) s.fuel = 100.0f;
  }
}

inline void applyRadiation(ShipState& s, float dt, bool inZone, float dmgK = 1.0f) {
  if (!inZone) return;
  s.shield -= kPulsarDamagePerSec * dmgK * dt;
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

// === v3.6.0: орбитальный ассист ===
// Вблизи массивного центра при тангенциальном курсировании корабль сам
// выходит на круговую орбиту: радиальная скорость мягко гасится, модуль
// тангенциальной подтягивается к v = sqrt(a*r). Работает только в окне
// скоростей (0.55..2.2 от круговой) и при недоминирующем радиальном ходе,
// чтобы не «прилипать» на любом пролёте. Возвращает true, если ассист
// скорректировал курс. Чистая математика — тестируется без GPU.
inline bool orbitAssist(const Vec3& pos, const Vec3& gravAcc, Vec3& inputVel,
                        Vec3& driftVel, double dt) {
  if (dt <= 0.0) return false;
  const double r = std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
  if (r < 1e-3) return false;
  const double a2 =
      gravAcc.x * gravAcc.x + gravAcc.y * gravAcc.y + gravAcc.z * gravAcc.z;
  if (a2 < 1e-12) return false;
  const Vec3 er(pos.x / r, pos.y / r, pos.z / r);  // от центра к кораблю
  double vx = inputVel.x + driftVel.x;
  double vy = inputVel.y + driftVel.y;
  double vz = inputVel.z + driftVel.z;
  const double vr = vx * er.x + vy * er.y + vz * er.z;
  const double tx0 = vx - er.x * vr, ty0 = vy - er.y * vr, tz0 = vz - er.z * vr;
  const double vt = std::sqrt(tx0 * tx0 + ty0 * ty0 + tz0 * tz0);
  const double a = std::sqrt(a2);
  const double vCirc = std::sqrt(a * r);
  if (vCirc < 1e-9) return false;
  if (vt < 0.55 * vCirc || vt > 2.2 * vCirc) return false;
  if (std::fabs(vr) > 0.8 * vt) return false;
  const double kAdj = 1.0 - std::exp(-dt * 1.1);
  const double vrNew = vr * (1.0 - kAdj);
  const double vtNew = vt + (vCirc - vt) * kAdj;
  const double tx = tx0 / vt, ty = ty0 / vt, tz = tz0 / vt;
  const double gx = er.x * vrNew + tx * vtNew;
  const double gy = er.y * vrNew + ty * vtNew;
  const double gz = er.z * vrNew + tz * vtNew;
  // Дрейф плавно всасывается в управляемую скорость; суммарный вектор — цель.
  driftVel = Vec3(driftVel.x * (1.0 - kAdj), driftVel.y * (1.0 - kAdj),
                  driftVel.z * (1.0 - kAdj));
  inputVel = Vec3(gx - driftVel.x, gy - driftVel.y, gz - driftVel.z);
  return true;
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

// === Difficulty === (v3.6.0): пресет сложности из настроек. Множители
// передаются в canJump/spendJumpFuel/updateFuel/applyRadiation (K=1 —
// базовая балансировка, дефолт Normal). hints=false — минимум подсказок.
// v3.8.0: hazardK — множитель урона астероидного поля.
struct DifficultyTuning {
  float jumpCostK = 1.0f;
  float starFuelK = 1.0f;
  float radiationK = 1.0f;
  float hazardK = 1.0f;
  bool hints = true;
};

inline DifficultyTuning difficultyTuning(int difficulty) {
  DifficultyTuning t;
  if (difficulty <= 0) {  // Easy: дешёвый прыжок, быстрая заправка, мягкая радиация
    t.jumpCostK = 0.6f;
    t.starFuelK = 1.6f;
    t.radiationK = 0.6f;
    t.hazardK = 0.6f;
  } else if (difficulty >= 2) {  // Hard: дорогой прыжок, злая радиация, без подсказок
    t.jumpCostK = 1.6f;
    t.starFuelK = 0.7f;
    t.radiationK = 1.5f;
    t.hazardK = 1.6f;
    t.hints = false;
  }
  return t;
}

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
