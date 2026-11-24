// test_colony_orbit.cpp — РЕГРЕССИОННЫЕ ТЕСТЫ (issue #ED): мягкие гуарды
// в colonyAnchorNormal/colonySurfaceFrame и поведение sqrt(a*r)-орбит
// (orbitAssist, stepOrbitalCruise из Types.h). Чистые числа — без GPU/GL.
//
// Что тестируем (core, без GPU/GL):
//  1. colonyAnchorNormal: для всех anchor-id → конечные векторы;
//     для незнакомого id → kDefault (никогда не NaN).
//  2. colonySurfaceFrame: анкор и тангенсы — конечные, |anchor|≈1.
//  3. orbitAssist: тангенциальный вектор втягивается к v = sqrt(a*r),
//     модификация вектора не вносит NaN.
//  4. stepOrbitalCruise: на скорости v = 0.1*vCirc запускает каждый кадр,
//     telem становится не NaN, |inputVel| не уходит в ноль.

#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "ColonySystem.h"
#include "Types.h"

namespace {

int g_fail = 0;

bool finite(const astro::Vec3& v) {
  return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

void check(bool ok, const std::string& what) {
  if (!ok) {
    std::cerr << "FAIL: " << what << "\n";
    ++g_fail;
  } else {
    std::cout << "PASS: " << what << "\n";
  }
}

void expectFiniteVec(const astro::Vec3& v, const std::string& what) {
  char buf[128];
  std::snprintf(buf, sizeof(buf), "%s (finite)", what.c_str());
  check(finite(v), buf);
}

}  // namespace

int main() {
  // 1. colonyAnchorNormal — все известные anchor id: конечные, длина 1.
  {
    static const char* kIds[] = {"moon",     "mars",     "mercury", "europa",
                                 "ganymede",  "callisto", "titan",   "hercules-b"};
    for (const char* id : kIds) {
      const float* n = astro::colonyAnchorNormal(id);
      check(n && finite(astro::Vec3(n[0], n[1], n[2])),
            std::string("colonyAnchorNormal finite for ") + id);
      const float len = std::sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
      check(len > 0.5f && len < 1.5f,
            (std::string("colonyAnchorNormal len for ") + id).c_str());
    }
  }

  // 1b. Незнакомый id → kDefault, конечный.
  {
    const float* n = astro::colonyAnchorNormal("nope-world");
    check(n && finite(astro::Vec3(n[0], n[1], n[2])),
          "colonyAnchorNormal unknown id -> finite default");
  }

  // 2. colonySurfaceFrame — анкор и тангенсы конечны.
  {
    static const char* kIds[] = {"moon", "mars", "europa", "titan", "hercules-b"};
    for (const char* id : kIds) {
      const astro::ColonySurfaceFrame f = astro::colonySurfaceFrame(id);
      expectFiniteVec(astro::Vec3(f.anchor[0], f.anchor[1], f.anchor[2]),
                      (std::string("frame anchor ") + id).c_str());
      expectFiniteVec(astro::Vec3(f.tangentU[0], f.tangentU[1], f.tangentU[2]),
                      (std::string("frame tangentU ") + id).c_str());
      expectFiniteVec(astro::Vec3(f.tangentV[0], f.tangentV[1], f.tangentV[2]),
                      (std::string("frame tangentV ") + id).c_str());
      const float du = std::sqrt(f.tangentU[0] * f.tangentU[0] + f.tangentU[1] * f.tangentU[1] +
                                 f.tangentU[2] * f.tangentU[2]);
      const float dv = std::sqrt(f.tangentV[0] * f.tangentV[0] + f.tangentV[1] * f.tangentV[1] +
                                 f.tangentV[2] * f.tangentV[2]);
      check(du > 0.9f && dv > 0.9f,
            (std::string("frame tangents normalized ") + id).c_str());
    }
  }

  // 3. orbitAssist — тангенциальная скорость втягивается к v = sqrt(a*r).
  {
    // Позиция в (r, 0, 0); центр тяготения (-a, 0, 0).
    const double r = 7000.0;          // radius in "units"
    const double g = 9.81;
    astro::Vec3 pos(r, 0.0, 0.0);
    astro::Vec3 gravAcc(-g, 0.0, 0.0);
    astro::Vec3 inputVel(0.0, 0.9 * std::sqrt(g * r), 0.0);  // 0.9..2.2 of vCirc OK
    astro::Vec3 driftVel(0.0, 0.0, 0.0);
    const bool ok = astro::ship::orbitAssist(pos, gravAcc, inputVel, driftVel, 0.016f);
    check(ok, "orbitAssist returns true for in-window tangential speed");
    const double vtNew = std::sqrt(inputVel.x * inputVel.x +
                                   inputVel.y * inputVel.y +
                                   inputVel.z * inputVel.z);
    const double vCirc = std::sqrt(g * r);
    check(finite(inputVel), "orbitAssist: inputVel finite");
    check(vtNew > vCirc * 0.9, "orbitAssist: speed pulls toward v = sqrt(a*r)");
  }

  // 4. stepOrbitalCruise — сходит к vCirc за десятки шагов (kAdj ≈ 0.04 на dt=0.016).
  {
    const double r = 7000.0;
    const double g = 9.81;
    const astro::Vec3 pos(r, 0.0, 0.0);
    const astro::Vec3 gravAcc(-g, 0.0, 0.0);
    astro::Vec3 inputVel(0.0, 0.1 * std::sqrt(g * r), 0.0);
    astro::Vec3 driftVel(0.0, 0.0, 0.0);
    astro::ship::OrbitalCruiseTelemetry telem;
    bool locked = false;
    for (int i = 0; i < 40 && !locked; ++i) {
      const bool ok = astro::ship::stepOrbitalCruise(pos, gravAcc, inputVel, driftVel, r * 10.0,
                                               0.016f, 0.0f, telem);
      if (!ok) break;
      locked = telem.locked;
      check(finite(inputVel), "stepOrbitalCruise: inputVel finite");
      check(finite(driftVel), "stepOrbitalCruise: driftVel finite");
    }
    check(locked, "stepOrbitalCruise: teleport ships get locked to vCirc");
    check(std::isfinite(telem.speed) && std::isfinite(telem.circularSpeed),
          "stepOrbitalCruise: telemetry finite");
    check(telem.speed > 0.4 * telem.circularSpeed, "stepOrbitalCruise: speed near vCirc");
  }

  if (g_fail > 0) {
    std::cerr << g_fail << " check(s) FAILED\n";
    return 1;
  }
  std::cout << "colony+orbit tests: ALL PASS\n";
  return 0;
}
