// test_kerr.cpp (issue #159): устойчивость kerrBCrit на сингулярности.
//
// Проблема: прежняя формула -(r^3-3Mr^2+a^2r+Ma^2)/(a(r_ph-M)) при a -> M
// даёт 0/0 -> NaN, а защита через абсолютный eps = 1e-7 не срабатывала на
// игровых масштабах (Sgr A* rs = 2.95e9 м, M87* rs = 6.6e9 м).
// Теперь используется устойчивая форма r_ph(r_ph+3M)/(2*sqrt(M*r_ph)).

#include <cmath>
#include <cstdio>
#include <iostream>

#include "Kerr.h"

namespace {

int g_fail = 0;

void check(bool ok, const char* what) {
  if (!ok) {
    std::cerr << "FAIL: " << what << "\n";
    ++g_fail;
  } else {
    std::cout << "PASS: " << what << "\n";
  }
}

void expectNear(const char* what, double got, double want, double relTol) {
  const double denom = (std::abs(want) > 0.0) ? std::abs(want) : 1.0;
  const double rel = std::abs(got - want) / denom;
  const bool ok = std::isfinite(got) && (rel <= relTol);
  if (!ok) {
    std::cerr << "FAIL: " << what << ": got=" << got << " want=" << want
              << " rel=" << rel << "\n";
    ++g_fail;
  } else {
    std::cout << "PASS: " << what << " (rel=" << rel << ")\n";
  }
}

}  // namespace

int main() {
  // 1. Экстремум a = M: точные пределы 2M (prograde) и 7M (retrograde)
  //    на трёх масштабах (раньше — NaN).
  {
    const double rsList[] = {1.0, 2953.0, 2.95e9, 6.6e9};
    for (double rs : rsList) {
      const double M = 0.5 * rs;
      char buf[96];
      std::snprintf(buf, sizeof(buf), "a=M prograde rs=%.4g -> 2M", rs);
      expectNear(buf, astro::kerrBCrit(M, rs, true), 2.0 * M, 1e-12);
      std::snprintf(buf, sizeof(buf), "a=M retrograde rs=%.4g -> 7M", rs);
      expectNear(buf, astro::kerrBCrit(M, rs, false), 7.0 * M, 1e-12);
    }
  }

  // 2. Предел Шварцшильда a -> 0: b_crit = 3*sqrt(3)*M = 1.5*sqrt(3)*rs.
  {
    expectNear("a=0 -> 1.5*sqrt(3)*Rs", astro::kerrBCrit(0.0, 1.0, true),
               1.5 * std::sqrt(3.0), 1e-12);
    expectNear("a=0 -> 1.5*sqrt(3)*Rs (Sgr A* scale)",
               astro::kerrBCrit(0.0, 2.95e9, true),
               1.5 * std::sqrt(3.0) * 2.95e9, 1e-12);
  }

  // 3. Литературные значения (Bardeen 1973) при a = 0.9M.
  {
    const double rs = 1.0;
    const double M = 0.5 * rs;
    expectNear("a=0.9M prograde -> 2.84442 M",
               astro::kerrBCrit(0.9 * M, rs, true) / M, 2.84442, 1e-5);
    expectNear("a=0.9M retrograde -> 6.83232 M",
               astro::kerrBCrit(0.9 * M, rs, false) / M, 6.83232, 1e-5);
  }

  // 4. Непрерывность на экстремуме: разрыва быть не должно.
  {
    const double rs = 1.0;
    const double M = 0.5 * rs;
    const double bNear = astro::kerrBCrit(0.999999 * M, rs, true);
    const double bAt = astro::kerrBCrit(M, rs, true);
    expectNear("no jump at a -> M (prograde)", bNear, bAt, 5e-3);
    const double rNear = astro::kerrBCrit(0.999999 * M, rs, false);
    const double rAt = astro::kerrBCrit(M, rs, false);
    expectNear("no jump at a -> M (retrograde)", rNear, rAt, 5e-3);
  }

  // 5. Масштабная инвариантность: b_crit линейно по rs.
  {
    const double rs1 = 1.0;
    const double rs2 = 2.95e9;
    const double b1 = astro::kerrBCrit(0.45 * 0.5 * rs1, rs1, true);
    const double b2 = astro::kerrBCrit(0.45 * 0.5 * rs2, rs2, true);
    expectNear("scale invariance (rs=1 vs 2.95e9)", b1 * rs2, b2, 1e-12);
  }

  // 6. Монотонность по |a| и порядок prograde < Schwarzschild < retrograde.
  {
    const double rs = 1.0;
    const double M = 0.5 * rs;
    double prevP = astro::kerrBCrit(0.0, rs, true);
    double prevR = astro::kerrBCrit(0.0, rs, false);
    bool mono = true;
    for (int i = 1; i <= 50; ++i) {
      const double a = M * (i / 50.0);
      const double p = astro::kerrBCrit(a, rs, true);
      const double r = astro::kerrBCrit(a, rs, false);
      if (!(p <= prevP) || !(r >= prevR)) mono = false;
      prevP = p;
      prevR = r;
    }
    check(mono, "b_crit monotonic in |a|");
    const double schw = 1.5 * std::sqrt(3.0) * rs;
    check(astro::kerrBCrit(0.9 * M, rs, true) < schw &&
              schw < astro::kerrBCrit(0.9 * M, rs, false),
          "prograde < Schwarzschild < retrograde");
  }

  // 7. Вырожденные входы не должны давать NaN.
  {
    check(std::isfinite(astro::kerrBCrit(0.0, 0.0, true)), "rs=0 -> finite");
    check(std::isfinite(astro::kerrBCrit(1e30, 1.0, true)), "|a| > M -> finite");
    check(std::isfinite(astro::kerrBCrit(1e30, 1.0, false)),
          "|a| > M retrograde -> finite");
    check(std::isfinite(astro::kerrPhotonSphereRadius(1e30, 1.0, true)),
          "photon sphere |a| > M -> finite");
  }

  if (g_fail > 0) {
    std::cerr << g_fail << " check(s) FAILED\n";
    return 1;
  }
  std::cout << "kerr tests: ALL PASS\n";
  return 0;
}
