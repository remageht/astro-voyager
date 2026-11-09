#pragma once

#include <cmath>
#include <string>

namespace astro {

// Minimal 3D vector to keep core buildable without GLM.
// GL renderer will use glm::vec3; conversion is 1:1.
// (v3.3.0: moved here from Camera.h so the ship state stays GL-free.)
struct Vec3 {
  double x = 0.0, y = 0.0, z = 0.0;
  Vec3() = default;
  Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
  Vec3 operator+(const Vec3& o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
  Vec3 operator-(const Vec3& o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
  Vec3 operator*(double s) const { return Vec3(x * s, y * s, z * s); }
};

inline Vec3 operator*(double s, const Vec3& v) {
  return Vec3(v.x * s, v.y * s, v.z * s);
}

// === Ship survival mode === (v3.3.0)
// Core-safe ship state (no GL): used by the headless core, the interaction
// tests and the GL app. Units are game units, fuel/shield are 0..100.
struct ShipState {
  float fuel = 100.0f;    // warp jumps cost kJumpFuelCost; regenerates near a star
  float shield = 100.0f;  // pulsar radiation drains it; death at 0
  float hull = 100.0f;    // v4.0.0: armor/panel integrity (visual wear, dock repair)
  Vec3 gravVel;           // gravitational drift velocity (world units/s)
  bool fuelNoticeShown = false;  // one-shot "drift to star" notice flag
  bool boostActive = false;      // conventional boost engaged
  bool isDocked = false;         // v3.11.0: docked at orbital station / shipyard
  std::string dockedStationId;   // id of current dock site
  // v4.6.1: Planetary Landing & Surface Operations
  bool gearDeployed = false;
  float gearProgress = 0.0f;
  bool isLanded = false;
  std::string landedBodyId;
  float suspensionCompression = 0.0f;
};

// === Ship Customization & Shipyard === (v3.11.0)
// Livery, plasma emission, ring spin rate and engineering upgrades.
// Defaults are neutral (tints 1.0, spin 1.0, upgrades 0) so the baseline
// reference screenshots and flight balance remain bit-for-bit identical.
struct ShipCustomization {
  int style = 0;                               // 0 Dart, 1 Ring Ship, 2 Shuttle, 3 Model (Warp Cruiser)
  float primaryColor[3] = {1.0f, 1.0f, 1.0f};  // base hull tint multiplier
  float accentColor[3] = {1.0f, 1.0f, 1.0f};   // secondary livery trim (cyan, gold, dark)
  float emissiveColor[3] = {1.0f, 1.0f, 1.0f}; // plasma glow tint (nozzle, rings, core)
  float ringSpinSpeed = 1.0f;                  // multiplier for warp ring rotation rate (0..3)
  float warpGlowIntensity = 1.0f;              // multiplier for warp field glow (0.2..3.0)
  int engineUpgrade = 0;                       // 0..3: +15% boost speed & fuel efficiency per level
  int shieldUpgrade = 0;                       // 0..3: +20% hazard / radiation resistance per level
  int warpUpgrade = 0;                         // 0..3: -12% jump fuel cost per level
  // === Station Hub v4.7: RP Engineering Matrix (0..3 each) ===
  int warpCapacitorLevel = 0;   // L1 +25% spool/-10% cost, L2 +50%/-20%, L3 +100%/-35%
  int shieldMatrixLevel = 0;    // shield capacity +25%/+50%/+100%, regen scaling
  int sensorArrayLevel = 0;     // target capture range +15%/+30%/+50%
  int rcsGimbalLevel = 0;       // angular maneuverability +15%/+30%/+50%
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
constexpr float kRespawnHull = 100.0f;      // v4.0.0: rebuilt at Sol
constexpr float kGravDampPerSec = 0.35f;    // drift velocity damping
constexpr float kGravVelMax = 2.0f;         // drift speed cap (units/s)
constexpr float kGravMinDist = 0.5f;        // avoids the r->0 singularity

// Conventional Boost balance constants (v3.9.0):
// Форсаж на обычном топливе в досветовом полёте (Left Shift, warp off).
// Расход: kSublightFuelRatePerSec = 2.0 ед./с (полного бака 100 ед. хватает на 50 с).
constexpr float kSublightFuelRatePerSec = 2.0f;
constexpr float kBoostSpeedMultiplier = 2.0f;   // форсаж: множитель скорости x2.0 (x1.7 - x2.2)
constexpr float kBoostRespMultiplier = 1.8f;    // форсаж: множитель приёмистости x1.8

inline bool canBoost(const ShipState& s) {
  return s.fuel > 0.0f;
}

inline bool spendBoostFuel(ShipState& s, float dt, float rateK = 1.0f) {
  if (s.fuel <= 0.0f) {
    s.fuel = 0.0f;
    return false;
  }
  s.fuel -= kSublightFuelRatePerSec * rateK * dt;
  if (s.fuel < 0.0f) s.fuel = 0.0f;
  return true;
}

inline bool spendSublightFuel(ShipState& s, float dt, float rateK = 1.0f) {
  return spendBoostFuel(s, dt, rateK);
}

inline bool canJump(const ShipState& s, float costK = 1.0f) {
  return s.fuel >= kJumpFuelCost * costK;
}

// v3.6.0: варп-роли корпусов. v4.0.0: варп-двигатель несут только Ring Ship
// (1) и Model OBJ (3); Dart (0) и Shuttle (2) — досветовые корпуса.
inline bool styleCanWarp(int style) { return style == 1 || style == 3; }

// v4.0.0: форсаж досветовых корпусов ограничен. У Dart и Shuttle нет
// варп-двигателя, поэтому и разгон у них «досветовый»: x1.35 / x1.20 вместо
// x2.0 у Ring Ship и Model. Приёмистость снижена пропорционально.
inline float styleBoostSpeedMult(int style) {
  switch (style) {
    case 0: return 1.35f;  // Dart: лёгкий перехватчик
    case 2: return 1.20f;  // Shuttle: тяжёлый челнок
    default: return kBoostSpeedMultiplier;
  }
}

inline float styleBoostRespMult(int style) {
  switch (style) {
    case 0: return 1.45f;
    case 2: return 1.30f;
    default: return kBoostRespMultiplier;
  }
}

// v3.11.0: инженерные модификации корпуса и оборудования
inline float upgradeBoostSpeedMult(int lvl) {
  int l = lvl > 3 ? 3 : (lvl < 0 ? 0 : lvl);
  return 1.0f + 0.15f * static_cast<float>(l);
}

inline float upgradeShieldDamageFactor(int lvl) {
  int l = lvl > 3 ? 3 : (lvl < 0 ? 0 : lvl);
  return 1.0f / (1.0f + 0.20f * static_cast<float>(l));
}

inline float upgradeJumpCostFactor(int lvl) {
  int l = lvl > 3 ? 3 : (lvl < 0 ? 0 : lvl);
  return 1.0f - 0.12f * static_cast<float>(l);
}

// === Station Hub v4.7: RP Engineering Matrix helpers ===
inline int upgradeCostRP(int lvl) {  // next level cost for generic branch
  // Warp: 150/300/500, others scale similarly
  if (lvl <= 0) return 150;
  if (lvl == 1) return 300;
  return 500;
}
inline int warpCapacitorCostRP(int lvl) {
  if (lvl <= 0) return 150;
  if (lvl == 1) return 300;
  return 500;
}
inline float warpCapacitorSpeedMult(int lvl) {
  int l = lvl > 3 ? 3 : (lvl < 0 ? 0 : lvl);
  static constexpr float kT[4] = {1.0f, 1.25f, 1.5f, 2.0f};
  return kT[l];
}
inline float warpCapacitorCostFactor(int lvl) {
  int l = lvl > 3 ? 3 : (lvl < 0 ? 0 : lvl);
  static constexpr float kT[4] = {1.0f, 0.90f, 0.80f, 0.65f};
  return kT[l];
}
inline float shieldMatrixCapacityMult(int lvl) {
  int l = lvl > 3 ? 3 : (lvl < 0 ? 0 : lvl);
  static constexpr float kT[4] = {1.0f, 1.25f, 1.5f, 2.0f};
  return kT[l];
}
inline float shieldMatrixRegenMult(int lvl) {
  int l = lvl > 3 ? 3 : (lvl < 0 ? 0 : lvl);
  static constexpr float kT[4] = {1.0f, 1.25f, 1.5f, 2.0f};
  return kT[l];
}
inline float sensorArrayRangeMult(int lvl) {
  int l = lvl > 3 ? 3 : (lvl < 0 ? 0 : lvl);
  static constexpr float kT[4] = {1.0f, 1.15f, 1.30f, 1.50f};
  return kT[l];
}
inline float rcsGimbalMult(int lvl) {
  int l = lvl > 3 ? 3 : (lvl < 0 ? 0 : lvl);
  static constexpr float kT[4] = {1.0f, 1.15f, 1.30f, 1.50f};
  return kT[l];
}

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

// v4.0.0: износ корпуса (0..100). Это не смерть: щит остаётся единственным
// смертельным ресурсом, а корпус отвечает за «побитый/восстановленный» вид
// OBJ-корпуса и чинится в доке.
inline void damageHull(ShipState& s, float amount) {
  if (amount <= 0.0f) return;
  s.hull -= amount;
  if (s.hull < 0.0f) s.hull = 0.0f;
}

inline void repairHull(ShipState& s) { s.hull = 100.0f; }

inline float hullWear(const ShipState& s) {
  const float h = s.hull < 0.0f ? 0.0f : (s.hull > 100.0f ? 100.0f : s.hull);
  return 1.0f - h / 100.0f;
}

inline void applyRadiation(ShipState& s, float dt, bool inZone, float dmgK = 1.0f) {
  if (!inZone) return;
  const float dmg = kPulsarDamagePerSec * dmgK * dt;
  s.shield -= dmg;
  if (s.shield < 0.0f) s.shield = 0.0f;
  // v4.0.0: радиация прожигает и обшивку — корпус копит визуальный износ.
  damageHull(s, dmg * 0.35f);
}

inline bool isDead(const ShipState& s) { return s.shield <= 0.0f; }

inline void respawn(ShipState& s) {
  s.shield = kRespawnShield;
  s.fuel = kRespawnFuel;
  s.hull = kRespawnHull;
  s.gravVel = Vec3();
  s.fuelNoticeShown = false;
  s.boostActive = false;
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

// === v5.3.0 Alpha: Диегетическое орбитальное курсирование (Keplerian Parking Orbit) ===
struct OrbitalCruiseTelemetry {
  bool active = false;
  bool locked = false;
  double radius = 0.0;           // Расстояние до центра гравитации (мир)
  double altitudeKm = 0.0;       // Высота над поверхностью (км)
  double speed = 0.0;            // Текущая орбитальная скорость
  double circularSpeed = 0.0;    // Первая космическая скорость v = sqrt(a * r)
  double periodSec = 0.0;        // Период витка в секундах T = 2 * pi * r / v
  double trueAnomalyDeg = 0.0;   // Фаза витка (0..360 deg)
  Vec3 orbitalNormal{0.0, 1.0, 0.0}; // Нормаль к плоскости орбиты
  std::string bodyName = "";     // Имя небесного тела
};

inline bool stepOrbitalCruise(const Vec3& pos, const Vec3& gravAcc,
                              Vec3& inputVel, Vec3& driftVel,
                              double bodyRadius, double dt,
                              double throttleAdjust, // >0: W (подъём), <0: S (деорбитальный сход)
                              OrbitalCruiseTelemetry& telem) {
  if (dt <= 0.0) return false;
  const double r = std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
  if (r < 1e-3) return false;
  const double a2 = gravAcc.x * gravAcc.x + gravAcc.y * gravAcc.y + gravAcc.z * gravAcc.z;
  if (a2 < 1e-12) return false;
  const double a = std::sqrt(a2);
  const double vCirc = std::sqrt(a * r);
  if (vCirc < 1e-6) return false;

  const Vec3 er(pos.x / r, pos.y / r, pos.z / r);
  double vx = inputVel.x + driftVel.x;
  double vy = inputVel.y + driftVel.y;
  double vz = inputVel.z + driftVel.z;
  const double vr = vx * er.x + vy * er.y + vz * er.z;

  Vec3 vtVec(vx - er.x * vr, vy - er.y * vr, vz - er.z * vr);
  double vt = std::sqrt(vtVec.x * vtVec.x + vtVec.y * vtVec.y + vtVec.z * vtVec.z);

  if (vt < 0.1 * vCirc) {
    Vec3 refNorm = (std::abs(er.y) < 0.9) ? Vec3(0.0, 1.0, 0.0) : Vec3(1.0, 0.0, 0.0);
    vtVec = Vec3(refNorm.y * er.z - refNorm.z * er.y,
                 refNorm.z * er.x - refNorm.x * er.z,
                 refNorm.x * er.y - refNorm.y * er.x);
    double l = std::sqrt(vtVec.x * vtVec.x + vtVec.y * vtVec.y + vtVec.z * vtVec.z);
    if (l > 1e-6) {
      vtVec.x /= l; vtVec.y /= l; vtVec.z /= l;
      vt = 0.5 * vCirc;
    }
  }

  Vec3 vtDir = (vt > 1e-6) ? Vec3(vtVec.x / vt, vtVec.y / vt, vtVec.z / vt) : Vec3(0.0, 0.0, 1.0);

  Vec3 N(er.y * vtDir.z - er.z * vtDir.y,
         er.z * vtDir.x - er.x * vtDir.z,
         er.x * vtDir.y - er.y * vtDir.x);
  double nLen = std::sqrt(N.x * N.x + N.y * N.y + N.z * N.z);
  if (nLen > 1e-6) {
    N.x /= nLen; N.y /= nLen; N.z /= nLen;
  } else {
    N = Vec3(0.0, 1.0, 0.0);
  }

  double desiredRadiusRate = throttleAdjust * (vCirc * 0.35);
  double targetVr = desiredRadiusRate;
  double targetVt = vCirc * (1.0 + throttleAdjust * 0.1);

  const double kAdj = 1.0 - std::exp(-dt * 2.5);
  const double vrNew = vr + (targetVr - vr) * kAdj;
  const double vtNew = vt + (targetVt - vt) * kAdj;

  const Vec3 vGoal(er.x * vrNew + vtDir.x * vtNew,
                   er.y * vrNew + vtDir.y * vtNew,
                   er.z * vrNew + vtDir.z * vtNew);

  driftVel = Vec3(driftVel.x * (1.0 - kAdj),
                  driftVel.y * (1.0 - kAdj),
                  driftVel.z * (1.0 - kAdj));
  inputVel = Vec3(vGoal.x - driftVel.x,
                  vGoal.y - driftVel.y,
                  vGoal.z - driftVel.z);

  telem.active = true;
  telem.radius = r;
  double alt = r - bodyRadius;
  if (alt < 0.0) alt = 0.0;
  telem.altitudeKm = alt * 100.0;
  telem.speed = vt;
  telem.circularSpeed = vCirc;
  telem.periodSec = (vCirc > 1e-4) ? (2.0 * 3.14159265358979323846 * r / vCirc) : 0.0;
  telem.orbitalNormal = N;
  telem.locked = (std::abs(vr) < 0.18 * vCirc) && (std::abs(vt - vCirc) < 0.25 * vCirc);

  double angle = std::atan2(pos.z, pos.x) * (180.0 / 3.14159265358979323846);
  if (angle < 0.0) angle += 360.0;
  telem.trueAnomalyDeg = angle;

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
// zoomK (v4.0): множитель сноса колёсиком; 1 = дефолт.
inline Vec3 offsetCameraPos(const Vec3& pos, const Vec3& fwd, ViewMode v,
                            float zoomK = 1.0f) {
  if (v == ViewMode::Cockpit) return pos;
  const double zk = static_cast<double>(zoomK);
  const double back = ((v == ViewMode::Chase) ? kChaseBack : kFarBack) * zk;
  const double up = (((v == ViewMode::Chase) ? kChaseUp : kFarUp) - 0.35) * zk +
                    0.35;  // подъём почти пропорционален, но не уходит в 0
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

// === Safe O(1) angle normalization in [-pi, pi] ===
namespace math {
inline double normalizeAngleSigned(double angle) {
  if (!std::isfinite(angle)) return 0.0;
  constexpr double kPi = 3.14159265358979323846;
  constexpr double kTwoPi = 2.0 * kPi;
  return std::remainder(angle, kTwoPi);
}
}  // namespace math

using math::normalizeAngleSigned;

// === Accessibility & Ergonomics (v5.4.0) ===
enum class ColorblindPalette : int {
  Default = 0,      // Cyan (#00E5FF) & Amber (#FFB300)
  Protanopia = 1,   // Blue (#3388FF) & Yellow (#FFE600) - Red-blind
  Deuteranopia = 2, // Cyan (#00B4D8) & Orange (#F77F00) - Green-blind
  Tritanopia = 3,   // Teal (#00F5D4) & Magenta/Rose (#FF007F) - Blue-blind
  HighContrast = 4  // Pure White (#FFFFFF) & Pure Yellow (#FFFF00) - WCAG AAA
};

struct AccessiblePalette {
  Vec3 primary;     // reticle, active borders, main HUD
  Vec3 secondary;   // secondary readouts, speed, distance
  Vec3 warning;     // cautions, fuel warning
  Vec3 hazard;      // critical warnings, blackout, overheat
  Vec3 success;     // dock ready, sample collected
  Vec3 background;  // HUD panel backing
};

inline AccessiblePalette getAccessiblePalette(int mode) {
  AccessiblePalette p;
  switch (mode) {
    case 1:  // Protanopia (Red-blind: rich Blue, Amber warning, Pure White hazard)
      p.primary = Vec3{0.20f, 0.55f, 1.0f};
      p.secondary = Vec3{0.45f, 0.75f, 1.0f};
      p.warning = Vec3{1.0f, 0.85f, 0.10f};
      p.hazard = Vec3{1.0f, 1.0f, 1.0f};
      p.success = Vec3{0.30f, 0.85f, 0.95f};
      p.background = Vec3{0.02f, 0.031f, 0.055f};
      break;
    case 2:  // Deuteranopia (Green-blind: Vivid Cyan, High-contrast Amber/Orange)
      p.primary = Vec3{0.0f, 0.75f, 0.95f};
      p.secondary = Vec3{0.35f, 0.65f, 0.95f};
      p.warning = Vec3{1.0f, 0.60f, 0.0f};
      p.hazard = Vec3{1.0f, 0.35f, 0.05f};
      p.success = Vec3{0.20f, 0.75f, 1.0f};
      p.background = Vec3{0.02f, 0.031f, 0.055f};
      break;
    case 3:  // Tritanopia (Blue-blind: Vibrant Aqua/Teal, Warm Orange, Deep Magenta)
      p.primary = Vec3{0.0f, 0.85f, 0.75f};
      p.secondary = Vec3{0.30f, 0.90f, 0.70f};
      p.warning = Vec3{1.0f, 0.55f, 0.10f};
      p.hazard = Vec3{0.95f, 0.10f, 0.35f};
      p.success = Vec3{0.15f, 0.95f, 0.65f};
      p.background = Vec3{0.02f, 0.031f, 0.055f};
      break;
    case 4:  // High Contrast (WCAG AAA: 100% luminance separation, Neon Green success)
      p.primary = Vec3{1.0f, 1.0f, 1.0f};
      p.secondary = Vec3{0.85f, 0.95f, 1.0f};
      p.warning = Vec3{1.0f, 1.0f, 0.0f};
      p.hazard = Vec3{1.0f, 0.0f, 0.45f};
      p.success = Vec3{0.0f, 1.0f, 0.25f};
      p.background = Vec3{0.0f, 0.0f, 0.0f};
      break;
    case 0:  // Default Sci-Fi (Cyan & Amber)
    default:
      p.primary = Vec3{0.0f, 0.898f, 1.0f};
      p.secondary = Vec3{0.22f, 0.74f, 0.97f};
      p.warning = Vec3{1.0f, 0.75f, 0.0f};
      p.hazard = Vec3{1.0f, 0.25f, 0.25f};
      p.success = Vec3{0.20f, 0.95f, 0.50f};
      p.background = Vec3{0.02f, 0.031f, 0.055f};
      break;
  }
  return p;
}

// === Docking Realism: Telemetry, 3D Corridor & RCS (Issue #65) ===
namespace docking {

struct DockTelemetry {
  float rangeMeters = 0.0f;        // Physical distance in meters (1 world unit = 25m)
  float closingRateMps = 0.0f;     // Closing speed in m/s (<0 = closing, >0 = opening)
  float yawErrorDeg = 0.0f;        // Shortest-arc yaw deviation in degrees
  float pitchErrorDeg = 0.0f;      // Shortest-arc pitch deviation in degrees
  float lateralErrorMeters = 0.0f; // Deviation from docking corridor axis
  bool inCorridor = false;         // Within approach funnel (< 3.0 units, < 1.5 units off-axis)
  bool axisAligned = false;        // Angular error <= 2.5 deg & lateral <= 0.45m
  bool captureReady = false;       // Near latch range (< 0.55 units) & aligned
};

// Computes telemetry between ship and target docking port
inline DockTelemetry computeTelemetry(const Vec3& shipPos, const Vec3& shipVel,
                                      double shipYaw, double shipPitch,
                                      const Vec3& portPos, const Vec3& portNormal,
                                      double targetYaw, double targetPitch) {
  DockTelemetry t;
  const double dx = shipPos.x - portPos.x;
  const double dy = shipPos.y - portPos.y;
  const double dz = shipPos.z - portPos.z;
  const double distWorld = std::sqrt(dx * dx + dy * dy + dz * dz);
  t.rangeMeters = static_cast<float>(distWorld * 25.0);

  // Closing rate along line-of-sight: negative means closing in
  if (distWorld > 1e-4) {
    const double dirX = -dx / distWorld;
    const double dirY = -dy / distWorld;
    const double dirZ = -dz / distWorld;
    const double relVelProj = shipVel.x * dirX + shipVel.y * dirY + shipVel.z * dirZ;
    t.closingRateMps = static_cast<float>(-relVelProj * 25.0);
  }

  // Shortest-arc angular deviations: O(1) remainder guaranteed finite & bounded
  const double dYaw = normalizeAngleSigned(targetYaw - shipYaw);
  t.yawErrorDeg = static_cast<float>(dYaw * 180.0 / 3.14159265358979323846);

  const double dPitch = normalizeAngleSigned(targetPitch - shipPitch);
  t.pitchErrorDeg = static_cast<float>(dPitch * 180.0 / 3.14159265358979323846);

  // Longitudinal & lateral positions along corridor axis
  const double projDist = dx * portNormal.x + dy * portNormal.y + dz * portNormal.z;
  const double perpX = dx - projDist * portNormal.x;
  const double perpY = dy - projDist * portNormal.y;
  const double perpZ = dz - projDist * portNormal.z;
  const double latDist = std::sqrt(perpX * perpX + perpY * perpY + perpZ * perpZ);
  t.lateralErrorMeters = static_cast<float>(latDist * 25.0);

  t.inCorridor = (projDist > -0.25 && projDist < 3.5 && latDist < 1.6);
  t.axisAligned = (std::abs(t.yawErrorDeg) <= 2.5f &&
                   std::abs(t.pitchErrorDeg) <= 2.5f &&
                   t.lateralErrorMeters <= 0.45f);
  t.captureReady = (distWorld <= 0.55 && t.axisAligned);

  return t;
}

// 4 concentric corridor gate distances along docking axis
constexpr float kCorridorGateDistances[4] = {0.45f, 0.95f, 1.55f, 2.25f};

// Evaluates whether a corridor gate has been entered / passed
inline bool isGatePassed(float projDist, float gateDist) {
  return projDist <= (gateDist + 0.15f);
}

}  // namespace docking

}  // namespace astro
